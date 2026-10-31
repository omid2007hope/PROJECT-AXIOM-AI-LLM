#ifndef TENSOR_HPP
#define TENSOR_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "../src/CUDA/util/memory_allocation.cuh"

// Supported hardware devices and numeric representations
enum class DeviceType : uint8_t { CPU = 0, CUDA = 1 };
enum class DataType : uint8_t { Float32 = 0, BFloat16 = 1 };

// Dispatch routing key flags
enum class DispatchKey : uint32_t {
	CPU = 1 << 0,
	CUDA = 1 << 1,
	Autograd = 1 << 2
};

// Bitset representing active dispatch keys
class DispatchKeySet {
public:
	constexpr DispatchKeySet() : mask(0) {}
	constexpr explicit DispatchKeySet(DispatchKey key) : mask(static_cast<uint32_t>(key)) {}
	constexpr explicit DispatchKeySet(uint32_t initial_mask) : mask(initial_mask) {}

	bool has_key(DispatchKey key) const {
		return (mask & static_cast<uint32_t>(key)) != 0;
	}

	DispatchKeySet add_key(DispatchKey key) const {
		return DispatchKeySet(mask | static_cast<uint32_t>(key));
	}

	DispatchKeySet remove_key(DispatchKey key) const {
		return DispatchKeySet(mask & ~static_cast<uint32_t>(key));
	}

	uint32_t raw_mask() const {
		return mask;
	}

private:
	uint32_t mask = 0;
};

// Reference-counted storage implementation containing shape, stride, and device pointer
class TensorImplementation {
public:
	std::vector<size_t> shape;
	std::vector<size_t> strides;
	size_t total_elements = 0;
	DeviceType device = DeviceType::CPU;
	DataType data_type = DataType::Float32;
	void* data_pointer = nullptr;
	bool owns_memory = true;
	std::shared_ptr<TensorImplementation> gradient;

	TensorImplementation(std::vector<size_t> dims,
			     DeviceType dev = DeviceType::CPU,
			     DataType dtype = DataType::Float32)
		: shape(std::move(dims)), device(dev), data_type(dtype), owns_memory(true) {
		compute_strides();
		allocate_storage();
	}

	// Non-owning view constructor (zero-copy strided view)
	TensorImplementation(std::vector<size_t> dims,
			     std::vector<size_t> custom_strides,
			     void* shared_data,
			     DeviceType dev,
			     DataType dtype)
		: shape(std::move(dims)), strides(std::move(custom_strides)),
		  device(dev), data_type(dtype), data_pointer(shared_data), owns_memory(false) {
		total_elements = 1;
		for (size_t d : shape) {
			total_elements *= d;
		}
	}

	~TensorImplementation() {
		if (owns_memory && data_pointer) {
			if (device == DeviceType::CPU) {
				delete[] static_cast<char*>(data_pointer);
			} else if (device == DeviceType::CUDA) {
				free_device_memory(data_pointer);
			}
			data_pointer = nullptr;
		}
	}

	float* data_as_float() {
		return static_cast<float*>(data_pointer);
	}

	const float* data_as_float() const {
		return static_cast<const float*>(data_pointer);
	}

	std::shared_ptr<TensorImplementation> create_view(const std::vector<size_t>& new_shape) {
		size_t view_elements = 1;
		for (size_t d : new_shape) {
			view_elements *= d;
		}
		if (view_elements != total_elements) {
			return nullptr;
		}
		std::vector<size_t> new_strides(new_shape.size(), 1);
		for (int i = static_cast<int>(new_shape.size()) - 2; i >= 0; --i) {
			new_strides[i] = new_strides[i + 1] * new_shape[i + 1];
		}
		return std::make_shared<TensorImplementation>(new_shape, new_strides, data_pointer, device, data_type);
	}

	std::shared_ptr<TensorImplementation> transpose_matrix(size_t dim0, size_t dim1) {
		if (dim0 >= shape.size() || dim1 >= shape.size()) {
			return nullptr;
		}
		std::vector<size_t> transposed_shape = shape;
		std::vector<size_t> transposed_strides = strides;
		std::swap(transposed_shape[dim0], transposed_shape[dim1]);
		std::swap(transposed_strides[dim0], transposed_strides[dim1]);
		return std::make_shared<TensorImplementation>(transposed_shape, transposed_strides, data_pointer, device, data_type);
	}

private:
	void compute_strides() {
		total_elements = 1;
		for (size_t d : shape) {
			total_elements *= d;
		}
		strides.resize(shape.size(), 1);
		if (!shape.empty()) {
			strides.back() = 1;
			for (int i = static_cast<int>(shape.size()) - 2; i >= 0; --i) {
				strides[i] = strides[i + 1] * shape[i + 1];
			}
		}
	}

	void allocate_storage() {
		size_t element_size = (data_type == DataType::Float32) ? sizeof(float) : sizeof(uint16_t);
		size_t total_bytes = total_elements * element_size;
		if (device == DeviceType::CPU) {
			data_pointer = new char[total_bytes];
			std::fill(static_cast<char*>(data_pointer), static_cast<char*>(data_pointer) + total_bytes, 0);
		} else if (device == DeviceType::CUDA) {
			data_pointer = allocate_device_memory(total_bytes);
		}
	}
};

// Forward declaration of Tensor
class Tensor;

// Node recorded on the reverse-mode computational tape
struct TapeNode {
	std::string operation_name;
	std::function<void()> backward_closure;
};

// Reverse-mode dynamic computational tape
class AutogradTape {
public:
	static AutogradTape& get_instance() {
		static AutogradTape instance;
		return instance;
	}

	bool is_active() const {
		return recording_active;
	}

	void set_active(bool active) {
		recording_active = active;
	}

	void record_tape_node(TapeNode node) {
		if (recording_active) {
			node_history.push_back(std::move(node));
		}
	}

	void execute_backward_pass() {
		if (node_history.empty()) {
			return;
		}
		for (int i = static_cast<int>(node_history.size()) - 1; i >= 0; --i) {
			if (node_history[i].backward_closure) {
				node_history[i].backward_closure();
			}
		}
		node_history.clear();
	}

	void clear_tape() {
		node_history.clear();
	}

private:
	bool recording_active = false;
	std::vector<TapeNode> node_history;
};

// Lightweight value-type Tensor handle
class Tensor {
public:
	std::shared_ptr<TensorImplementation> implementation;
	bool requires_grad = false;

	Tensor() = default;

	explicit Tensor(std::shared_ptr<TensorImplementation> impl, bool req_grad = false)
		: implementation(std::move(impl)), requires_grad(req_grad) {}

	std::shared_ptr<TensorImplementation> grad() const {
		return implementation ? implementation->gradient : nullptr;
	}

	void set_gradient(const Tensor& g) const {
		if (implementation && g.implementation) {
			implementation->gradient = g.implementation;
		}
	}

	static Tensor create_tensor(std::vector<size_t> shape, DeviceType dev = DeviceType::CPU, bool req_grad = false) {
		auto impl = std::make_shared<TensorImplementation>(std::move(shape), dev);
		return Tensor(impl, req_grad);
	}

	static Tensor create_from_vector(const std::vector<float>& values,
					 std::vector<size_t> shape,
					 DeviceType dev = DeviceType::CPU,
					 bool req_grad = false) {
		auto impl = std::make_shared<TensorImplementation>(shape, DeviceType::CPU);
		if (values.size() == impl->total_elements) {
			std::copy(values.begin(), values.end(), impl->data_as_float());
		}
		if (dev == DeviceType::CUDA) {
			auto cuda_impl = std::make_shared<TensorImplementation>(shape, DeviceType::CUDA);
			upload_device_buffer(cuda_impl->data_pointer, impl->data_as_float(), impl->total_elements * sizeof(float));
			return Tensor(cuda_impl, req_grad);
		}
		return Tensor(impl, req_grad);
	}

	const std::vector<size_t>& shape() const {
		return implementation->shape;
	}

	const std::vector<size_t>& strides() const {
		return implementation->strides;
	}

	size_t num_elements() const {
		return implementation ? implementation->total_elements : 0;
	}

	DeviceType device() const {
		return implementation ? implementation->device : DeviceType::CPU;
	}

	float* data_ptr() {
		return implementation ? implementation->data_as_float() : nullptr;
	}

	const float* data_ptr() const {
		return implementation ? implementation->data_as_float() : nullptr;
	}

	Tensor create_view(const std::vector<size_t>& new_shape) const {
		if (!implementation) return Tensor();
		auto view_impl = implementation->create_view(new_shape);
		return Tensor(view_impl, requires_grad);
	}

	Tensor transpose_matrix(size_t dim0, size_t dim1) const {
		if (!implementation) return Tensor();
		auto transp_impl = implementation->transpose_matrix(dim0, dim1);
		return Tensor(transp_impl, requires_grad);
	}

	void accumulate_gradient(const Tensor& incoming_grad) const {
		if (!requires_grad || !incoming_grad.implementation || !implementation) {
			return;
		}
		if (!implementation->gradient) {
			implementation->gradient = std::make_shared<TensorImplementation>(shape(), device(), DataType::Float32);
		}
		float* target_ptr = implementation->gradient->data_as_float();
		const float* source_ptr = incoming_grad.data_ptr();
		size_t elements = num_elements();
		for (size_t i = 0; i < elements; ++i) {
			target_ptr[i] += source_ptr[i];
		}
	}

	void backward() const {
		if (!implementation) return;
		if (!implementation->gradient) {
			implementation->gradient = std::make_shared<TensorImplementation>(shape(), device(), DataType::Float32);
			std::fill(implementation->gradient->data_as_float(),
				  implementation->gradient->data_as_float() + num_elements(), 1.0f);
		}
		AutogradTape::get_instance().execute_backward_pass();
	}
};

// ─────────────────────────────────────────────────────────────────────────────
// ATen Dispatcher Operations
// ─────────────────────────────────────────────────────────────────────────────

inline Tensor tensor_matmul(const Tensor& a, const Tensor& b) {
	if (a.shape().size() != 2 || b.shape().size() != 2) {
		throw std::runtime_error("tensor_matmul requires 2D matrices");
	}
	size_t M = a.shape()[0];
	size_t K = a.shape()[1];
	size_t K2 = b.shape()[0];
	size_t N = b.shape()[1];
	if (K != K2) {
		throw std::runtime_error("tensor_matmul dimension mismatch");
	}

	Tensor result = Tensor::create_tensor({M, N}, a.device(), a.requires_grad || b.requires_grad);
	float* c_ptr = result.data_ptr();
	const float* a_ptr = a.data_ptr();
	const float* b_ptr = b.data_ptr();

	for (size_t i = 0; i < M; ++i) {
		for (size_t j = 0; j < N; ++j) {
			float sum = 0.0f;
			for (size_t k = 0; k < K; ++k) {
				sum += a_ptr[i * K + k] * b_ptr[k * N + j];
			}
			c_ptr[i * N + j] = sum;
		}
	}

	if (AutogradTape::get_instance().is_active() && (a.requires_grad || b.requires_grad)) {
		TapeNode node;
		node.operation_name = "tensor_matmul";
		node.backward_closure = [a, b, result]() mutable {
			if (!result.grad()) return;
			const float* dL_dC = result.grad()->data_as_float();
			size_t M_dim = a.shape()[0];
			size_t K_dim = a.shape()[1];
			size_t N_dim = b.shape()[1];

			if (a.requires_grad) {
				// dA = dC * B^T -> [M, N] * [N, K] = [M, K]
				Tensor grad_a = Tensor::create_tensor({M_dim, K_dim}, a.device(), false);
				float* da_ptr = grad_a.data_ptr();
				const float* b_data = b.data_ptr();
				for (size_t i = 0; i < M_dim; ++i) {
					for (size_t k = 0; k < K_dim; ++k) {
						float sum = 0.0f;
						for (size_t j = 0; j < N_dim; ++j) {
							sum += dL_dC[i * N_dim + j] * b_data[k * N_dim + j];
						}
						da_ptr[i * K_dim + k] = sum;
					}
				}
				a.accumulate_gradient(grad_a);
			}

			if (b.requires_grad) {
				// dB = A^T * dC -> [K, M] * [M, N] = [K, N]
				Tensor grad_b = Tensor::create_tensor({K_dim, N_dim}, b.device(), false);
				float* db_ptr = grad_b.data_ptr();
				const float* a_data = a.data_ptr();
				for (size_t k = 0; k < K_dim; ++k) {
					for (size_t j = 0; j < N_dim; ++j) {
						float sum = 0.0f;
						for (size_t i = 0; i < M_dim; ++i) {
							sum += a_data[i * K_dim + k] * dL_dC[i * N_dim + j];
						}
						db_ptr[k * N_dim + j] = sum;
					}
				}
				b.accumulate_gradient(grad_b);
			}
		};
		AutogradTape::get_instance().record_tape_node(std::move(node));
	}

	return result;
}

inline Tensor tensor_add(const Tensor& a, const Tensor& b) {
	size_t elements = a.num_elements();
	bool is_broadcast_bias = (b.num_elements() == a.shape().back());

	Tensor result = Tensor::create_tensor(a.shape(), a.device(), a.requires_grad || b.requires_grad);
	float* c_ptr = result.data_ptr();
	const float* a_ptr = a.data_ptr();
	const float* b_ptr = b.data_ptr();

	size_t last_dim = a.shape().back();
	for (size_t i = 0; i < elements; ++i) {
		float b_val = is_broadcast_bias ? b_ptr[i % last_dim] : b_ptr[i];
		c_ptr[i] = a_ptr[i] + b_val;
	}

	if (AutogradTape::get_instance().is_active() && (a.requires_grad || b.requires_grad)) {
		TapeNode node;
		node.operation_name = "tensor_add";
		node.backward_closure = [a, b, result, is_broadcast_bias, last_dim]() mutable {
			if (!result.grad()) return;
			const float* dL_dC = result.grad()->data_as_float();

			if (a.requires_grad) {
				Tensor grad_a(result.grad(), false);
				a.accumulate_gradient(grad_a);
			}

			if (b.requires_grad) {
				if (is_broadcast_bias) {
					Tensor grad_b = Tensor::create_tensor(b.shape(), b.device(), false);
					float* gb_ptr = grad_b.data_ptr();
					size_t total = result.num_elements();
					for (size_t i = 0; i < total; ++i) {
						gb_ptr[i % last_dim] += dL_dC[i];
					}
					b.accumulate_gradient(grad_b);
				} else {
					Tensor grad_b(result.grad(), false);
					b.accumulate_gradient(grad_b);
				}
			}
		};
		AutogradTape::get_instance().record_tape_node(std::move(node));
	}

	return result;
}

inline Tensor tensor_scale(const Tensor& a, float factor) {
	Tensor result = Tensor::create_tensor(a.shape(), a.device(), a.requires_grad);
	float* c_ptr = result.data_ptr();
	const float* a_ptr = a.data_ptr();
	size_t total = a.num_elements();

	for (size_t i = 0; i < total; ++i) {
		c_ptr[i] = a_ptr[i] * factor;
	}

	if (AutogradTape::get_instance().is_active() && a.requires_grad) {
		TapeNode node;
		node.operation_name = "tensor_scale";
		node.backward_closure = [a, result, factor]() mutable {
			if (!result.grad()) return;
			Tensor grad_a = Tensor::create_tensor(a.shape(), a.device(), false);
			float* ga_ptr = grad_a.data_ptr();
			const float* dL_dC = result.grad()->data_as_float();
			size_t total_elements = a.num_elements();
			for (size_t i = 0; i < total_elements; ++i) {
				ga_ptr[i] = dL_dC[i] * factor;
			}
			a.accumulate_gradient(grad_a);
		};
		AutogradTape::get_instance().record_tape_node(std::move(node));
	}

	return result;
}

#endif // TENSOR_HPP

