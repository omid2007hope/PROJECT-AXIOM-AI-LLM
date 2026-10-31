#include <chrono>
#include <cuda_bf16.h>
#include <cuda_runtime.h>

#include "../util/caching_allocator.cuh"
#include "../util/cuda_utils.cuh"
#include "backward_pass.cuh"
#include "forward_pass.cuh"

// The CUTLASS visitor headers are not self-contained: gemm_universal.h must come
// first, so this order must not be re-sorted.
// clang-format off
#include "cutlass/cutlass.h"
#include "cutlass/gemm/device/gemm_universal.h"
#include "cutlass/epilogue/thread/activation.h"
#include "cutlass/epilogue/threadblock/fusion/visitors.hpp"
#include "cutlass/gemm/kernel/default_gemm_universal_with_visitor.h"
#include "cutlass/gemm/device/gemm_universal_adapter.h"
// clang-format on

// =============================================================================
// Forward pass:  y = W x + b  as ONE fused CUTLASS kernel on the Tensor Cores
//
// The output layer is deliberately linear: the regression targets are signed
// (sin() in [-1, 1]), and a ReLU can neither output a negative value nor pass a
// gradient once it is at 0, so it permanently kills every neuron it clamps.
//
// The layer is a GEMM   Y[M x N] = X[M x K] * W^T[K x N]
//     M = 1         (one context window per step)
//     N = out_dim   (one output per neuron)
//     K = in_dim    (window elements)
//
// The multiply runs on the Tensor Cores (bf16 inputs, FP32 accumulation). The
// "epilogue" that normally needs separate kernels, each one writing to VRAM and
// reading it back, is an Epilogue Visitor Tree (EVT) executed on the accumulators
// while they are still in registers:
//
//              Store(bf16)
//                  |
//          Add( Accum , Bias )
//           /
//   VisitorAccFetch    VisitorRowBroadcast
// =============================================================================
namespace {

using ElementIn = cutlass::bfloat16_t; // layout-compatible with __nv_bfloat16
using ElementAccumulator = float;
using ElementCompute = float;

using LayoutX = cutlass::layout::RowMajor;    // x: K contiguous
using LayoutW = cutlass::layout::ColumnMajor; // W is stored [out][in] == W^T column-major
using LayoutY = cutlass::layout::RowMajor;

constexpr int AlignmentIn = 8;	// 128-bit vector loads of bf16 (in_dim must be a multiple of 8)
constexpr int AlignmentOut = 1; // out_dim is tiny, so it is not forced to a multiple of 8

using ThreadblockShape = cutlass::gemm::GemmShape<64, 64, 32>;
using WarpShape = cutlass::gemm::GemmShape<32, 32, 32>;
using InstructionShape = cutlass::gemm::GemmShape<16, 8, 16>; // bf16 mma.sync m16n8k16
constexpr int NumStages = 4;
constexpr int EpilogueStages = 1;

using OutputTileThreadMap = cutlass::epilogue::threadblock::OutputTileThreadLayout<ThreadblockShape,
										   WarpShape,
										   ElementIn,
										   AlignmentOut,
										   EpilogueStages>;

using Accum = cutlass::epilogue::threadblock::VisitorAccFetch;

// One bias value per output column, repeated for every row (stride 0 along M)
using Bias = cutlass::epilogue::threadblock::VisitorRowBroadcast<
	OutputTileThreadMap,
	ElementIn,
	cute::Stride<cute::_0, cute::_1, int32_t>>;

using AddBias =
	cutlass::epilogue::threadblock::VisitorCompute<cutlass::plus,
						       ElementCompute,
						       ElementCompute,
						       cutlass::FloatRoundStyle::round_to_nearest>;

using EvtAddBias = cutlass::epilogue::threadblock::Sm80EVT<AddBias, Accum, Bias>;

using Store =
	cutlass::epilogue::threadblock::VisitorAuxStore<OutputTileThreadMap,
							ElementIn,
							cutlass::FloatRoundStyle::round_to_nearest,
							cute::Stride<int64_t, cute::_1, int64_t>>;

using EvtStore = cutlass::epilogue::threadblock::Sm80EVT<Store, EvtAddBias>;

using GemmKernel = typename cutlass::gemm::kernel::DefaultGemmWithVisitor<
	ElementIn,
	LayoutX,
	cutlass::ComplexTransform::kNone,
	AlignmentIn,
	ElementIn,
	LayoutW,
	cutlass::ComplexTransform::kNone,
	AlignmentIn,
	ElementIn,
	LayoutY,
	AlignmentOut,
	ElementAccumulator,
	ElementCompute,
	cutlass::arch::OpClassTensorOp,
	cutlass::arch::Sm80, // Ada (sm_89) runs the Ampere Tensor Core kernels
	ThreadblockShape,
	WarpShape,
	InstructionShape,
	EvtStore,
	cutlass::gemm::threadblock::GemmIdentityThreadblockSwizzle<>,
	NumStages,
	cutlass::arch::OpMultiplyAdd,
	EpilogueStages>::GemmKernel;

using Gemm = cutlass::gemm::device::GemmUniversalAdapter<GemmKernel>;

} // namespace

__global__ static void gather_embeddings_kernel(const uint32_t* __restrict__ token_ids,
						const __nv_bfloat16* __restrict__ table,
						__nv_bfloat16* __restrict__ context_window,
						size_t num_tokens,
						size_t embedding_dim,
						size_t vocab_size) {
	size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
	size_t total_elements = num_tokens * embedding_dim;
	if (idx < total_elements) {
		size_t token_idx = idx / embedding_dim;
		size_t dim_idx = idx % embedding_dim;
		uint32_t tok_id = token_ids[token_idx];
		if (tok_id == 0) {
			// PAD must contribute exactly 0.0 to X * W
			context_window[idx] = __float2bfloat16(0.0f);
			return;
		}
		if (tok_id >= vocab_size) {
			tok_id = 1; // UNK fallback
		}
		context_window[idx] = table[tok_id * embedding_dim + dim_idx];
	}
}

// Milestone 3: RMSNorm Forward Kernel (in-place normalization before Tensor Cores)
__global__ static void rms_norm_forward_kernel(__nv_bfloat16* __restrict__ x,
					       float* __restrict__ inv_rms_out,
					       size_t n,
					       float eps) {
	__shared__ float s_sum[32];
	float sum_sq = 0.0f;

	for (size_t i = threadIdx.x; i < n; i += blockDim.x) {
		float val = __bfloat162float(x[i]);
		if (isfinite(val)) {
			sum_sq += val * val;
		}
	}

	for (int offset = 16; offset > 0; offset /= 2) {
		sum_sq += __shfl_down_sync(0xffffffff, sum_sq, offset);
	}

	int lane = threadIdx.x % 32;
	int wid = threadIdx.x / 32;
	if (lane == 0) {
		s_sum[wid] = sum_sq;
	}
	__syncthreads();

	if (wid == 0) {
		float warp_sum = (lane < (blockDim.x / 32)) ? s_sum[lane] : 0.0f;
		for (int offset = 16; offset > 0; offset /= 2) {
			warp_sum += __shfl_down_sync(0xffffffff, warp_sum, offset);
		}
		if (lane == 0) {
			float mean_sq = warp_sum / static_cast<float>(n > 0 ? n : 1);
			float inv_rms = rsqrtf(mean_sq + eps);
			if (!isfinite(inv_rms) || isinf(inv_rms)) {
				inv_rms = 1.0f;
			}
			s_sum[0] = inv_rms;
			if (inv_rms_out) {
				*inv_rms_out = inv_rms;
			}
		}
	}
	__syncthreads();

	float inv_rms = s_sum[0];
	for (size_t i = threadIdx.x; i < n; i += blockDim.x) {
		float val = __bfloat162float(x[i]);
		float norm_val = val * inv_rms;
		norm_val = fmaxf(-10.0f, fminf(10.0f, norm_val));
		x[i] = __float2bfloat16(norm_val);
	}
}

// Milestone 3: Tied Logit Projection Kernel (Y * E^T)
__global__ static void compute_tied_logits_kernel(const __nv_bfloat16* __restrict__ y,
						  const __nv_bfloat16* __restrict__ table,
						  float* __restrict__ logits,
						  size_t vocab_size,
						  size_t emb_dim,
						  float inv_temp) {

	size_t v = blockIdx.x * (size_t)blockDim.x + threadIdx.x;

	if (v < vocab_size) {

		float dot = 0.0f;

		const __nv_bfloat16* emb_row = table + v * emb_dim;

		for (size_t d = 0; d < emb_dim; ++d) {
			dot += __bfloat162float(y[d]) * __bfloat162float(emb_row[d]);
		}

		logits[v] = (v == 0) ? -1e30f : (dot * inv_temp); // PAD is never a prediction
	}
}

__global__ static void copy_bf16_to_float_kernel(const __nv_bfloat16* __restrict__ src,
						 float* __restrict__ dst,
						 size_t n) {
	size_t i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n) {
		dst[i] = __bfloat162float(src[i]);
	}
}

// Milestone 3: Fused Softmax & Argmax Kernel on GPU
__global__ static void softmax_argmax_kernel(float* __restrict__ logits_and_probs,
					     uint32_t* __restrict__ pred_id,
					     size_t n) {
	__shared__ float s_max[32];
	__shared__ uint32_t s_arg[32];
	__shared__ float s_sum[32];

	float local_max = -1e30f;
	uint32_t local_arg = 0;

	for (size_t i = threadIdx.x; i < n; i += blockDim.x) {
		float val = logits_and_probs[i];
		if (isfinite(val) && val > local_max) {
			local_max = val;
			local_arg = static_cast<uint32_t>(i);
		}
	}

	for (int offset = 16; offset > 0; offset /= 2) {
		float other_max = __shfl_down_sync(0xffffffff, local_max, offset);
		uint32_t other_arg = __shfl_down_sync(0xffffffff, local_arg, offset);
		if (other_max > local_max) {
			local_max = other_max;
			local_arg = other_arg;
		}
	}

	int lane = threadIdx.x % 32;
	int wid = threadIdx.x / 32;
	if (lane == 0) {
		s_max[wid] = local_max;
		s_arg[wid] = local_arg;
	}
	__syncthreads();

	if (wid == 0) {
		float warp_max = (lane < (blockDim.x / 32)) ? s_max[lane] : -1e30f;
		uint32_t warp_arg = (lane < (blockDim.x / 32)) ? s_arg[lane] : 0;
		for (int offset = 16; offset > 0; offset /= 2) {
			float other_max = __shfl_down_sync(0xffffffff, warp_max, offset);
			uint32_t other_arg = __shfl_down_sync(0xffffffff, warp_arg, offset);
			if (other_max > warp_max) {
				warp_max = other_max;
				warp_arg = other_arg;
			}
		}
		if (lane == 0) {
			s_max[0] = warp_max;
			s_arg[0] = warp_arg;
			if (pred_id) {
				*pred_id = warp_arg;
			}
		}
	}
	__syncthreads();

	float global_max = s_max[0];

	float sum_exp = 0.0f;
	for (size_t i = threadIdx.x; i < n; i += blockDim.x) {
		float exp_val = expf(fmaxf(-50.0f, fminf(50.0f, logits_and_probs[i] - global_max)));
		logits_and_probs[i] = exp_val;
		sum_exp += exp_val;
	}

	for (int offset = 16; offset > 0; offset /= 2) {
		sum_exp += __shfl_down_sync(0xffffffff, sum_exp, offset);
	}
	if (lane == 0) {
		s_sum[wid] = sum_exp;
	}
	__syncthreads();

	if (wid == 0) {
		float warp_sum = (lane < (blockDim.x / 32)) ? s_sum[lane] : 0.0f;
		for (int offset = 16; offset > 0; offset /= 2) {
			warp_sum += __shfl_down_sync(0xffffffff, warp_sum, offset);
		}
		if (lane == 0) {
			s_sum[0] = warp_sum > 1e-12f ? warp_sum : 1.0f;
		}
	}
	__syncthreads();

	float inv_sum = 1.0f / s_sum[0];
	for (size_t i = threadIdx.x; i < n; i += blockDim.x) {
		logits_and_probs[i] *= inv_sum;
	}
}

extern "C" CUDA_API bool forward_pass_with_embeddings(const GpuPortion* gpu_portion,
						      const GpuEmbeddings* gpu_embeddings,
						      const GpuWeights* gpu_weights,
						      GpuActivations* gpu_activations,
						      size_t input_dim,
						      size_t output_dim,
						      int buffer_index) {

	// Safe Guard
	if (!gpu_portion || !gpu_weights || !gpu_activations || !gpu_weights->d_weights ||
	    !gpu_weights->d_bias || input_dim == 0 || output_dim == 0) {
		return false;
	}

	cudaStream_t stream = (cudaStream_t)gpu_portion->stream[buffer_index];

	// The two buffers share the weights and the output, so this step may only start
	// after the previous step (other buffer) finished updating them. Its upload is
	// already queued on this stream and still overlaps with that step.
	cudaStreamCaptureStatus capture_status = cudaStreamCaptureStatusNone;
	if (stream) {
		cudaError_t cap_err = cudaStreamIsCapturing(stream, &capture_status);
		if (cap_err != cudaSuccess) {
			cudaGetLastError();
			capture_status = cudaStreamCaptureStatusNone;
		}
	}
	if (capture_status == cudaStreamCaptureStatusNone) {
		cudaEvent_t previous_step_done = (cudaEvent_t)gpu_portion->compute_done[1 - buffer_index];
		if (previous_step_done && gpu_portion->compute_done_recorded[1 - buffer_index]) {
			if (!CUDA_CHECK(cudaStreamWaitEvent(stream, previous_step_done, 0)))
				return false;
		}
	}

	// Ensure d_context_window is allocated on GPU for the gather scratchpad or GEMM input
	GpuPortion* non_const_gpu = const_cast<GpuPortion*>(gpu_portion);
	for (size_t i = 0; i < non_const_gpu->context_ptr; ++i) {
		if (non_const_gpu->context_elements != input_dim ||
		    !non_const_gpu->d_context_window[i]) {
			if (non_const_gpu->d_context_window[i]) {
				free_vram(non_const_gpu->d_context_window[i], non_const_gpu->stream[i]);
				non_const_gpu->d_context_window[i] = nullptr;
			}
			non_const_gpu->d_context_window[i] =
				allocate_vram(input_dim * sizeof(__nv_bfloat16), non_const_gpu->stream[i]);
			if (!non_const_gpu->d_context_window[i])
				return false;
		}
	}
	non_const_gpu->context_elements = input_dim;

	// Phase 4: Forward Gather Kernel & PCIe Bandwidth Optimization
	// If token IDs and embedding table are provided, branchlessly gather tokens into d_context_window on the GPU
	if (gpu_portion->token_ids_active[buffer_index] && gpu_embeddings &&
	    gpu_embeddings->d_table) {
		size_t emb_dim =
			gpu_embeddings->embedding_dim > 0 ? gpu_embeddings->embedding_dim : 32;
		size_t num_tok = gpu_portion->token_count > 0 ? gpu_portion->token_count
							      : (input_dim / emb_dim);
		int threads = 256;
		int blocks = static_cast<int>((input_dim + threads - 1) / threads);

		cudaGetLastError(); // Clear any sticky driver error
		gather_embeddings_kernel<<<blocks, threads, 0, stream>>>(
			static_cast<const uint32_t*>(gpu_portion->d_token_ids[buffer_index]),
			static_cast<const __nv_bfloat16*>(gpu_embeddings->d_table),
			static_cast<__nv_bfloat16*>(gpu_portion->d_context_window[buffer_index]),
			num_tok,
			emb_dim,
			gpu_embeddings->vocab_size);
		if (!CUDA_CHECK(cudaGetLastError()))
			return false;
	}

	if (!gpu_portion->d_context_window[buffer_index]) {
		return false;
	}

	// Milestone 3: RMSNorm Forward Pass (Normalizes variance of flattened input to 1.0)
	// Applied only when embeddings are active to protect large context windows from variance explosion
	if (gpu_embeddings && gpu_embeddings->d_table) {
		if (!gpu_activations->d_inv_rms) {
			gpu_activations->d_inv_rms = allocate_vram(sizeof(float), stream);
			if (!gpu_activations->d_inv_rms)
				return false;
		}
		int norm_threads = 1024;
		rms_norm_forward_kernel<<<1, norm_threads, 0, stream>>>(
			static_cast<__nv_bfloat16*>(gpu_portion->d_context_window[buffer_index]),
			static_cast<float*>(gpu_activations->d_inv_rms),
			input_dim,
			1e-5f);
		if (!CUDA_CHECK(cudaGetLastError()))
			return false;
	} else if (gpu_activations->d_inv_rms) {
		float unity = 1.0f;
		cudaMemcpyAsync(gpu_activations->d_inv_rms,
				&unity,
				sizeof(float),
				cudaMemcpyHostToDevice,
				stream);
	}

	if (gpu_activations->output_elements != output_dim || !gpu_activations->d_output) {
		if (gpu_activations->d_output) {
			free_vram(gpu_activations->d_output, stream);
			gpu_activations->d_output = nullptr;
		}

		gpu_activations->d_output =
			allocate_vram(output_dim * sizeof(__nv_bfloat16), stream);
		if (!gpu_activations->d_output)
			return false;
		gpu_activations->output_elements = output_dim;
	}

	// The loss accumulator is written by the backward pass; it starts at zero
	if (!gpu_activations->d_loss) {
		gpu_activations->d_loss = allocate_vram(sizeof(float), stream);
		if (!gpu_activations->d_loss)
			return false;
		if (!CUDA_CHECK(cudaMemsetAsync(gpu_activations->d_loss, 0, sizeof(float), stream)))
			return false;
	}

	using cute::_0;
	using cute::_1;

	const int M = 1;
	const int N = static_cast<int>(output_dim);
	const int K = static_cast<int>(input_dim);

	// Visitor arguments are listed children first, then the node itself
	typename EvtStore::Arguments callback_args {
		{
			{}, // Accum
			{static_cast<const ElementIn*>(gpu_weights->d_bias),
			 ElementIn(0),
			 {_0 {}, _1 {}, int32_t(N)}}, // Bias
			{}			      // AddBias
		},				      // EvtAddBias
		{static_cast<ElementIn*>(gpu_activations->d_output),
		 {int64_t(N), _1 {}, int64_t(M) * N}} // Store
	};

	typename Gemm::Arguments arguments(cutlass::gemm::GemmUniversalMode::kGemm,
					   cutlass::gemm::GemmCoord(M, N, K),
					   1, // batch_count
					   callback_args,
					   gpu_portion->d_context_window[buffer_index], // ptr_A: x
					   gpu_weights->d_weights,			// ptr_B: W
					   nullptr,	// ptr_C (the EVT does the store)
					   nullptr,	// ptr_D
					   int64_t(0),	// batch_stride_A
					   int64_t(0),	// batch_stride_B
					   int64_t(0),	// batch_stride_C
					   int64_t(0),	// batch_stride_D
					   int64_t(K),	// lda
					   int64_t(K),	// ldb
					   int64_t(0),	// ldc
					   int64_t(0)); // ldd

	Gemm gemm_op;

	cutlass::Status status = gemm_op.can_implement(arguments);
	if (status != cutlass::Status::kSuccess) {
		fprintf(stderr,
			"[CUTLASS] forward_pass unsupported (in_dim=%zu must be a multiple of %d): %s\n",
			input_dim,
			AlignmentIn,
			cutlass::cutlassGetStatusString(status));
		return false;
	}

	// A single-slice GEMM needs no workspace
	if (Gemm::get_workspace_size(arguments) != 0) {
		fprintf(stderr, "[CUTLASS] forward_pass unexpectedly needs a workspace\n");
		return false;
	}

	status = gemm_op(arguments, nullptr, stream);
	if (status != cutlass::Status::kSuccess) {
		fprintf(stderr,
			"[CUTLASS] forward_pass failed: %s\n",
			cutlass::cutlassGetStatusString(status));
		return false;
	}

	// Milestone 3: Discrete Autoregressive Token Classification
	// Compute Softmax probabilities and Argmax winning token on the GPU
	size_t vocab_sz =
		(gpu_embeddings && gpu_embeddings->d_table)
			? (gpu_embeddings->active_vocab_size > 0 ? gpu_embeddings->active_vocab_size
								 : gpu_embeddings->vocab_size)
			: output_dim;
	if (gpu_activations->vocab_elements != vocab_sz || !gpu_activations->d_probs) {
		if (gpu_activations->d_probs) {
			free_vram(gpu_activations->d_probs, stream);
			gpu_activations->d_probs = nullptr;
		}
		gpu_activations->d_probs = allocate_vram(vocab_sz * sizeof(float), stream);
		if (!gpu_activations->d_probs)
			return false;
		gpu_activations->vocab_elements = vocab_sz;
	}
	if (!gpu_activations->d_pred_id) {
		gpu_activations->d_pred_id = allocate_vram(sizeof(uint32_t), stream);
		if (!gpu_activations->d_pred_id)
			return false;
	}

	if (gpu_embeddings && gpu_embeddings->d_table) {
		size_t emb_dim = gpu_embeddings->embedding_dim > 0 ? gpu_embeddings->embedding_dim
								   : output_dim;

		float inv_temp = (gpu_embeddings->temperature > 1e-4f)
					 ? (1.0f / gpu_embeddings->temperature)
					 : 1.0f;

		int l_threads = 256;
		int l_blocks = static_cast<int>((vocab_sz + l_threads - 1) / l_threads);
		compute_tied_logits_kernel<<<l_blocks, l_threads, 0, stream>>>(
			static_cast<const __nv_bfloat16*>(gpu_activations->d_output),
			static_cast<const __nv_bfloat16*>(gpu_embeddings->d_table),
			static_cast<float*>(gpu_activations->d_probs),
			vocab_sz,
			emb_dim,
			inv_temp);
	} else {
		int d_threads = 256;
		int d_blocks = static_cast<int>((output_dim + d_threads - 1) / d_threads);
		copy_bf16_to_float_kernel<<<d_blocks, d_threads, 0, stream>>>(
			static_cast<const __nv_bfloat16*>(gpu_activations->d_output),
			static_cast<float*>(gpu_activations->d_probs),
			output_dim);
	}

	softmax_argmax_kernel<<<1, 1024, 0, stream>>>(
		static_cast<float*>(gpu_activations->d_probs),
		static_cast<uint32_t*>(gpu_activations->d_pred_id),
		vocab_sz);

	return CUDA_CHECK(cudaGetLastError());
}

extern "C" CUDA_API bool forward_pass(const GpuPortion* gpu_portion,
				      const GpuWeights* gpu_weights,
				      GpuActivations* gpu_activations,
				      size_t input_dim,
				      size_t output_dim,
				      int buffer_index) {
	return forward_pass_with_embeddings(gpu_portion,
					    nullptr,
					    gpu_weights,
					    gpu_activations,
					    input_dim,
					    output_dim,
					    buffer_index);
}

extern "C" CUDA_API bool run_cutlass_saturation_benchmark(int M,
							  int N,
							  int K,
							  int iterations,
							  double* out_tflops,
							  double* out_bandwidth_gb_s,
							  float* out_avg_ms) {
	if (M <= 0 || N <= 0 || K <= 0 || iterations <= 0) {
		return false;
	}

	if (K % AlignmentIn != 0) {
		return false;
	}

	using cute::_0;
	using cute::_1;

	size_t size_A = static_cast<size_t>(M) * K * sizeof(__nv_bfloat16);
	size_t size_B = static_cast<size_t>(K) * N * sizeof(__nv_bfloat16);
	size_t size_bias = static_cast<size_t>(N) * sizeof(__nv_bfloat16);
	size_t size_out = static_cast<size_t>(M) * N * sizeof(__nv_bfloat16);

	__nv_bfloat16* d_A = nullptr;
	__nv_bfloat16* d_B = nullptr;
	__nv_bfloat16* d_bias = nullptr;
	__nv_bfloat16* d_out = nullptr;

	if (cudaMalloc(&d_A, size_A) != cudaSuccess || cudaMalloc(&d_B, size_B) != cudaSuccess ||
	    cudaMalloc(&d_bias, size_bias) != cudaSuccess ||
	    cudaMalloc(&d_out, size_out) != cudaSuccess) {
		if (d_A)
			cudaFree(d_A);
		if (d_B)
			cudaFree(d_B);
		if (d_bias)
			cudaFree(d_bias);
		if (d_out)
			cudaFree(d_out);
		return false;
	}

	cudaMemset(d_A, 0x3c, size_A);
	cudaMemset(d_B, 0x3c, size_B);
	cudaMemset(d_bias, 0x00, size_bias);
	cudaMemset(d_out, 0x00, size_out);

	typename EvtStore::Arguments callback_args {
		{
			{}, // Accum
			{reinterpret_cast<const ElementIn*>(d_bias),
			 ElementIn(0),
			 {_0 {}, _1 {}, int32_t(N)}}, // Bias
			{}			      // AddBias
		},				      // EvtAddBias
		{reinterpret_cast<ElementIn*>(d_out), {int64_t(N), _1 {}, int64_t(M) * N}} // Store
	};

	typename Gemm::Arguments arguments(cutlass::gemm::GemmUniversalMode::kGemm,
					   cutlass::gemm::GemmCoord(M, N, K),
					   1, // batch_count
					   callback_args,
					   reinterpret_cast<ElementIn*>(d_A),
					   reinterpret_cast<ElementIn*>(d_B),
					   nullptr,
					   nullptr,
					   int64_t(0),
					   int64_t(0),
					   int64_t(0),
					   int64_t(0),
					   int64_t(K),
					   int64_t(K),
					   int64_t(0),
					   int64_t(0));

	Gemm gemm_op;

	cutlass::Status status = gemm_op.can_implement(arguments);
	if (status != cutlass::Status::kSuccess) {
		cudaFree(d_A);
		cudaFree(d_B);
		cudaFree(d_bias);
		cudaFree(d_out);
		return false;
	}

	size_t workspace_size = Gemm::get_workspace_size(arguments);
	void* d_workspace = nullptr;
	if (workspace_size > 0) {
		if (cudaMalloc(&d_workspace, workspace_size) != cudaSuccess) {
			cudaFree(d_A);
			cudaFree(d_B);
			cudaFree(d_bias);
			cudaFree(d_out);
			return false;
		}
	}

	cudaStream_t stream = nullptr;

	// Warm-up passes (locks Ada Lovelace Tensor Cores into P0 boost clock)
	for (int i = 0; i < 30; ++i) {
		gemm_op(arguments, d_workspace, stream);
	}
	cudaStreamSynchronize(stream);

	cudaEvent_t start = nullptr, stop = nullptr;
	cudaEventCreate(&start);
	cudaEventCreate(&stop);

	cudaEventRecord(start, stream);
	for (int i = 0; i < iterations; ++i) {
		gemm_op(arguments, d_workspace, stream);
	}
	cudaEventRecord(stop, stream);
	cudaEventSynchronize(stop);

	float total_ms = 0.0f;
	cudaEventElapsedTime(&total_ms, start, stop);

	cudaEventDestroy(start);
	cudaEventDestroy(stop);

	if (d_workspace)
		cudaFree(d_workspace);
	cudaFree(d_A);
	cudaFree(d_B);
	cudaFree(d_bias);
	cudaFree(d_out);

	if (total_ms <= 0.0f) {
		return false;
	}

	float avg_ms = total_ms / static_cast<float>(iterations);
	if (out_avg_ms)
		*out_avg_ms = avg_ms;

	double flops_per_iter =
		2.0 * static_cast<double>(M) * static_cast<double>(N) * static_cast<double>(K);
	double total_flops = flops_per_iter * static_cast<double>(iterations);
	double achieved_tflops = (total_flops / 1e12) / (total_ms / 1000.0);
	if (out_tflops)
		*out_tflops = achieved_tflops;

	double bytes_per_iter = static_cast<double>(size_A + size_B + size_bias + size_out);
	double total_bytes = bytes_per_iter * static_cast<double>(iterations);
	double achieved_bw_gb_s = (total_bytes / 1e9) / (total_ms / 1000.0);
	if (out_bandwidth_gb_s)
		*out_bandwidth_gb_s = achieved_bw_gb_s;

	return true;
}

__global__ void vram_saturation_kernel(const uint4* __restrict__ src,
				       uint4* __restrict__ dst,
				       size_t num_elements) {
	size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
	size_t stride = blockDim.x * gridDim.x;
	for (size_t i = idx; i < num_elements; i += stride) {
		dst[i] = src[i];
	}
}

extern "C" CUDA_API bool run_vram_saturation_benchmark(size_t safety_headroom_bytes,
						       size_t* out_allocated_bytes,
						       double* out_bandwidth_gb_s,
						       float* out_elapsed_ms) {
	size_t free_vram = 0, total_vram = 0;
	if (cudaMemGetInfo(&free_vram, &total_vram) != cudaSuccess) {
		return false;
	}

	if (free_vram <= safety_headroom_bytes) {
		return false;
	}

	size_t safe_budget = free_vram - safety_headroom_bytes;
	// Allocate 2 buffers: src and dst, 16-byte aligned
	size_t buf_size = (safe_budget / 2 / 16) * 16;
	constexpr size_t MAX_BUF_PER_SIDE = 3ULL * 1024 * 1024 * 1024; // 3 GB each = 6 GB total max
	if (buf_size > MAX_BUF_PER_SIDE) {
		buf_size = MAX_BUF_PER_SIDE;
	}

	uint4* d_src = nullptr;
	uint4* d_dst = nullptr;

	if (cudaMalloc(&d_src, buf_size) != cudaSuccess ||
	    cudaMalloc(&d_dst, buf_size) != cudaSuccess) {
		if (d_src)
			cudaFree(d_src);
		if (d_dst)
			cudaFree(d_dst);
		return false;
	}

	cudaMemset(d_src, 0x3c, buf_size);
	cudaMemset(d_dst, 0x00, buf_size);

	size_t num_elements = buf_size / sizeof(uint4);
	int threads = 256;
	int blocks = 58 * 8; // Saturate all 58 Ada SMs

	// Warmup
	vram_saturation_kernel<<<blocks, threads>>>(d_src, d_dst, num_elements);
	cudaDeviceSynchronize();

	cudaEvent_t start = nullptr, stop = nullptr;
	cudaEventCreate(&start);
	cudaEventCreate(&stop);

	constexpr int passes = 15;
	cudaEventRecord(start);
	for (int p = 0; p < passes; ++p) {
		vram_saturation_kernel<<<blocks, threads>>>(d_src, d_dst, num_elements);
	}
	cudaEventRecord(stop);
	cudaEventSynchronize(stop);

	float total_ms = 0.0f;
	cudaEventElapsedTime(&total_ms, start, stop);

	cudaEventDestroy(start);
	cudaEventDestroy(stop);

	cudaFree(d_src);
	cudaFree(d_dst);

	if (total_ms <= 0.0f) {
		return false;
	}

	if (out_allocated_bytes)
		*out_allocated_bytes = buf_size * 2;
	if (out_elapsed_ms)
		*out_elapsed_ms = total_ms;

	double total_bytes_transferred =
		2.0 * static_cast<double>(buf_size) * static_cast<double>(passes);
	double achieved_bw = (total_bytes_transferred / 1e9) / (total_ms / 1000.0);
	if (out_bandwidth_gb_s)
		*out_bandwidth_gb_s = achieved_bw;

	return true;
}


// =============================================================================
// Phase 3: Multi-Head Attention, SwiGLU & Transformer Block Implementations
// =============================================================================

__global__ static void batched_gemm_multiply_kernel(
	const __nv_bfloat16* __restrict__ A,
	const __nv_bfloat16* __restrict__ B,
	__nv_bfloat16* __restrict__ C,
	int M,
	int N,
	int K,
	bool transpose_A,
	bool transpose_B) {

	int col = blockIdx.x * blockDim.x + threadIdx.x;
	int row = blockIdx.y * blockDim.y + threadIdx.y;

	if (row < M && col < N) {
		float sum = 0.0f;
		for (int k = 0; k < K; ++k) {
			float a_val = transpose_A ? __bfloat162float(A[k * M + row])
						  : __bfloat162float(A[row * K + k]);
			float b_val = transpose_B ? __bfloat162float(B[col * K + k])
						  : __bfloat162float(B[k * N + col]);
			sum += a_val * b_val;
		}
		C[row * N + col] = __float2bfloat16(sum);
	}
}

extern "C" CUDA_API bool batched_gemm_multiply(const void* matrix_a,
					       const void* matrix_b,
					       void* matrix_c,
					       int rows_a,
					       int cols_b,
					       int common_dim,
					       bool transpose_a,
					       bool transpose_b,
					       void* stream) {
	if (!matrix_a || !matrix_b || !matrix_c || rows_a <= 0 || cols_b <= 0 || common_dim <= 0) {
		return false;
	}

	cudaStream_t s = (cudaStream_t)stream;
	dim3 block_dim(16, 16);
	dim3 grid_dim((cols_b + block_dim.x - 1) / block_dim.x,
		      (rows_a + block_dim.y - 1) / block_dim.y);

	batched_gemm_multiply_kernel<<<grid_dim, block_dim, 0, s>>>(
		static_cast<const __nv_bfloat16*>(matrix_a),
		static_cast<const __nv_bfloat16*>(matrix_b),
		static_cast<__nv_bfloat16*>(matrix_c),
		rows_a,
		cols_b,
		common_dim,
		transpose_a,
		transpose_b);

	return CUDA_CHECK(cudaGetLastError());
}

__global__ static void fused_causal_attention_kernel(
	const __nv_bfloat16* __restrict__ query,
	const __nv_bfloat16* __restrict__ key,
	const __nv_bfloat16* __restrict__ value,
	__nv_bfloat16* __restrict__ output,
	size_t sequence_length,
	size_t embedding_dim,
	size_t num_heads,
	size_t head_dim,
	float scale) {

	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	size_t total_queries = sequence_length * num_heads;
	if (idx >= total_queries)
		return;

	size_t i = idx / num_heads; // query token index in [0, sequence_length - 1]
	size_t h = idx % num_heads; // head index in [0, num_heads - 1]

	size_t head_offset = h * head_dim;
	const __nv_bfloat16* q_vec = query + i * embedding_dim + head_offset;

	// Online softmax state
	float max_score = -1e30f;
	float sum_exp = 0.0f;
	float acc[64] = {0.0f}; // Supports head_dim <= 64

	// Causal constraint: only attend to j <= i
	for (size_t j = 0; j <= i; ++j) {
		const __nv_bfloat16* k_vec = key + j * embedding_dim + head_offset;
		float dot = 0.0f;
		for (size_t d = 0; d < head_dim && d < 64; ++d) {
			dot += __bfloat162float(q_vec[d]) * __bfloat162float(k_vec[d]);
		}
		float score = dot * scale;

		float new_max = fmaxf(max_score, score);
		float exp_prev = expf(max_score - new_max);
		float exp_curr = expf(score - new_max);

		sum_exp = sum_exp * exp_prev + exp_curr;
		max_score = new_max;

		const __nv_bfloat16* v_vec = value + j * embedding_dim + head_offset;
		for (size_t d = 0; d < head_dim && d < 64; ++d) {
			acc[d] = acc[d] * exp_prev + exp_curr * __bfloat162float(v_vec[d]);
		}
	}

	float inv_sum = 1.0f / (sum_exp > 1e-12f ? sum_exp : 1.0f);
	__nv_bfloat16* out_vec = output + i * embedding_dim + head_offset;
	for (size_t d = 0; d < head_dim && d < 64; ++d) {
		float val = acc[d] * inv_sum;
		out_vec[d] = __float2bfloat16(val);
	}
}

extern "C" CUDA_API bool fused_causal_attention(const void* query,
						const void* key,
						const void* value,
						void* output,
						size_t sequence_length,
						size_t embedding_dim,
						size_t num_heads,
						void* stream) {
	if (!query || !key || !value || !output || sequence_length == 0 || embedding_dim == 0 ||
	    num_heads == 0) {
		return false;
	}

	size_t head_dim = embedding_dim / num_heads;
	if (head_dim == 0 || head_dim > 64) {
		return false;
	}

	float scale = 1.0f / sqrtf(static_cast<float>(head_dim));
	size_t total_queries = sequence_length * num_heads;
	int threads = 128;
	int blocks = static_cast<int>((total_queries + threads - 1) / threads);
	cudaStream_t s = (cudaStream_t)stream;

	fused_causal_attention_kernel<<<blocks, threads, 0, s>>>(
		static_cast<const __nv_bfloat16*>(query),
		static_cast<const __nv_bfloat16*>(key),
		static_cast<const __nv_bfloat16*>(value),
		static_cast<__nv_bfloat16*>(output),
		sequence_length,
		embedding_dim,
		num_heads,
		head_dim,
		scale);

	return CUDA_CHECK(cudaGetLastError());
}

__global__ static void swiglu_forward_kernel(
	const __nv_bfloat16* __restrict__ gate,
	const __nv_bfloat16* __restrict__ up,
	__nv_bfloat16* __restrict__ output,
	size_t total_elements) {

	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	if (idx < total_elements) {
		float g = __bfloat162float(gate[idx]);
		float u = __bfloat162float(up[idx]);
		float sig = 1.0f / (1.0f + expf(-g));
		float swish = g * sig;
		float h = swish * u;
		output[idx] = __float2bfloat16(h);
	}
}

extern "C" CUDA_API bool swiglu_forward(const void* gate,
					const void* up,
					void* output,
					size_t total_elements,
					void* stream) {
	if (!gate || !up || !output || total_elements == 0) {
		return false;
	}

	int threads = 256;
	int blocks = static_cast<int>((total_elements + threads - 1) / threads);
	cudaStream_t s = (cudaStream_t)stream;

	swiglu_forward_kernel<<<blocks, threads, 0, s>>>(
		static_cast<const __nv_bfloat16*>(gate),
		static_cast<const __nv_bfloat16*>(up),
		static_cast<__nv_bfloat16*>(output),
		total_elements);

	return CUDA_CHECK(cudaGetLastError());
}

__global__ static void tensor_add_inplace_kernel(
	__nv_bfloat16* __restrict__ dst,
	const __nv_bfloat16* __restrict__ src,
	size_t total_elements) {

	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	if (idx < total_elements) {
		float d = __bfloat162float(dst[idx]);
		float s = __bfloat162float(src[idx]);
		dst[idx] = __float2bfloat16(d + s);
	}
}

extern "C" CUDA_API bool transformer_forward_block(void* hidden_states,
						   const GpuWeights* gpu_weights,
						   GpuActivations* gpu_activations,
						   size_t sequence_length,
						   size_t embedding_dim,
						   size_t num_heads,
						   size_t ffn_dim,
						   void* stream) {
	if (!hidden_states || !gpu_weights || !gpu_activations || sequence_length == 0 ||
	    embedding_dim == 0 || num_heads == 0 || ffn_dim == 0) {
		return false;
	}

	cudaStream_t s = (cudaStream_t)stream;
	size_t total_seq_dim = sequence_length * embedding_dim;
	size_t total_seq_ffn = sequence_length * ffn_dim;

	// Allocate intermediate activations using BFC caching allocator
	if (!gpu_activations->d_query) {
		gpu_activations->d_query = allocate_vram(total_seq_dim * sizeof(__nv_bfloat16), s);
		gpu_activations->d_key = allocate_vram(total_seq_dim * sizeof(__nv_bfloat16), s);
		gpu_activations->d_value = allocate_vram(total_seq_dim * sizeof(__nv_bfloat16), s);
		gpu_activations->d_attention_output = allocate_vram(total_seq_dim * sizeof(__nv_bfloat16), s);
		gpu_activations->d_intermediate_gate = allocate_vram(total_seq_ffn * sizeof(__nv_bfloat16), s);
		gpu_activations->d_intermediate_up = allocate_vram(total_seq_ffn * sizeof(__nv_bfloat16), s);
		gpu_activations->d_intermediate_swiglu = allocate_vram(total_seq_ffn * sizeof(__nv_bfloat16), s);
		gpu_activations->d_ffn_output = allocate_vram(total_seq_dim * sizeof(__nv_bfloat16), s);
		gpu_activations->d_residual_mid = allocate_vram(total_seq_dim * sizeof(__nv_bfloat16), s);
		gpu_activations->d_residual_out = allocate_vram(total_seq_dim * sizeof(__nv_bfloat16), s);
	}

	if (!gpu_activations->d_query || !gpu_activations->d_key || !gpu_activations->d_value ||
	    !gpu_activations->d_attention_output || !gpu_activations->d_intermediate_gate ||
	    !gpu_activations->d_intermediate_up || !gpu_activations->d_intermediate_swiglu ||
	    !gpu_activations->d_ffn_output || !gpu_activations->d_residual_mid ||
	    !gpu_activations->d_residual_out) {
		return false;
	}

	// 1. Save residual copy of X
	cudaMemcpyAsync(gpu_activations->d_residual_mid, hidden_states,
			total_seq_dim * sizeof(__nv_bfloat16), cudaMemcpyDeviceToDevice, s);

	// 2. Pre-Norm: RMSNorm on X
	if (gpu_activations->d_inv_rms) {
		rms_norm_forward_kernel<<<1, 1024, 0, s>>>(
			static_cast<__nv_bfloat16*>(hidden_states),
			static_cast<float*>(gpu_activations->d_inv_rms),
			total_seq_dim,
			1e-5f);
	}

	// 3. Q, K, V projections: [S x D] * [D x D] -> [S x D]
	int S = static_cast<int>(sequence_length);
	int D = static_cast<int>(embedding_dim);
	int F = static_cast<int>(ffn_dim);

	batched_gemm_multiply(hidden_states, gpu_weights->d_query_weights, gpu_activations->d_query,
			      S, D, D, false, false, s);
	batched_gemm_multiply(hidden_states, gpu_weights->d_key_weights, gpu_activations->d_key,
			      S, D, D, false, false, s);
	batched_gemm_multiply(hidden_states, gpu_weights->d_value_weights, gpu_activations->d_value,
			      S, D, D, false, false, s);

	// 4. Fused Causal Attention
	fused_causal_attention(gpu_activations->d_query, gpu_activations->d_key, gpu_activations->d_value,
			       gpu_activations->d_ffn_output /* temporary output buffer */,
			       sequence_length, embedding_dim, num_heads, s);

	// 5. Output projection: O * W_O -> d_attention_output
	batched_gemm_multiply(gpu_activations->d_ffn_output, gpu_weights->d_output_weights,
			      gpu_activations->d_attention_output, S, D, D, false, false, s);

	// 6. Pre-Norm Residual Stream: X_mid = X_residual + O_proj
	int add_threads = 256;
	int add_blocks = static_cast<int>((total_seq_dim + add_threads - 1) / add_threads);
	tensor_add_inplace_kernel<<<add_blocks, add_threads, 0, s>>>(
		static_cast<__nv_bfloat16*>(gpu_activations->d_residual_mid),
		static_cast<const __nv_bfloat16*>(gpu_activations->d_attention_output),
		total_seq_dim);

	// Copy X_mid back into hidden_states and into d_residual_out for FFN residual
	cudaMemcpyAsync(hidden_states, gpu_activations->d_residual_mid,
			total_seq_dim * sizeof(__nv_bfloat16), cudaMemcpyDeviceToDevice, s);
	cudaMemcpyAsync(gpu_activations->d_residual_out, hidden_states,
			total_seq_dim * sizeof(__nv_bfloat16), cudaMemcpyDeviceToDevice, s);

	// 7. Pre-Norm for FFN: RMSNorm(X_mid)
	if (gpu_activations->d_inv_rms) {
		rms_norm_forward_kernel<<<1, 1024, 0, s>>>(
			static_cast<__nv_bfloat16*>(hidden_states),
			static_cast<float*>(gpu_activations->d_inv_rms),
			total_seq_dim,
			1e-5f);
	}

	// 8. FFN Projections: G = X_norm2 * W_gate, U = X_norm2 * W_up
	batched_gemm_multiply(hidden_states, gpu_weights->d_gate_weights,
			      gpu_activations->d_intermediate_gate, S, F, D, false, false, s);
	batched_gemm_multiply(hidden_states, gpu_weights->d_up_weights,
			      gpu_activations->d_intermediate_up, S, F, D, false, false, s);

	// 9. SwiGLU: Swish(G) * U -> d_intermediate_swiglu
	swiglu_forward(gpu_activations->d_intermediate_gate, gpu_activations->d_intermediate_up,
		       gpu_activations->d_intermediate_swiglu, total_seq_ffn, s);

	// 10. Down projection: d_intermediate_swiglu * W_down -> d_ffn_output
	batched_gemm_multiply(gpu_activations->d_intermediate_swiglu, gpu_weights->d_down_weights,
			      gpu_activations->d_ffn_output, S, D, F, false, false, s);

	// 11. Final Residual Stream: X_out = X_mid + Down
	tensor_add_inplace_kernel<<<add_blocks, add_threads, 0, s>>>(
		static_cast<__nv_bfloat16*>(gpu_activations->d_residual_out),
		static_cast<const __nv_bfloat16*>(gpu_activations->d_ffn_output),
		total_seq_dim);

	// Write X_out back to hidden_states
	cudaMemcpyAsync(hidden_states, gpu_activations->d_residual_out,
			total_seq_dim * sizeof(__nv_bfloat16), cudaMemcpyDeviceToDevice, s);

	return CUDA_CHECK(cudaGetLastError());
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 4: PagedAttention KV-Cache & Block Management
// ─────────────────────────────────────────────────────────────────────────────

__global__ static void paged_causal_attention_kernel(
	const __nv_bfloat16* __restrict__ query,
	const __nv_bfloat16* __restrict__ key_blocks,
	const __nv_bfloat16* __restrict__ value_blocks,
	const int* __restrict__ block_table,
	__nv_bfloat16* __restrict__ output,
	size_t query_length,
	size_t context_length,
	size_t block_size,
	size_t hidden_dimension,
	size_t num_heads,
	size_t head_dimension,
	float scale)
{
	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	size_t total_queries = query_length * num_heads;
	if (idx >= total_queries)
		return;

	size_t q_idx = idx / num_heads; // query token index
	size_t h = idx % num_heads;     // head index
	size_t head_offset = h * head_dimension;

	const __nv_bfloat16* q_vec = query + q_idx * hidden_dimension + head_offset;

	size_t max_attend_j = (query_length == 1) ? (context_length - 1) : q_idx;

	float max_score = -1e30f;
	float sum_exp = 0.0f;
	float accumulator[64] = {0.0f};

	for (size_t j = 0; j <= max_attend_j; ++j) {
		size_t logical_block = j / block_size;
		int physical_block = block_table[logical_block];
		if (physical_block < 0)
			continue;
		size_t token_offset = j % block_size;

		size_t block_stride = physical_block * (block_size * hidden_dimension) +
				      token_offset * hidden_dimension + head_offset;
		const __nv_bfloat16* k_vec = key_blocks + block_stride;

		float dot = 0.0f;
		for (size_t d = 0; d < head_dimension && d < 64; ++d) {
			dot += __bfloat162float(q_vec[d]) * __bfloat162float(k_vec[d]);
		}
		float score = dot * scale;

		float new_max = fmaxf(max_score, score);
		float exp_prev = expf(max_score - new_max);
		float exp_curr = expf(score - new_max);

		sum_exp = sum_exp * exp_prev + exp_curr;
		max_score = new_max;

		const __nv_bfloat16* v_vec = value_blocks + block_stride;
		for (size_t d = 0; d < head_dimension && d < 64; ++d) {
			accumulator[d] = accumulator[d] * exp_prev + exp_curr * __bfloat162float(v_vec[d]);
		}
	}

	float inv_sum = 1.0f / (sum_exp > 1e-12f ? sum_exp : 1.0f);
	__nv_bfloat16* out_vec = output + q_idx * hidden_dimension + head_offset;
	for (size_t d = 0; d < head_dimension && d < 64; ++d) {
		float val = accumulator[d] * inv_sum;
		out_vec[d] = __float2bfloat16(val);
	}
}

extern "C" CUDA_API bool allocate_paged_cache(GpuPagedCache* cache,
					      size_t total_blocks,
					      size_t block_size,
					      size_t hidden_dim,
					      size_t num_heads) {
	if (!cache || total_blocks == 0 || block_size == 0 || hidden_dim == 0 || num_heads == 0)
		return false;

	free_paged_cache(cache);

	cache->total_blocks = total_blocks;
	cache->block_size = block_size;
	cache->hidden_dimension = hidden_dim;
	cache->num_heads = num_heads;

	size_t block_elements = block_size * hidden_dim;
	size_t total_elements = total_blocks * block_elements;
	size_t buffer_bytes = total_elements * sizeof(__nv_bfloat16);

	if (!CUDA_CHECK(cudaMalloc(&cache->d_key_blocks, buffer_bytes)))
		return false;
	if (!CUDA_CHECK(cudaMalloc(&cache->d_value_blocks, buffer_bytes)))
		return false;

	// Pre-allocate device staging buffer for up to 1024 logical block IDs
	size_t table_bytes = 1024 * sizeof(int);
	if (!CUDA_CHECK(cudaMalloc(&cache->d_block_table, table_bytes)))
		return false;

	cudaMemset(cache->d_key_blocks, 0, buffer_bytes);
	cudaMemset(cache->d_value_blocks, 0, buffer_bytes);
	cudaMemset(cache->d_block_table, 0xFF, table_bytes);

	return true;
}

extern "C" CUDA_API void free_paged_cache(GpuPagedCache* cache) {
	if (!cache)
		return;

	if (cache->d_key_blocks) {
		cudaFree(cache->d_key_blocks);
		cache->d_key_blocks = nullptr;
	}
	if (cache->d_value_blocks) {
		cudaFree(cache->d_value_blocks);
		cache->d_value_blocks = nullptr;
	}
	if (cache->d_block_table) {
		cudaFree(cache->d_block_table);
		cache->d_block_table = nullptr;
	}

	cache->total_blocks = 0;
	cache->block_size = 0;
	cache->hidden_dimension = 0;
	cache->num_heads = 0;
}

extern "C" CUDA_API bool copy_physical_block(const GpuPagedCache* cache,
					     int src_block,
					     int dst_block,
					     size_t valid_tokens,
					     void* stream) {
	if (!cache || !cache->d_key_blocks || !cache->d_value_blocks)
		return false;
	if (src_block < 0 || dst_block < 0)
		return false;
	if (static_cast<size_t>(src_block) >= cache->total_blocks ||
	    static_cast<size_t>(dst_block) >= cache->total_blocks)
		return false;

	if (valid_tokens == 0)
		return true;
	if (valid_tokens > cache->block_size)
		valid_tokens = cache->block_size;

	cudaStream_t s = (cudaStream_t)stream;
	size_t block_elements = cache->block_size * cache->hidden_dimension;
	size_t copy_elements = valid_tokens * cache->hidden_dimension;
	size_t copy_bytes = copy_elements * sizeof(__nv_bfloat16);

	__nv_bfloat16* k_base = static_cast<__nv_bfloat16*>(cache->d_key_blocks);
	__nv_bfloat16* v_base = static_cast<__nv_bfloat16*>(cache->d_value_blocks);

	const __nv_bfloat16* src_k = k_base + src_block * block_elements;
	__nv_bfloat16* dst_k = k_base + dst_block * block_elements;

	const __nv_bfloat16* src_v = v_base + src_block * block_elements;
	__nv_bfloat16* dst_v = v_base + dst_block * block_elements;

	cudaMemcpyAsync(dst_k, src_k, copy_bytes, cudaMemcpyDeviceToDevice, s);
	cudaMemcpyAsync(dst_v, src_v, copy_bytes, cudaMemcpyDeviceToDevice, s);

	return CUDA_CHECK(cudaGetLastError());
}

extern "C" CUDA_API bool append_paged_kv(const GpuPagedCache* cache,
					 int physical_block,
					 size_t block_offset,
					 const void* key_vec,
					 const void* val_vec,
					 void* stream) {
	if (!cache || !cache->d_key_blocks || !cache->d_value_blocks)
		return false;
	if (physical_block < 0 || static_cast<size_t>(physical_block) >= cache->total_blocks)
		return false;
	if (block_offset >= cache->block_size || !key_vec || !val_vec)
		return false;

	cudaStream_t s = (cudaStream_t)stream;
	size_t block_elements = cache->block_size * cache->hidden_dimension;
	size_t element_offset = physical_block * block_elements + block_offset * cache->hidden_dimension;
	size_t copy_bytes = cache->hidden_dimension * sizeof(__nv_bfloat16);

	__nv_bfloat16* k_base = static_cast<__nv_bfloat16*>(cache->d_key_blocks);
	__nv_bfloat16* v_base = static_cast<__nv_bfloat16*>(cache->d_value_blocks);

	cudaMemcpyAsync(k_base + element_offset, key_vec, copy_bytes, cudaMemcpyDefault, s);
	cudaMemcpyAsync(v_base + element_offset, val_vec, copy_bytes, cudaMemcpyDefault, s);

	return CUDA_CHECK(cudaGetLastError());
}

extern "C" CUDA_API bool paged_causal_attention(const void* query,
						const GpuPagedCache* cache,
						const int* d_block_table,
						void* output,
						size_t query_length,
						size_t context_length,
						size_t hidden_dim,
						size_t num_heads,
						void* stream) {
	if (!query || !cache || !cache->d_key_blocks || !cache->d_value_blocks ||
	    !d_block_table || !output || query_length == 0 || context_length == 0 ||
	    hidden_dim == 0 || num_heads == 0)
		return false;

	size_t head_dim = hidden_dim / num_heads;
	if (head_dim == 0 || head_dim > 64)
		return false;

	float scale = 1.0f / sqrtf(static_cast<float>(head_dim));
	size_t total_queries = query_length * num_heads;
	int threads = 128;
	int blocks = static_cast<int>((total_queries + threads - 1) / threads);
	cudaStream_t s = (cudaStream_t)stream;

	paged_causal_attention_kernel<<<blocks, threads, 0, s>>>(
		static_cast<const __nv_bfloat16*>(query),
		static_cast<const __nv_bfloat16*>(cache->d_key_blocks),
		static_cast<const __nv_bfloat16*>(cache->d_value_blocks),
		d_block_table,
		static_cast<__nv_bfloat16*>(output),
		query_length,
		context_length,
		cache->block_size,
		hidden_dim,
		num_heads,
		head_dim,
		scale);

	return CUDA_CHECK(cudaGetLastError());
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 5: Zero-Overhead Hardware Saturation via CUDA Graphs
// ─────────────────────────────────────────────────────────────────────────────

extern "C" CUDA_API bool capture_training_graph(GpuGraphExecution* graph_engine,
					     const GpuPortion* gpu_portion,
					     GpuEmbeddings* gpu_embeddings,
					     GpuWeights* gpu_weights,
					     GpuActivations* gpu_activations,
					     size_t input_dim,
					     size_t output_dim,
					     float learning_rate,
					     int buffer_index) {
	if (!graph_engine || !gpu_portion || !gpu_weights || !gpu_activations ||
	    input_dim == 0 || output_dim == 0) {
		return false;
	}

	free_training_graph(graph_engine);

	cudaStream_t stream = (cudaStream_t)gpu_portion->stream[buffer_index];
	if (!stream) {
		return false;
	}
	graph_engine->stream = stream;

	// Warmup execution: ensures all internal static structures and memory pools
	// are allocated prior to stream capture, fulfilling CUDA graph capture constraints.
	forward_pass_with_embeddings(gpu_portion, gpu_embeddings, gpu_weights,
				     gpu_activations, input_dim, output_dim, buffer_index);
	backward_pass_with_embeddings(gpu_portion, gpu_embeddings, gpu_weights,
				      gpu_activations, input_dim, output_dim,
				      learning_rate, buffer_index, true);
	cudaStreamSynchronize(stream);

	// Begin stream capture mode
	if (!CUDA_CHECK(cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal))) {
		return false;
	}

	// Capture forward pass
	if (!forward_pass_with_embeddings(gpu_portion, gpu_embeddings, gpu_weights,
					  gpu_activations, input_dim, output_dim, buffer_index)) {
		cudaGraph_t failed_graph = nullptr;
		cudaStreamEndCapture(stream, &failed_graph);
		if (failed_graph) cudaGraphDestroy(failed_graph);
		return false;
	}

	// Capture backward pass
	if (!backward_pass_with_embeddings(gpu_portion, gpu_embeddings, gpu_weights,
					   gpu_activations, input_dim, output_dim,
					   learning_rate, buffer_index, true)) {
		cudaGraph_t failed_graph = nullptr;
		cudaStreamEndCapture(stream, &failed_graph);
		if (failed_graph) cudaGraphDestroy(failed_graph);
		return false;
	}

	cudaGraph_t captured_graph = nullptr;
	if (!CUDA_CHECK(cudaStreamEndCapture(stream, &captured_graph))) {
		if (captured_graph) cudaGraphDestroy(captured_graph);
		return false;
	}

	cudaGraphExec_t instantiated_exec = nullptr;
	if (!CUDA_CHECK(cudaGraphInstantiate(&instantiated_exec, captured_graph, nullptr, nullptr, 0))) {
		cudaGraphDestroy(captured_graph);
		return false;
	}

	graph_engine->graph = captured_graph;
	graph_engine->graph_exec = instantiated_exec;
	graph_engine->is_instantiated = true;
	return true;
}

extern "C" CUDA_API bool launch_training_graph(GpuGraphExecution* graph_engine, void* stream) {
	if (!graph_engine || !graph_engine->is_instantiated || !graph_engine->graph_exec) {
		return false;
	}
	cudaStream_t s = stream ? (cudaStream_t)stream : (cudaStream_t)graph_engine->stream;
	return CUDA_CHECK(cudaGraphLaunch((cudaGraphExec_t)graph_engine->graph_exec, s));
}

extern "C" CUDA_API void free_training_graph(GpuGraphExecution* graph_engine) {
	if (!graph_engine)
		return;

	if (graph_engine->graph_exec) {
		cudaGraphExecDestroy((cudaGraphExec_t)graph_engine->graph_exec);
		graph_engine->graph_exec = nullptr;
	}
	if (graph_engine->graph) {
		cudaGraphDestroy((cudaGraph_t)graph_engine->graph);
		graph_engine->graph = nullptr;
	}
	graph_engine->is_instantiated = false;
	graph_engine->launch_latency_ms = 0.0f;
}

extern "C" CUDA_API float benchmark_graph_latency(GpuGraphExecution* graph_engine, void* stream, int iterations) {
	if (!graph_engine || !graph_engine->is_instantiated || !graph_engine->graph_exec || iterations <= 0) {
		return -1.0f;
	}
	cudaStream_t s = stream ? (cudaStream_t)stream : (cudaStream_t)graph_engine->stream;

	// Warmup launch
	cudaGraphLaunch((cudaGraphExec_t)graph_engine->graph_exec, s);
	cudaStreamSynchronize(s);

	// Measure host launch latency (CPU time spent dispatching graph to queue)
	auto start_time = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < iterations; ++i) {
		cudaGraphLaunch((cudaGraphExec_t)graph_engine->graph_exec, s);
	}
	auto end_time = std::chrono::high_resolution_clock::now();
	cudaStreamSynchronize(s);

	double total_elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
	float avg_latency_ms = static_cast<float>(total_elapsed_ms / iterations);
	graph_engine->launch_latency_ms = avg_latency_ms;
	return avg_latency_ms;
}
