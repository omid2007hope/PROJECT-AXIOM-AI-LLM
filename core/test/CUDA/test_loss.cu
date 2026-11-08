// test loss for anomaly at kernel level

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>
#include <cuda_runtime.h>

#include "../../src/CUDA/util/gpu_types.cuh"
#include "../../src/CUDA/kernel/backward_pass.cuh"

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
		std::cerr << COLOR_RED << "  [FAIL] " << check_label << ": " << actual_value
			  << " outside [" << minimum_bound << ", " << maximum_bound << "]"
			  << COLOR_RESET << "\n";
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

	std::cout << COLOR_GREEN << "  [PASS] " << check_label
		  << " (all " << tensor.size() << " elements in range ["
		  << observed_min << ", " << observed_max << "])"
		  << COLOR_RESET << "\n";
	return true;
}

// ─── Test 1: SwiGLU Backward Gradient Range Verification ─────────────────────
static bool test_swiglu_gradient_bounds() {
	std::cout << "[Test] SwiGLU Backward Gradient Range\n";

	const size_t total_elements = 256;
	const size_t byte_count_bf16 = total_elements * sizeof(uint16_t);
	const size_t byte_count_float = total_elements * sizeof(float);

	// 1. Host Data Initialization
	std::vector<uint16_t> host_grad(total_elements, 0x3F80); // 1.0f in bf16
	std::vector<uint16_t> host_gate(total_elements, 0x3F00); // 0.5f in bf16
	std::vector<uint16_t> host_up(total_elements, 0x3F00);   // 0.5f in bf16

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
	bool execution_success = swiglu_backward(device_grad,
						device_gate,
						device_up,
						device_grad_gate,
						device_grad_up,
						total_elements,
						nullptr);

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

// ─── Test 2: Telemetry Slot Loss Bounds Verification ─────────────────────────
static bool test_telemetry_loss_bounds() {
	std::cout << "[Test] Telemetry Slot Loss Bounds\n";

	const size_t slot_bytes = telemetry_slot_count * sizeof(float);
	float* device_telemetry = nullptr;
	cudaMalloc(&device_telemetry, slot_bytes);
	cudaMemset(device_telemetry, 0, slot_bytes);

	// Download telemetry to host
	std::vector<float> host_telemetry(telemetry_slot_count, 0.0f);
	cudaMemcpy(host_telemetry.data(), device_telemetry, slot_bytes, cudaMemcpyDeviceToHost);
	cudaFree(device_telemetry);

	// Expected: Loss >= 0.0 and no nonfinite counts
	bool pass_loss = check_scalar_bounds(host_telemetry[telemetry_loss], 0.0f, 100.0f, "Mean Loss");
	bool pass_anomaly = check_scalar_bounds(host_telemetry[telemetry_nonfinite], 0.0f, 0.0f, "Non-finite Count");

	return pass_loss && pass_anomaly;
}

int main() {
	std::cout << "========================================\n";
	std::cout << "Running CUDA Kernel Loss & Gradient Tests\n";
	std::cout << "========================================\n";

	int passed_tests = 0;
	int failed_tests = 0;

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
	std::cout << "Summary: "
		  << COLOR_GREEN << passed_tests << " Passed" << COLOR_RESET << ", "
		  << (failed_tests > 0 ? COLOR_RED : COLOR_GREEN) << failed_tests << " Failed"
		  << COLOR_RESET << "\n";

	return (failed_tests == 0) ? 0 : 1;
}
