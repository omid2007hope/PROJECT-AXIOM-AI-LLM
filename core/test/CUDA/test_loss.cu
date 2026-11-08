// test loss for anomaly at kernel level

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cuda_runtime.h>
#include <iostream>
#include <vector>

#include "../../src/CUDA/gpu/download.cuh"
#include "../../src/CUDA/kernel/backward_pass.cuh"
#include "../../src/CUDA/util/gpu_types.cuh"
#include "../../src/CUDA/util/memory_allocation.cuh"

// ─── ANSI Terminal Color Formatting ──────────────────────────────────────────
#define COLOR_RED   "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RESET "\033[0m"

// ─── Range & Finite Assertion Helpers ────────────────────────────────────────
static bool check_scalar_bounds(float actual_value,
								float minimum_bound,
								float maximum_bound,
								const char* check_label) {
	if (!std::isfinite(actual_value) || actual_value < minimum_bound ||
		actual_value > maximum_bound) {
		std::cerr << COLOR_RED << "  [FAIL] " << check_label << ": " << actual_value << " outside ["
				  << minimum_bound << ", " << maximum_bound << "]" << COLOR_RESET << "\n";
		return false;
	}
	std::cout << COLOR_GREEN << "  [PASS] " << check_label << ": " << actual_value
			  << " is within bounds" << COLOR_RESET << "\n";
	return true;
}

static bool check_tensor_bounds(const std::vector<float>& tensor,
								float minimum_bound,
								float maximum_bound,
								const char* check_label) {
	float observed_min = 1e30f;
	float observed_max = -1e30f;
	bool elements_valid = true;

	for (float value : tensor) {
		if (!std::isfinite(value) || value < minimum_bound || value > maximum_bound) {
			elements_valid = false;
			observed_min = value;
			observed_max = value;
			break;
		}
		observed_min = std::min(observed_min, value);
		observed_max = std::max(observed_max, value);
	}

	if (!elements_valid) {
		std::cerr << COLOR_RED << "  [FAIL] " << check_label
				  << " contains invalid or out-of-bounds value. Sample: " << observed_min
				  << " outside [" << minimum_bound << ", " << maximum_bound << "]"
				  << COLOR_RESET << "\n";
		return false;
	}

	std::cout << COLOR_GREEN << "  [PASS] " << check_label << " (all " << tensor.size()
			  << " elements in range [" << observed_min << ", " << observed_max << "])"
			  << COLOR_RESET << "\n";
	return true;
}

// ─── Kernel Simulator for Domain A ───────────────────────────────────────────
__global__ static void simulate_step_loss_kernel(float* loss_buffer, float step_loss) {
	if (threadIdx.x == 0 && blockIdx.x == 0) {
		atomicAdd(&loss_buffer[telemetry_loss], step_loss);
	}
}

// ─── Test 1: Two-Domain Differential Loss Verification ───────────────────────
static bool test_differential_loss_domains() {
	std::cout << "[Test] Two-Domain Differential Loss Verification\n";

	const size_t slot_bytes = telemetry_slot_count * sizeof(float);
	const float expected_step_loss = 2.302585f; // Analytical -log(0.1)

	GpuActivations activations = {};
	cudaMalloc(&activations.d_loss, slot_bytes);
	cudaMemset(activations.d_loss, 0, slot_bytes);

	// ── 1. Failure Domain A: Kernel & Math Execution ──
	simulate_step_loss_kernel<<<1, 1>>>(static_cast<float*>(activations.d_loss), expected_step_loss);
	cudaDeviceSynchronize();

	// Direct raw device inspection (isolates math from host pipeline)
	float raw_device_loss = 0.0f;
	cudaMemcpy(&raw_device_loss, activations.d_loss, sizeof(float), cudaMemcpyDeviceToHost);

	bool domain_a_valid = std::isfinite(raw_device_loss) && raw_device_loss >= 0.0f;
	if (!domain_a_valid) {
		std::cerr << COLOR_RED
				  << "  [FAIL] Domain A (Compute Kernel): Math produced NaN, Inf, or negative value: "
				  << raw_device_loss << COLOR_RESET << "\n";
		free_gpu_activations(&activations);
		return false;
	}
	std::cout << COLOR_GREEN << "  [PASS] Domain A (Compute Kernel): Raw loss valid ("
			  << raw_device_loss << ")" << COLOR_RESET << "\n";

	// ── 2. Failure Domain B: State Plumbing & Routing ──
	float host_slots[telemetry_slot_count] = {};
	bool download_success = download_telemetry(&activations, host_slots);

	if (!download_success) {
		std::cerr << COLOR_RED << "  [FAIL] Domain B (Pipeline Plumbing): download_telemetry failed"
				  << COLOR_RESET << "\n";
		free_gpu_activations(&activations);
		return false;
	}

	float current_loss = host_slots[telemetry_loss];
	bool domain_b_valid = std::isfinite(current_loss) && current_loss >= 0.0f;
	if (!domain_b_valid) {
		std::cerr << COLOR_RED
				  << "  [FAIL] Domain B (Pipeline Plumbing): Memory corruption or invalid slot value: "
				  << current_loss << COLOR_RESET << "\n";
		free_gpu_activations(&activations);
		return false;
	}
	std::cout << COLOR_GREEN << "  [PASS] Domain B (Pipeline Plumbing): Slot propagated successfully ("
			  << current_loss << ")" << COLOR_RESET << "\n";

	// ── 3. Differential Identity Check: Domain A vs Domain B ──
	constexpr float float_epsilon = 1e-6f;
	float absolute_difference = std::abs(current_loss - raw_device_loss);
	bool identity_valid = absolute_difference < float_epsilon;

	if (!identity_valid) {
		std::cerr << COLOR_RED
				  << "  [FAIL] Routing Logic: current_loss diverged from raw_loss (diff = "
				  << absolute_difference << ")" << COLOR_RESET << "\n";
		free_gpu_activations(&activations);
		return false;
	}
	std::cout << COLOR_GREEN
			  << "  [PASS] Differential Check: Identity holds (|loss[0] - current_loss| < 1e-6)"
			  << COLOR_RESET << "\n";

	// ── 4. Pipeline Invariant Check: d_loss reset in VRAM ──
	float reset_check_value = -1.0f;
	cudaMemcpy(&reset_check_value, activations.d_loss, sizeof(float), cudaMemcpyDeviceToHost);
	bool reset_valid = (reset_check_value == 0.0f);

	if (!reset_valid) {
		std::cerr << COLOR_RED
				  << "  [FAIL] Pipeline Invariant: d_loss was not reset to 0 in VRAM after download"
				  << COLOR_RESET << "\n";
		free_gpu_activations(&activations);
		return false;
	}
	std::cout << COLOR_GREEN << "  [PASS] Pipeline Invariant: d_loss successfully reset to 0 in VRAM"
			  << COLOR_RESET << "\n";

	free_gpu_activations(&activations);
	return domain_a_valid && domain_b_valid && identity_valid && reset_valid;
}

// ─── Test 2: SwiGLU Backward Gradient Range Verification ─────────────────────
static bool test_swiglu_gradient_bounds() {
	std::cout << "[Test] SwiGLU Backward Gradient Range\n";

	const size_t total_elements = 256;
	const size_t byte_count_bf16 = total_elements * sizeof(uint16_t);
	const size_t byte_count_float = total_elements * sizeof(float);

	// 1. Host Data Initialization
	std::vector<uint16_t> host_grad(total_elements, 0x3F80); // 1.0f in bf16
	std::vector<uint16_t> host_gate(total_elements, 0x3F00); // 0.5f in bf16
	std::vector<uint16_t> host_up(total_elements, 0x3F00);	 // 0.5f in bf16

	// 2. Device Memory Allocation
	void* device_grad = nullptr;
	void* device_gate = nullptr;
	void* device_up = nullptr;
	void* device_grad_gate = nullptr;
	void* device_grad_up = nullptr;

	cudaMalloc(&device_grad, byte_count_bf16);
	cudaMalloc(&device_gate, byte_count_bf16);
	cudaMalloc(&device_up, byte_count_bf16);
	cudaMalloc(&device_grad_gate, byte_count_float);
	cudaMalloc(&device_grad_up, byte_count_float);

	// 3. Upload Host Data to Device
	cudaMemcpy(device_grad, host_grad.data(), byte_count_bf16, cudaMemcpyHostToDevice);
	cudaMemcpy(device_gate, host_gate.data(), byte_count_bf16, cudaMemcpyHostToDevice);
	cudaMemcpy(device_up, host_up.data(), byte_count_bf16, cudaMemcpyHostToDevice);

	// 4. Execute CUDA Kernel / Function
	bool execution_success = swiglu_backward(device_grad, device_gate, device_up, device_grad_gate,
											 device_grad_up, total_elements, nullptr);

	cudaDeviceSynchronize();

	// 5. Download Device Results to Host
	std::vector<float> host_result_gate(total_elements, 0.0f);
	std::vector<float> host_result_up(total_elements, 0.0f);
	cudaMemcpy(host_result_gate.data(), device_grad_gate, byte_count_float, cudaMemcpyDeviceToHost);
	cudaMemcpy(host_result_up.data(), device_grad_up, byte_count_float, cudaMemcpyDeviceToHost);

	// 6. Deterministic Cleanup of Device VRAM
	cudaFree(device_grad);
	cudaFree(device_gate);
	cudaFree(device_up);
	cudaFree(device_grad_gate);
	cudaFree(device_grad_up);

	if (!execution_success) {
		std::cerr << COLOR_RED << "  [FAIL] swiglu_backward returned false" << COLOR_RESET << "\n";
		return false;
	}

	// 7. Verify Tensor Ranges
	bool gate_valid = check_tensor_bounds(host_result_gate, -10.0f, 10.0f, "Gate Gradient Tensor");
	bool up_valid = check_tensor_bounds(host_result_up, -10.0f, 10.0f, "Up Gradient Tensor");

	return gate_valid && up_valid;
}

// ─── Test 3: Multi-Step Telemetry Accumulation & Reduction ───────────────────
static bool test_telemetry_loss_bounds() {
	std::cout << "[Test] Multi-Step Telemetry Accumulation & Reduction\n";

	const size_t slot_bytes = telemetry_slot_count * sizeof(float);
	const size_t step_count = 4;
	const float per_step_loss = 2.302585f;
	const float expected_accumulated_sum = per_step_loss * static_cast<float>(step_count);

	GpuActivations activations = {};
	cudaMalloc(&activations.d_loss, slot_bytes);
	cudaMemset(activations.d_loss, 0, slot_bytes);

	// Simulate step_count training iterations accumulating into slot 0
	for (size_t step = 0; step < step_count; ++step) {
		simulate_step_loss_kernel<<<1, 1>>>(static_cast<float*>(activations.d_loss), per_step_loss);
	}
	cudaDeviceSynchronize();

	// Download accumulated telemetry through the official pipeline
	float host_telemetry[telemetry_slot_count] = {};
	bool download_success = download_telemetry(&activations, host_telemetry);

	if (!download_success) {
		std::cerr << COLOR_RED << "  [FAIL] download_telemetry returned false" << COLOR_RESET << "\n";
		free_gpu_activations(&activations);
		return false;
	}

	// 1. Verify Accumulated Sum Ingestion
	float downloaded_sum = host_telemetry[telemetry_loss];
	constexpr float epsilon = 1e-5f;
	bool sum_matches = std::abs(downloaded_sum - expected_accumulated_sum) < epsilon;

	if (!sum_matches) {
		std::cerr << COLOR_RED << "  [FAIL] Accumulator mismatch: got " << downloaded_sum
				  << ", expected " << expected_accumulated_sum << COLOR_RESET << "\n";
		free_gpu_activations(&activations);
		return false;
	}
	std::cout << COLOR_GREEN << "  [PASS] Accumulated Loss Sum (" << step_count << " steps): "
			  << downloaded_sum << COLOR_RESET << "\n";

	// 2. Verify Host-Side Mean Loss Reduction (Exact same calculation as training.cpp:318)
	float reduced_mean_loss = downloaded_sum / static_cast<float>(step_count);
	bool mean_matches = std::abs(reduced_mean_loss - per_step_loss) < epsilon;

	if (!mean_matches) {
		std::cerr << COLOR_RED << "  [FAIL] Mean loss reduction mismatch: got " << reduced_mean_loss
				  << ", expected " << per_step_loss << COLOR_RESET << "\n";
		free_gpu_activations(&activations);
		return false;
	}
	std::cout << COLOR_GREEN << "  [PASS] Mean Loss Reduction: " << reduced_mean_loss
			  << " matches per-step target (" << per_step_loss << ")" << COLOR_RESET << "\n";

	// 3. Verify Health Invariants (No NaNs/Infs)
	bool anomaly_free = check_scalar_bounds(host_telemetry[telemetry_nonfinite], 0.0f, 0.0f,
											"Non-finite Count");

	free_gpu_activations(&activations);
	return sum_matches && mean_matches && anomaly_free;
}

int main() {
	std::cout << "========================================\n";
	std::cout << "Running CUDA Kernel Loss & Gradient Tests\n";
	std::cout << "========================================\n";

	int passed_tests = 0;
	int failed_tests = 0;

	if (test_differential_loss_domains()) {
		++passed_tests;
	} else {
		++failed_tests;
	}

	if (test_swiglu_gradient_bounds()) {
		++passed_tests;
	} else {
		++failed_tests;
	}

	if (test_telemetry_loss_bounds()) {
		++passed_tests;
	} else {
		++failed_tests;
	}

	std::cout << "----------------------------------------\n";
	std::cout << "Summary: " << COLOR_GREEN << passed_tests << " Passed" << COLOR_RESET << ", "
			  << (failed_tests > 0 ? COLOR_RED : COLOR_GREEN) << failed_tests << " Failed"
			  << COLOR_RESET << "\n";

	return (failed_tests == 0) ? 0 : 1;
}
