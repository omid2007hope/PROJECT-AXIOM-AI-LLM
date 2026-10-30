#include "../include/optimizer.hpp"

#include <algorithm> // std::shuffle
#include <chrono>
#include <cstring>   // std::memcpy
#include <numeric>   // std::iota
#include <thread>

void Optimizer::read_chunk_into(std::vector<std::bfloat16_t>& buffer,
				size_t chunk_size,
				size_t context_size,
				size_t dim) {
	if (!file_stream.is_open()) {
		file_stream.open(data_path, std::ios::binary);
		if (!file_stream) {
			std::cerr << "Failed to open " << data_path << "\n";
			return;
		}
	}

	if (file_stream.eof()) {
		file_stream.clear();
		file_stream.seekg(0, std::ios::beg);
	}

	// Prepend seam carry-forward if present
	std::vector<std::bfloat16_t> carry = seam_carry;
	size_t carry_elements = context_size * dim;
	if (carry.size() > carry_elements) {
		carry.assign(carry.end() - carry_elements, carry.end());
	}

	buffer.clear();
	buffer = carry;

	size_t brain_floats_needed = (chunk_size > buffer.size()) ? (chunk_size - buffer.size()) : 0;
	size_t prev_size = buffer.size();

	buffer.resize(prev_size + brain_floats_needed);

	file_stream.read(
		reinterpret_cast<char*>(buffer.data() + prev_size),
		brain_floats_needed * sizeof(std::bfloat16_t));

	std::streamsize bytes_read = file_stream.gcount();
	size_t brain_floats_read = bytes_read / sizeof(std::bfloat16_t);
	buffer.resize(prev_size + brain_floats_read);

	// Ensure minimum data if file is small
	size_t min_needed = (context_size + 1) * dim;
	while (buffer.size() > 0 && buffer.size() < min_needed) {
		file_stream.clear();
		file_stream.seekg(0, std::ios::beg);
		size_t cur = buffer.size();
		size_t needed = min_needed - cur;
		buffer.resize(cur + needed);
		file_stream.read(
			reinterpret_cast<char*>(buffer.data() + cur),
			needed * sizeof(std::bfloat16_t));
		std::streamsize more = file_stream.gcount();
		if (more == 0) {
			buffer.resize(cur);
			break;
		}
		buffer.resize(cur + (more / sizeof(std::bfloat16_t)));
	}

	// Update seam carry for the next chunk
	if (buffer.size() >= carry_elements) {
		seam_carry.assign(buffer.end() - carry_elements, buffer.end());
	}
}

void Optimizer::prefetch_loop(size_t chunk_size, size_t context_size, size_t dim) {
	while (prefetch_active.load(std::memory_order_acquire)) {
		if (chunk_buffer.can_produce()) {
			ChunkSlot& slot = chunk_buffer.get_producer_slot();
			read_chunk_into(slot.tokens, chunk_size, context_size, dim);
			slot.elements = slot.tokens.size();
			chunk_buffer.push_produced();
		} else {
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}
	}
}

void Optimizer::start_prefetch(const Portion& portion) {
	if (!prefetch_active.load(std::memory_order_acquire)) {
		prefetch_active.store(true, std::memory_order_release);
		prefetch_worker = std::thread(
			&Optimizer::prefetch_loop,
			this,
			portion.ram_buffer_size,
			portion.context_window_size,
			portion.token_dim);
	}
}

void Optimizer::stop_prefetch() {
	if (prefetch_active.load(std::memory_order_acquire)) {
		prefetch_active.store(false, std::memory_order_release);
		if (prefetch_worker.joinable()) {
			prefetch_worker.join();
		}
		chunk_buffer.clear();
	}
}

void Optimizer::disk_to_ram(Portion& portion) {
	if (portion.buffer_in_use.load(std::memory_order_acquire) == false &&
	    portion.context_in_use.load(std::memory_order_acquire) == false) {
		portion.buffer_in_use.store(true, std::memory_order_release);

		if (chunk_buffer.can_consume()) {
			ChunkSlot& slot = chunk_buffer.get_consumer_slot();
			portion.ram_buffer_tokens = std::move(slot.tokens);
			chunk_buffer.pop_consumed();
		} else {
			read_chunk_into(
				portion.ram_buffer_tokens,
				portion.ram_buffer_size,
				portion.context_window_size,
				portion.token_dim);
		}

		portion.offset = 0;
		portion.buffer_in_use.store(false, std::memory_order_release);
	}
}

bool Optimizer::load_context_window(Portion& portion) {
	if (portion.context_in_use.load(std::memory_order_acquire) == false &&
	    portion.buffer_in_use.load(std::memory_order_acquire) == false) {

		portion.context_in_use.store(true, std::memory_order_release);
		portion.buffer_in_use.store(true, std::memory_order_release);

		int buffer_index = portion.buffer_index;

		if (!portion.context_window_tokens[buffer_index] ||
		    !portion.target_tokens[buffer_index]) {
			std::cerr << "[Optimizer] Pinned memory not allocated. Call "
				     "portion.allocate_pinned() first.\n";
			portion.buffer_in_use.store(false, std::memory_order_release);
			portion.context_in_use.store(false, std::memory_order_release);
			return false;
		}

		if (!portion.ram_buffer_token_ids.empty()) {
			const size_t total_windows =
				(portion.ram_buffer_token_ids.size() > portion.context_window_size)
					? portion.ram_buffer_token_ids.size() - portion.context_window_size
					: 0;

			if (portion.offset == 0 || portion.window_order.size() != total_windows) {
				portion.window_order.resize(total_windows);
				std::iota(portion.window_order.begin(), portion.window_order.end(), 0U);
				std::shuffle(portion.window_order.begin(),
					     portion.window_order.end(),
					     portion.shuffle_engine);
			}

			if (portion.offset < total_windows) {
				const size_t window_start = portion.window_order[portion.offset];
				if (portion.context_token_ids[buffer_index]) {
					std::memcpy(portion.context_token_ids[buffer_index],
						    portion.ram_buffer_token_ids.data() + window_start,
						    portion.context_window_size * sizeof(uint32_t));
				}
				uint32_t target_id = portion.ram_buffer_token_ids[window_start + portion.context_window_size];
				std::memset(portion.target_tokens[buffer_index], 0,
					    portion.target_elements * sizeof(std::bfloat16_t));
				*reinterpret_cast<uint32_t*>(portion.target_tokens[buffer_index]) = target_id;

				portion.has_token_ids = true;
				portion.offset += 1;

				portion.buffer_in_use.store(false, std::memory_order_release);
				portion.context_in_use.store(false, std::memory_order_release);
				return true;
			} else {
				portion.offset = 0;
				portion.buffer_in_use.store(false, std::memory_order_release);
				portion.context_in_use.store(false, std::memory_order_release);
				return false;
			}
		}

		if (portion.offset + portion.all_elements + portion.target_elements <=
		    portion.ram_buffer_tokens.size()) {

			std::memcpy(portion.context_window_tokens[buffer_index],
				    portion.ram_buffer_tokens.data() + portion.offset,
				    portion.all_elements * sizeof(std::bfloat16_t));

			std::memcpy(portion.target_tokens[buffer_index],
				    portion.ram_buffer_tokens.data() + portion.offset +
					    portion.all_elements,
				    portion.target_elements * sizeof(std::bfloat16_t));

			portion.has_token_ids = false;
			portion.offset += portion.token_dim;

			portion.buffer_in_use.store(false, std::memory_order_release);
			portion.context_in_use.store(false, std::memory_order_release);

			return true;
		} else {
			portion.offset = 0;

			portion.buffer_in_use.store(false, std::memory_order_release);
			portion.context_in_use.store(false, std::memory_order_release);

			return false;
		}
	}

	return false;
}
