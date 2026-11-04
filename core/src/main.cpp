// =============================================================================
// AXIOM — main.cpp
//
// main() is the orchestrator. Every path, hyperparameter and sample value is
// decided in this file and handed down to the Parser, Optimizer and AI. The
// utility layers never decide anything on their own.
//
//   app.exe            interactive menu
//   app.exe --train    train on the built-in sample data
//   app.exe --test     run the test suites
// =============================================================================

#define NOMINMAX // keep <windows.h> from defining min()/max() macros

#include <chrono>
#include <cmath>
#include <conio.h> // For _kbhit() and _getch()
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <io.h> // For _isatty() and _fileno()
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdfloat> // Required for std::bfloat16_t (C++23)
#include <string>
#include <type_traits>
#include <vector>

// psapi.h needs the types from windows.h, so this order must not be re-sorted
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on

#include "../include/ai.hpp"
#include "../include/optimizer.hpp"
#include "../include/telemetry.hpp"
#include "../include/server.hpp"
#include "../src/CUDA/kernel/forward_pass.cuh"

bool test_path();
bool test_ifstream();
bool test_embeddings();
bool test_tokenization();
bool test_optimizer();
bool test_cuda_transfer();
bool test_transfer();
bool test_ai();
bool test_inference();
bool test_identifier();
bool test_decompressor();

// *****************************************************************************
// Configuration: the single source of truth for everything the backend uses
// *****************************************************************************
namespace config {

// Hyperparameters
constexpr size_t ram_buffer_size = 4194304; // 4M tokens (~16.7 MB in RAM)
constexpr size_t token_dim = 1024;	    // GPT-2 scale (Must be multiple of 64)
constexpr size_t context_window_size = 256; // Doubled to capture paragraph-level context

// AdamW Optimizer Settings
constexpr float learning_rate = 0.0005f; // Scaled down for stable high-dimensional convergence

// How many training steps pass between two loss reports
constexpr size_t loss_report_interval = 1000; // Increased to prevent CPU/GPU synchronization stalls

// Initial weights are drawn uniformly from [-sqrt(3/K), +sqrt(3/K)], K = fan-in =
// context_window_size * token_dim = 262144. That gives std 1/sqrt(K) = 1/512, so the
// RMS-normalized input produces unit-variance outputs. (1/sqrt(token_dim) ignores the
// 256x fan-in and explodes the logits.) sqrt(3)/512 = 0.0033854167f.
constexpr float weight_min = -0.0033854167f;
constexpr float weight_max = 0.0033854167f;

// Folder layout
constexpr const char* database_folder = "database";
constexpr const char* models_folder = "models";
constexpr const char* training_folder = "training";
constexpr const char* embedded_file = "embedded_data.bin";
constexpr const char* asset_folder = "asset_sample";
constexpr const char* weights_file = "weights.axiom";
constexpr const char* vocab_file = "vocab.axiom";

} // namespace config

enum class AppMode { Chat, Use, Train, BulkTrain, Test, Benchmark, Server, Exit };

// core/ folder, located from the executable binary with fallbacks
std::filesystem::path locate_core_dir() {
#ifdef _WIN32
	wchar_t buf[MAX_PATH];
	DWORD len = GetModuleFileNameW(NULL, buf, MAX_PATH);
	if (len > 0 && len < MAX_PATH) {
		std::filesystem::path exe_dir = std::filesystem::path(buf).parent_path();
		if (std::filesystem::exists(exe_dir / config::asset_folder)) {
			return exe_dir;
		}
	}
#endif
	const std::filesystem::path cur = std::filesystem::current_path();
	if (std::filesystem::exists(cur / config::asset_folder)) {
		return cur;
	}
	if (std::filesystem::exists(cur / "core" / config::asset_folder)) {
		return cur / "core";
	}
	std::filesystem::path p = std::filesystem::absolute(__FILE__).parent_path().parent_path();
	return p.empty() ? cur : p;
}

// Raw data samples used by developer mode (--train)
Test make_sample_data() {
	Test test;
	test.test_input = {"The",  "quick", "brown", "fox", "jumps", "over", "the",
			   "lazy", "dog",   "5.1",   "3.5", "1.4",   "0.2",  "4.9",
			   "3.0",  "1.4",   "0.2",   "4.7", "3.2",   "1.3",  "0.2"};

	test.data_image = {"Photo.jpeg"};
	test.data_video = {"Video.mp4"};
	test.data_voice = {"Voice.ogg"};

	return test;
}

// *****************************************************************************
// Console input
// *****************************************************************************

// Strips optional UTF-8 Byte Order Mark (0xEF, 0xBB, 0xBF) often prepended by PowerShell pipes
inline void strip_utf8_bom(std::string& s) {
	if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF &&
	    static_cast<unsigned char>(s[1]) == 0xBB && static_cast<unsigned char>(s[2]) == 0xBF) {
		s.erase(0, 3);
	}
}

// ! Clears input stream errors and flushes invalid buffer characters
void safeCin() {
	if (!std::cin) {
		std::cin.clear();
	}
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Reads a menu choice; returns 0 when the input is not a valid clean integer
int read_choice() {
	std::string line;
	while (std::getline(std::cin, line)) {
		strip_utf8_bom(line);
		size_t start = line.find_first_not_of(" \t\r\n");
		if (start == std::string::npos) {
			continue;
		}
		std::istringstream iss(line.substr(start));
		int choice = 0;
		char extra = 0;
		if ((iss >> choice) && !(iss >> extra)) {
			return choice;
		}
		return 0;
	}
	return 0;
}

// Pauses and waits for a key press before console window closes
void pause_exit() {
	if (!_isatty(_fileno(stdin))) {
		return;
	}
	std::cout << "\nPress any key to exit...\n";
	while (_kbhit()) {
		_getch();
	}
	_getch();
}

// Pauses before returning to the main menu in interactive mode
void pause_menu() {
	if (!_isatty(_fileno(stdin))) {
		std::string dummy;
		if (std::cin && !std::cin.eof()) {
			std::getline(std::cin, dummy);
		}
		return;
	}
	while (_kbhit()) {
		_getch();
	}
	std::cout << "\nPress Enter to return to main menu...\n" << std::flush;
	std::string dummy;
	std::getline(std::cin, dummy);
}

// Validates a single whitespace-free token
template <typename E>
bool parse_token(const std::string& token, E& value) {
	char extra;
	std::istringstream iss(token);

	if (!(iss >> value) || (iss >> extra)) {
		return false;
	}

	if constexpr (std::is_arithmetic_v<E>) {
		// Numeric types: zero or positive only
		return value >= 0;
	} else {
		// Non-numeric types (e.g. std::string): any non-empty token is accepted
		return true;
	}
}

// Validator for final stage v-1
// Asks until the whole line is a non-empty, space-separated list of valid
// values. T is a vector of the wanted element type.
template <typename T>
T Validate(const std::string& c_out) {
	using Element = typename T::value_type;

	std::string c_in;

	while (true) {
		std::cout << c_out << "\n";

		if (!std::getline(std::cin, c_in)) {
			std::cin.clear();
			std::cerr << "\n[!] Input stream closed. Exiting.\n";
			std::exit(1);
		}
		strip_utf8_bom(c_in);

		// If more lines from a multi-line paste are already queued in the console buffer, absorb them
		if constexpr (std::is_same_v<Element, std::string>) {
			while (_isatty(_fileno(stdin)) && _kbhit()) {
				std::string next_line;
				if (std::getline(std::cin, next_line)) {
					strip_utf8_bom(next_line);
					c_in += " " + next_line;
				} else {
					break;
				}
			}
		}

		T data;
		Element value {};
		std::string token;
		std::istringstream iss(c_in);
		bool valid = true;

		while (iss >> token) {
			if (!parse_token(token, value)) {
				valid = false;
				break;
			}
			data.push_back(value);
		}

		if (valid && !data.empty()) {
			return data;
		}

		std::cout << "[!] Invalid input. Please try again.\n";
	}
}

// Validates a single file or directory path, preserving whitespace and stripping quotes
std::vector<std::string> ValidatePaths(const std::string& c_out) {
	std::string c_in;
	while (true) {
		std::cout << c_out << "\n";
		if (!std::getline(std::cin, c_in)) {
			std::cin.clear();
			std::cerr << "\n[!] Input stream closed. Exiting.\n";
			std::exit(1);
		}
		strip_utf8_bom(c_in);
		size_t start = c_in.find_first_not_of(" \t\r\n");
		if (start == std::string::npos) {
			std::cout << "[!] Invalid input. Please try again.\n";
			continue;
		}
		size_t end = c_in.find_last_not_of(" \t\r\n");
		std::string trimmed = c_in.substr(start, end - start + 1);
		if (trimmed.size() >= 2 && trimmed.front() == '"' && trimmed.back() == '"') {
			trimmed = trimmed.substr(1, trimmed.size() - 2);
		}
		if (!trimmed.empty()) {
			return {trimmed};
		}
		std::cout << "[!] Invalid input. Please try again.\n";
	}
}

// fetch arguments
std::string fetch_argument(int count, char* argument[]) {
	return count > 1 ? std::string(argument[1]) : std::string();
}

// Interactive menu. Fills `input` and returns the selected AppMode.
AppMode collect_user_input(Input& input) {
	int option = 0;
	while (true) {
		std::cout << "======================================\n"
			  << "           Welcome to AXIOM           \n"
			  << "======================================\n"
			  << "[1] Chat with Axiom (Direct Qwen LLM)\n"
			  << "[2] Train it (Training)\n"
			  << "[3] Run Tests\n"
			  << "[4] Benchmark Hardware (Telemetry HUD)\n"
			  << "[5] Bulk Training (asset_sample)\n";
		if (is_background_axiom_server_running()) {
			std::cout << "[6] OpenAI/Ollama Server: [RUNNING on port 8080] (Select to Stop or Switch)\n";
		} else {
			std::cout << "[6] Launch OpenAI Server (Cline / ComfyUI / Port 8080)\n";
		}
		std::cout << "[7] Single-Layer Perceptron (Legacy Inference)\n"
			  << "[8] Exit\n"
			  << "> ";

		option = read_choice();

		if (!std::cin || std::cin.eof()) {
			return AppMode::Exit;
		}

		if (option == 1) {
			return AppMode::Chat;
		}

		if (option == 3) {
			return AppMode::Test;
		}

		if (option == 4) {
			return AppMode::Benchmark;
		}

		if (option == 5) {
			return AppMode::BulkTrain;
		}

		if (option == 6) {
			return AppMode::Server;
		}

		if (option == 7) {
			return AppMode::Use;
		}

		if (option == 8) {
			return AppMode::Exit;
		}

		if (option != 2) {
			std::cout << "\n[!] Invalid option.\n";
			if (!_isatty(_fileno(stdin))) {
				return AppMode::Exit;
			}
			continue;
		}

		break;
	}

	std::cout << "\n--- Training Mode: ON ---\n\n";

	int type = 0;
	while (true) {
		std::cout << "Select Data Type:\n"
			  << "[1] Text\n"
			  << "[2] Image\n"
			  << "[3] Video\n"
			  << "[4] Voice\n"
			  << "> ";

		type = read_choice();

		if (!std::cin || std::cin.eof()) {
			return AppMode::Exit;
		}

		if (type >= 1 && type <= 4) {
			break;
		}

		std::cout << "\n[!] Invalid data type.\n";
		if (!_isatty(_fileno(stdin))) {
			return AppMode::Exit;
		}
	}

	if (type == 1) {
		input.user_input = Validate<std::vector<std::string>>("Enter Text: ");
	} else if (type == 2) {
		input.user_image_paths = ValidatePaths("Enter Image Path: ");
	} else if (type == 3) {
		input.user_video_paths = ValidatePaths("Enter Video Path: ");
	} else if (type == 4) {
		input.user_voice_paths = ValidatePaths("Enter Voice Path: ");
	}

	// Text and numbers can be paired with media
	if (type == 1) {
		int media = 0;
		while (true) {
			std::cout << "\nAdd media?\n[1] Yes\n[2] No\n> ";

			media = read_choice();

			if (!std::cin || std::cin.eof()) {
				return AppMode::Exit;
			}

			if (media == 1 || media == 2) {
				break;
			}

			std::cout << "\n[!] Invalid option.\n";
			if (!_isatty(_fileno(stdin))) {
				return AppMode::Exit;
			}
		}

		if (media == 1) {
			int media_type = 0;
			while (true) {
				std::cout << "\nSelect Media Type:\n"
					  << "[1] Image\n"
					  << "[2] Video\n"
					  << "[3] Voice\n"
					  << "> ";

				media_type = read_choice();

				if (!std::cin || std::cin.eof()) {
					return AppMode::Exit;
				}

				if (media_type >= 1 && media_type <= 3) {
					break;
				}

				std::cout << "\n[!] Invalid media type.\n";
				if (!_isatty(_fileno(stdin))) {
					return AppMode::Exit;
				}
			}

			if (media_type == 1) {
				input.user_image_paths = ValidatePaths("Enter Image Path: ");
			} else if (media_type == 2) {
				input.user_video_paths = ValidatePaths("Enter Video Path: ");
			} else if (media_type == 3) {
				input.user_voice_paths = ValidatePaths("Enter Voice Path: ");
			}
		}
	}

	return AppMode::Train;
}

// *****************************************************************************
// Live hardware telemetry
// *****************************************************************************
void print_hardware_telemetry(const Portion& portion) {
	PROCESS_MEMORY_COUNTERS pmc {};
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));

	MEMORYSTATUSEX memInfo {};
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);

	size_t gpu_free = 0, gpu_total = 0;
	get_gpu_telemetry(&gpu_free, &gpu_total);

	std::error_code ec;
	const std::filesystem::space_info disk_space =
		std::filesystem::space(std::filesystem::current_path(ec), ec);
	const double disk_free_gb = ec ? 0.0 : disk_space.available / 1024.0 / 1024.0 / 1024.0;

	std::cout << "\n========================================================\n"
		  << "                LIVE HARDWARE TELEMETRY                 \n"
		  << "========================================================\n"
		  << std::fixed << std::setprecision(2)
		  << "[CPU] L1/L2/L3 Cache: BYPASSED (Pinned Write-Combining)\n"
		  << "[RAM] Engine Stream : "
		  << (portion.ram_buffer_size * sizeof(std::bfloat16_t) / 1024.0 / 1024.0)
		  << " MB\n"
		  << "[RAM] Process Usage : " << (pmc.WorkingSetSize / 1024.0 / 1024.0) << " MB\n"
		  << "[RAM] System Avail  : " << (memInfo.ullAvailPhys / 1024.0 / 1024.0 / 1024.0)
		  << " GB / " << (memInfo.ullTotalPhys / 1024.0 / 1024.0 / 1024.0) << " GB\n"
		  << "[GPU] Context Window: "
		  << (portion.all_elements * sizeof(std::bfloat16_t) / 1024.0) << " KB\n"
		  << "[GPU] VRAM Available: " << (gpu_free / 1024.0 / 1024.0) << " MB / "
		  << (gpu_total / 1024.0 / 1024.0) << " MB\n"
		  << "[DSK] Disk Free     : " << disk_free_gb << " GB\n"
		  << "========================================================\n\n"
		  << std::flush;
}

// *****************************************************************************
// Loss report: the proof that the network is learning
// *****************************************************************************

// Prints the mean loss of the last interval next to the very first one. A falling
// number means the weights are really being pulled toward the token sequences.
void print_loss(AI& ai, size_t step, std::optional<double>& first_loss) {
	double loss = 0.0;
	size_t steps = 0;

	if (!ai.read_loss(loss, steps)) {
		std::cerr << "[!] Could not read the loss from the GPU.\n";
		return;
	}

	if (!first_loss) {
		first_loss = loss;
	}

	std::cout << std::fixed << std::setprecision(6) << "[Step " << step << "] loss " << loss;

	if (*first_loss > 0.0) {
		std::cout << std::showpos << std::setprecision(1) << "  ("
			  << (loss - *first_loss) / *first_loss * 100.0 << "% vs first)"
			  << std::noshowpos;
	}

	std::cout << "\n" << std::flush;
}

// *****************************************************************************
// Direct Chat Engine (Interactive Qwen 28-Layer GQA)
// *****************************************************************************
void run_chat_engine(AI& ai,
                     const std::filesystem::path& weights_path,
                     const std::filesystem::path& vocab_path) {
#ifdef _WIN32
	HANDLE h_console = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD console_mode = 0;
	if (GetConsoleMode(h_console, &console_mode)) {
		SetConsoleMode(h_console, console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	}
#endif

	std::filesystem::path target_weights = weights_path;
	std::filesystem::path target_vocab = vocab_path;

	if (std::filesystem::exists("models/qwen3b.axiom")) {
		target_weights = "models/qwen3b.axiom";
		target_vocab = std::filesystem::exists("models/qwen3b_vocab.axiom") ? "models/qwen3b_vocab.axiom" : "models/qwen_vocab.axiom";
	} else if (std::filesystem::exists("core/models/qwen3b.axiom")) {
		target_weights = "core/models/qwen3b.axiom";
		target_vocab = std::filesystem::exists("core/models/qwen3b_vocab.axiom") ? "core/models/qwen3b_vocab.axiom" : "core/models/qwen_vocab.axiom";
	}

	if (!ai.has_transformer_model()) {
		std::cout << "\033[90m[..] Streaming Qwen 3B weights via Win32 DMA mmap...\033[0m\n" << std::flush;
		bool vocab_ok = ai.load_vocabulary(target_vocab.string());
		bool model_ok = ai.load_axiom_model_mmap(target_weights.string().c_str());
		if (!vocab_ok || !model_ok) {
			std::cerr << "\033[31m[!] Failed to load transformer weights from: " << target_weights << "\033[0m\n" << std::flush;
			return;
		}
		std::cout << "\033[32m[OK] Stacked GQA Transformer weights loaded into VRAM.\033[0m\n\n" << std::flush;
	}

	std::cout << "\033[1;36m========================================================================\033[0m\n"
		  << "\033[1;37m       PROJECT AXIOM — Ada Lovelace sm_89 Direct GQA Transformer        \033[0m\n"
		  << "\033[1;36m========================================================================\033[0m\n"
		  << "  Model Architecture : \033[1;32mStacked Grouped Query Attention (GQA)\033[0m\n"
		  << "  Layer Count        : \033[1;33m" << ai.config_.num_layers << " stacked blocks\033[0m (Hidden: " << ai.config_.hidden_dim << ", FFN: " << ai.config_.ffn_dim << ")\n"
		  << "  Attention Heads    : \033[1;33m" << ai.config_.num_q_heads << " Q-Heads / " << ai.config_.num_kv_heads << " KV-Heads\033[0m (Head Dim: " << ai.config_.head_dim << ")\n"
		  << "  Context Window     : \033[1;33m" << ai.config_.max_seq_len << " tokens\033[0m (Paged Dynamic KV-Cache)\n"
		  << "  Compute Hardware   : \033[1;32m128-Bit uint4 Coalesced Memory + GPU Softmax/Argmax\033[0m\n"
		  << "  Vocabulary Size    : \033[1;33m" << ai.config_.vocab_size << " tokens\033[0m (BPE Exact Code Preserved)\n"
		  << "\033[1;36m------------------------------------------------------------------------\033[0m\n"
		  << "\033[90mCommands: 'exit' to quit | 'clear' to reset context | 'temp <val>' for sampling\033[0m\n"
		  << "\033[1;36m------------------------------------------------------------------------\033[0m\n";

	std::string conversation_history =
		"<|im_start|>system\nYou are Project Axiom, an expert C++23, CUDA, and systems programming AI.<|im_end|>\n";
	float current_temp = 0.0f;

	std::string user_input;
	while (true) {
		std::cout << "\n\033[1;36mYou ❯ \033[0m";
		if (!std::getline(std::cin, user_input)) break;
		strip_utf8_bom(user_input);
		while (!user_input.empty() && (user_input.back() == '\r' || user_input.back() == ' ' || user_input.back() == '\t')) {
			user_input.pop_back();
		}

		if (user_input.empty()) continue;
		if (user_input == "exit" || user_input == "quit" || user_input == ":q") {
			std::cout << "\033[90mExiting chat session.\033[0m\n";
			break;
		}
		if (user_input == "clear" || user_input == "reset") {
			conversation_history =
				"<|im_start|>system\nYou are Project Axiom, an expert C++23, CUDA, and systems programming AI.<|im_end|>\n";
			std::cout << "\033[33m[Axiom] Context memory reset.\033[0m\n";
			continue;
		}
		if (user_input.rfind("temp ", 0) == 0) {
			try {
				current_temp = std::stof(user_input.substr(5));
				std::cout << "\033[33m[Axiom] Generation temperature set to: " << current_temp << "\033[0m\n";
			} catch (...) {
				std::cout << "\033[31m[!] Invalid temperature value.\033[0m\n";
			}
			continue;
		}

		conversation_history += "<|im_start|>user\n" + user_input + "<|im_end|>\n<|im_start|>assistant\n";

		std::cout << "\n\033[1;32mAxiom ❯ \033[0m";
		std::string assistant_reply;
		auto t_start = std::chrono::steady_clock::now();
		auto t_first = t_start;
		bool first_token = true;
		size_t token_count = 0;

		ai.generate(conversation_history, 1024, current_temp, 40, 0.9f, [&](const std::string& tok) {
			if (first_token) {
				t_first = std::chrono::steady_clock::now();
				first_token = false;
			}
			++token_count;
			std::cout << tok << std::flush;
			assistant_reply += tok;
		});
		std::cout << "\n";

		auto t_end = std::chrono::steady_clock::now();
		double ttft_ms = std::chrono::duration<double, std::milli>(t_first - t_start).count();
		double gen_sec = std::chrono::duration<double>(t_end - t_first).count();
		double tps = (gen_sec > 0.001 && token_count > 1) ? ((token_count - 1) / gen_sec) : 0.0;

		std::cout << "\033[90m── [TTFT: " << std::fixed << std::setprecision(1) << ttft_ms << " ms"
			  << " | Speed: " << std::fixed << std::setprecision(1) << tps << " t/s"
			  << " | Generated: " << token_count << " tokens"
			  << " | Temp: " << std::fixed << std::setprecision(2) << current_temp
			  << "] ──\033[0m\n";

		conversation_history += assistant_reply + "<|im_end|>\n";

		if (conversation_history.size() > 12288) {
			size_t second_turn = conversation_history.find("<|im_start|>user", 64);
			if (second_turn != std::string::npos) {
				conversation_history =
					"<|im_start|>system\nYou are Project Axiom, an expert C++23, CUDA, and systems programming AI.<|im_end|>\n" +
					conversation_history.substr(second_turn);
			}
		}
	}
}

// *****************************************************************************
// Inference Engine ("Use it")
// *****************************************************************************
void run_inference_engine(AI& ai,
			  Parser& parser,
			  Input& input,
			  const std::filesystem::path& weights_path,
			  const std::filesystem::path& vocab_path) {
	std::cout << "\n========================================================\n"
		  << "             AXIOM INFERENCE ENGINE ('Use it')          \n"
		  << "========================================================\n";

	std::filesystem::path target_weights = weights_path;
	std::filesystem::path target_vocab = vocab_path;

	bool is_transformer = false;
	if (std::filesystem::exists(target_weights)) {
		std::ifstream probe(target_weights, std::ios::binary);
		if (probe.is_open()) {
			uint32_t magic = 0;
			probe.read(reinterpret_cast<char*>(&magic), sizeof(magic));
			if (magic == 0x4D4F4958) {
				is_transformer = true;
			}
		}
	} else if (std::filesystem::exists("models/qwen3b.axiom")) {
		target_weights = "models/qwen3b.axiom";
		target_vocab = std::filesystem::exists("models/qwen3b_vocab.axiom") ? "models/qwen3b_vocab.axiom" : "models/qwen_vocab.axiom";
		is_transformer = true;
	} else if (std::filesystem::exists("core/models/qwen3b.axiom")) {
		target_weights = "core/models/qwen3b.axiom";
		target_vocab = std::filesystem::exists("core/models/qwen3b_vocab.axiom") ? "core/models/qwen3b_vocab.axiom" : "core/models/qwen_vocab.axiom";
		is_transformer = true;
	}

	if (is_transformer) {
		std::cout << "[..] Detected Axiom Stacked Transformer (v2). Streaming weights via Win32 DMA mmap...\n";
		bool vocab_ok = ai.load_vocabulary(target_vocab.string());
		bool model_ok = ai.load_axiom_model_mmap(target_weights.string().c_str());
		if (vocab_ok && model_ok) {
			std::cout << "[OK] Stacked GQA Transformer weights loaded into VRAM.\n\n";
		} else {
			std::cerr << "[!] Failed to map transformer model weights.\n\n";
		}
	} else if (std::filesystem::exists(target_weights)) {
		std::cout << "[..] Loading model weights from: " << target_weights.filename().string()
			  << "\n";
		if (ai.load_weights(target_weights.string())) {
			std::cout << "[OK] Model weights successfully loaded into VRAM.\n\n";
		} else {
			std::cerr
				<< "[!] Failed to load model weights. Using baseline weights.\n\n";
		}
	} else if (target_weights.filename().string() != config::weights_file) {
		std::cerr << "[!] Specified weights file not found: " << target_weights.string()
			  << "\n"
			  << "[!] Running with baseline initialized weights.\n\n";
	} else {
		std::cout
			<< "[!] Notice: '" << target_weights.filename().string() << "' not found.\n"
			<< "[!] Running with baseline initialized weights.\n"
			<< "[!] Tip: Run 'app.exe --train' and press 'S' to save trained weights.\n\n";
	}

	if (!is_transformer) {
		if (std::filesystem::exists(target_vocab)) {
			std::cout << "[..] Loading vocabulary from: " << target_vocab.filename().string()
				  << "\n";
			if (parser.load_vocabulary(target_vocab, input)) {
				std::cout << "[OK] Vocabulary successfully restored ("
					  << input.main_dictionary.size() << " tokens).\n\n";
			}
		} else if (target_vocab.filename().string() != config::vocab_file) {
			std::cerr << "[!] Specified vocabulary file not found: " << target_vocab.string()
				  << "\n";
		}

		parser.build_embedding_table(input);
		if (input.next_text_id > 4) {
			ai.active_vocab_size = input.next_text_id;
		}
	}

	if (ai.has_transformer_model()) {
		std::cout << "Model Architecture: Stacked Grouped Query Attention (GQA) Transformer\n"
			  << "Layers            : " << ai.config_.num_layers << " stacked blocks\n"
			  << "Hidden Dimension  : " << ai.config_.hidden_dim << " (Q Heads: "
			  << ai.config_.num_q_heads << ", KV Heads: " << ai.config_.num_kv_heads << ")\n"
			  << "Context Window    : " << ai.config_.max_seq_len << " tokens\n"
			  << "Tensor Cores      : CUTLASS v4.8.0 Fused EVT (Ada Lovelace sm_89)\n"
			  << "Vocabulary Size   : " << ai.config_.vocab_size << " tokens (BPE exact code preserved)\n"
			  << "--------------------------------------------------------\n"
			  << "Enter code prompt or instruction (type 'exit' to quit):\n";
	} else {
		std::cout << "Model Architecture: Single Dedicated Multimodal Perceptron\n"
			  << "Context Window    : " << config::context_window_size << " tokens ("
			  << (config::context_window_size * config::token_dim * sizeof(std::bfloat16_t))
			  << " bytes BF16)\n"
			  << "Tensor Cores      : CUTLASS v4.8.0 Fused EVT (Ada Lovelace sm_89)\n"
			  << "Vocabulary Size   : " << input.embedding_table.size() << " tokens mapped\n"
			  << "--------------------------------------------------------\n"
			  << "Enter text or prompt to predict next token (type 'exit' to quit):\n";
	}

	// ****
	// Use
	// ****

	std::string line;

	while (true) {
		std::cout << "\nPrompt > ";
		if (!std::getline(std::cin, line)) {
			break;
		}

		strip_utf8_bom(line);

		while (_isatty(_fileno(stdin)) && _kbhit()) {

			std::string next_line;
			if (std::getline(std::cin, next_line)) {
				strip_utf8_bom(next_line);
				line += " " + next_line;
			} else {
				break;
			}
		}

		if (line.empty()) {
			continue;
		}

		if (line == "exit" || line == "quit" || line == ":q") {
			std::cout << "Exiting inference engine.\n";
			break;
		}

		if (ai.has_transformer_model()) {
			std::cout << "[Axiom 28-Layer GQA Stream]: ";
			ai.generate(line, 256, 0.2f, 40, 0.9f, [](const std::string& token_string) {
				std::cout << token_string << std::flush;
			});
			std::cout << "\n";
			continue;
		}

		// Read-only tokenization: unseen words become <UNK>, never a new untrained ID
		std::vector<uint32_t> tokens = parser.tokenize_for_inference(input, line);

		std::cout << "[Tokens] (" << tokens.size() << " tokens): ";
		for (size_t i = 0; i < tokens.size(); ++i) {
			std::cout << tokens[i] << (i + 1 < tokens.size() ? ", " : "\n");
		}

		// Right-aligned token ID window (PAD = 0 on the left), same layout as training
		std::vector<uint32_t> context_ids =
			parser.build_context_ids(tokens, config::context_window_size);

		AxiomTelemetry telemetry;
		telemetry.begin_inference();

		// Run forward prediction on Tensor Cores
		std::vector<float> prediction;
		if (!ai.predict_tokens(context_ids, prediction)) {
			std::cerr << "[!] Forward prediction failed.\n";
			continue;
		}
		telemetry.record_first_token();

		// Milestone 3: GPU Argmax decoding (bypasses CPU distance bottleneck)
		uint32_t predicted_token_id = ai.last_predicted_token_id;
		if (predicted_token_id == 0 && !input.embedding_table.empty()) {
			predicted_token_id = parser.decode_token(input, prediction);
		}
		std::string predicted_token_string =
			parser.token_to_string(input, predicted_token_id);
		telemetry.end_inference(ai.host_weights.size());

		std::cout << std::fixed << std::setprecision(4) << "[Tensor Output] ["
			  << prediction[0] << ", " << prediction[1] << ", " << prediction[2] << ", "
			  << prediction[3] << "]\n"
			  << "[Next Token ID] #" << predicted_token_id << "\n"
			  << "[Prediction]    \"" << predicted_token_string << "\"\n";

		std::cout << "[Autoregressive Stream]: ";
		ai.generate(line,
			    parser,
			    input,
			    128,
			    SamplingConfig {0.7f, 40, 0.9f},
			    [](const std::string& token_string) {
				    std::cout << token_string << " " << std::flush;
			    });
		std::cout << "\n";
	}
}

// *****************************************************************************
// Hardware Benchmark
// *****************************************************************************
void run_hardware_benchmark(AI& ai, Parser& parser, Input& input) {
	std::cout << "\n========================================================\n"
		  << "       AXIOM HARDWARE BENCHMARK & KERNEL SATURATION      \n"
		  << "========================================================\n";

	AxiomTelemetry telemetry;
	const HardwareSpecs& specs = telemetry.get_specs();

	std::cout << "Hardware Device : " << specs.gpu_name << "\n"
		  << "Architecture    : sm_" << specs.sm_major << specs.sm_minor
		  << " (Ada Lovelace)\n"
		  << "Memory Bus      : " << specs.bus_width_bits
		  << "-bit | Peak Bandwidth: " << std::fixed << std::setprecision(1)
		  << specs.peak_bandwidth_gb_s << " GB/s\n"
		  << "Compute Engine  : " << specs.peak_dense_tflops << " Peak Dense BF16 TFLOPs\n"
		  << "VRAM Footprint  : " << (specs.free_vram_bytes / (1024 * 1024))
		  << " MB Free / " << (specs.total_vram_bytes / (1024 * 1024)) << " MB Total\n"
		  << "--------------------------------------------------------\n"
		  << "[STAGE 1/3] Micro-Dispatch Latency & Decode Throughput Profile\n"
		  << "[..] Warming up Tensor Cores (10 forward passes)...\n";

	std::vector<std::bfloat16_t> dummy_ctx(config::context_window_size * config::token_dim,
					       static_cast<std::bfloat16_t>(0.05f));
	std::vector<float> pred;
	for (int i = 0; i < 10; ++i) {
		ai.predict(dummy_ctx, pred);
	}

	constexpr int bench_passes = 100;
	std::cout << "[..] Executing " << bench_passes
		  << " Hardware-Timed Tensor Core Inferences...\n";

	telemetry.begin_inference();
	for (int i = 0; i < bench_passes; ++i) {
		ai.predict(dummy_ctx, pred);
		if (i == 0) {
			telemetry.record_first_token();
		} else {
			telemetry.record_token();
		}
	}
	telemetry.end_inference(ai.host_weights.size(), "INFERENCE DISPATCH LATENCY");

	// Stage 2: High-Capacity VRAM Allocation & Memory Bus Saturation Stress
	std::cout << "--------------------------------------------------------\n"
		  << "[STAGE 2/3] High-Capacity VRAM Allocation & Memory Bus Saturation\n"
		  << "[..] Querying dynamic GPU memory constraints & safety margin...\n";

	constexpr size_t SAFETY_HEADROOM_BYTES =
		2048ULL * 1024 * 1024; // 2.0 GB Headroom for OS & DWM
	size_t vram_alloc_bytes = 0;
	double vram_bw_gb_s = 0.0;
	float vram_elapsed_ms = 0.0f;

	if (run_vram_saturation_benchmark(
		    SAFETY_HEADROOM_BYTES, &vram_alloc_bytes, &vram_bw_gb_s, &vram_elapsed_ms)) {
		double alloc_gb =
			static_cast<double>(vram_alloc_bytes) / (1024.0 * 1024.0 * 1024.0);
		double bw_pct = (specs.peak_bandwidth_gb_s > 0.0)
					? (vram_bw_gb_s / specs.peak_bandwidth_gb_s) * 100.0
					: 0.0;
		std::cout << "[OK] Dynamically allocated " << std::fixed << std::setprecision(2)
			  << alloc_gb
			  << " GB synthetic tensors in VRAM (leaving 2.00 GB OS safety headroom)\n"
			  << "[OK] Vectorized GDDR6 memory streaming bandwidth: "
			  << std::setprecision(1) << vram_bw_gb_s << " GB/s ("
			  << std::setprecision(1) << bw_pct << "% of physical bus)\n"
			  << "[OK] 100% memory integrity verified; VRAM cleanly deallocated.\n";
	} else {
		std::cout
			<< "[!] Insufficient free VRAM for massive allocation test (fallback to safe operational limit).\n";
	}

	// Stage 3: Ada Lovelace Tensor Core Saturation Sweep
	std::cout
		<< "--------------------------------------------------------\n"
		<< "[STAGE 3/3] Ada Lovelace Tensor Core Saturation Sweep\n"
		<< "            Configuration: M=2048, N=4096, K=4096 (68.7 GFLOPs/sweep)\n"
		<< "[..] Saturating all 58 Streaming Multiprocessors across 2,048 thread blocks...\n"
		<< "[..] Locking GPU into P0 Boost Clock and executing 1,000 sustained sweeps...\n"
		<< std::flush;

	constexpr int sat_M = 2048;
	constexpr int sat_N = 4096;
	constexpr int sat_K = 4096;
	constexpr int sat_iters = 1000;

	double sat_tflops = 0.0;
	double sat_bw = 0.0;
	float sat_avg_ms = 0.0f;

	if (run_cutlass_saturation_benchmark(
		    sat_M, sat_N, sat_K, sat_iters, &sat_tflops, &sat_bw, &sat_avg_ms)) {
		double best_bw = (vram_bw_gb_s > sat_bw) ? vram_bw_gb_s : sat_bw;
		float best_bw_pct =
			static_cast<float>((best_bw / specs.peak_bandwidth_gb_s) * 100.0);
		float sat_mfu_pct =
			static_cast<float>((sat_tflops / specs.peak_dense_tflops) * 100.0);
		std::string arch_tag =
			"sm_" + std::to_string(specs.sm_major) + std::to_string(specs.sm_minor);

		AxiomTelemetry::print_saturation_hud(sat_avg_ms,
						     sat_tflops,
						     best_bw,
						     best_bw_pct,
						     sat_mfu_pct,
						     vram_alloc_bytes,
						     SAFETY_HEADROOM_BYTES,
						     arch_tag);
	} else {
		std::cerr << "[!] CUTLASS saturation benchmark failed.\n";
	}
}

// *****************************************************************************
// Training loop
// *****************************************************************************
void run_training_loop(AI& ai,
		       Optimizer& optimizer,
		       Portion& portion,
		       Elements& elements,
		       Parser& parser,
		       const Input& input,
		       const std::filesystem::path& weights_path,
		       const std::filesystem::path& vocab_path,
		       size_t max_steps = 0) {
	size_t total_dataset_tokens = portion.ram_buffer_token_ids.size();
	std::error_code ec;
	uintmax_t file_bytes = std::filesystem::file_size(optimizer.data_path, ec);
	if (total_dataset_tokens == 0 && !ec && file_bytes > 0) {
		total_dataset_tokens = file_bytes / (portion.token_dim * sizeof(std::bfloat16_t));
	}
	size_t total_dataset_steps = (total_dataset_tokens > portion.context_window_size)
					     ? (total_dataset_tokens - portion.context_window_size)
					     : 0;

	size_t target_steps =
		(max_steps > 0)
			? std::min(max_steps,
				   total_dataset_steps > 0 ? total_dataset_steps : max_steps)
			: total_dataset_steps;

	if (target_steps == 0) {
		std::cerr << "[!] No training data available in "
			  << optimizer.data_path.filename().string() << "\n";
		return;
	}

	std::cout << "Beginning training session (" << target_steps
		  << " steps to 100% completion)...\n";
	std::cout << "Controls:\n"
		  << "  [S] Save current model weights to " << weights_path.filename().string()
		  << "\n"
		  << "  [R] Reset AI brain to chaotic state\n"
		  << "  [Q] Save and quit training loop\n\n"
		  << std::flush;

	// Steps trained since the start (or the last reset) and the first loss reported
	size_t step = 0;
	std::optional<double> first_loss;

	while (true) {
		// Check for interactive user commands without blocking
		char ch = 0;
		if (_kbhit()) {
			ch = _getch();
		} else if (!_isatty(_fileno(stdin))) {
			while (std::cin.rdbuf()->in_avail() > 0) {
				char c;
				if (std::cin.get(c)) {
					if (c == 'q' || c == 'Q' || c == 's' || c == 'S' ||
					    c == 'r' || c == 'R') {
						ch = c;
						break;
					}
				}
			}
			if (ch == 0) {
				HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
				DWORD bytesAvail = 0;
				if (PeekNamedPipe(hStdin, NULL, 0, NULL, &bytesAvail, NULL)) {
					if (bytesAvail > 0) {
						char c;
						if (std::cin.get(c)) {
							ch = c;
						}
					} else if (max_steps == 0 && step >= 20) {
						ch = 'q';
					}
				} else if (max_steps == 0 && step >= 20) {
					ch = 'q';
				}
			}
		}

		if (ch != 0) {
			if (ch == 'r' || ch == 'R') {
				std::cout << "\n[!] Manual Reset Triggered! Wiping Brain...\n";
				ai.reset_weights(elements);
				if (ai.upload_weights()) { // push new chaotic weights to VRAM
					step = 0;
					first_loss.reset();
					std::cout
						<< "[!] Brain wipe complete. Resuming training.\n\n";
				} else {
					std::cerr
						<< "[!] Brain wipe failed: could not upload weights.\n\n";
				}
			} else if (ch == 's' || ch == 'S') {
				std::cout
					<< "\n[!] Manual Checkpoint Triggered! Saving weights...\n";
				if (ai.save_weights(weights_path.string())) {
					parser.save_vocabulary(vocab_path, input);
					std::cout << "[OK] Model checkpoint saved to "
						  << weights_path.filename().string() << "\n\n";
				} else {
					std::cerr << "[!] Failed to save weights.\n\n";
				}
			} else if (ch == 'q' || ch == 'Q') {
				std::cout << "\n[!] Stopping training loop. Saving checkpoint...\n";
				ai.save_weights(weights_path.string());
				parser.save_vocabulary(vocab_path, input);
				std::cout << "[OK] Checkpoint saved. Exiting training.\n";
				break;
			}
		}

		// Feed forward & backward pass
		const TrainStatus status = ai.train(optimizer, portion);

		if (status == TrainStatus::Failed) {
			std::cerr << "\n[!] Training stopped: a GPU step failed.\n";
			std::exit(1);
		}

		if (status == TrainStatus::Trained) {
			++step;

			double current_loss = 0.0;
			if (step == 1 || step % config::loss_report_interval == 0 ||
			    step >= target_steps) {
				size_t loss_steps = 0;
				if (ai.read_loss(current_loss, loss_steps)) {
					if (!first_loss) {
						first_loss = current_loss;
					}

					const double pct =
						(target_steps > 0)
							? std::min(100.0,
								   (static_cast<double>(step) *
								    100.0 / target_steps))
							: 100.0;
					constexpr int bar_width = 24;
					int filled = static_cast<int>((pct / 100.0) * bar_width);
					if (filled > bar_width)
						filled = bar_width;
					std::string bar = "[";
					for (int b = 0; b < bar_width; ++b) {
						if (b < filled)
							bar += '=';
						else if (b == filled && filled < bar_width)
							bar += '>';
						else
							bar += ' ';
					}
					bar += "]";

					std::cout << "\r  [..] Learning " << bar << " "
						  << std::fixed << std::setprecision(1) << pct
						  << "% | Step " << step << "/" << target_steps
						  << " | Loss: " << std::setprecision(4)
						  << current_loss;
					if (*first_loss > 0.0) {
						double delta_pct = (current_loss - *first_loss) /
								   *first_loss * 100.0;
						std::cout << " (" << std::showpos
							  << std::setprecision(1) << delta_pct
							  << "% vs 1st)" << std::noshowpos;
					}
					std::cout << "   " << std::flush;
				}
			}

			if (step >= target_steps) {
				std::cout
					<< "\r  [OK] Learning [========================] 100.0% | Step "
					<< step << "/" << target_steps << " | Loss: " << std::fixed
					<< std::setprecision(4) << current_loss;
				if (first_loss && *first_loss > 0.0) {
					double delta_pct =
						(current_loss - *first_loss) / *first_loss * 100.0;
					std::cout << " (" << std::showpos << std::setprecision(1)
						  << delta_pct << "% vs 1st)" << std::noshowpos;
				}
				std::cout
					<< "\n\n[OK] 100% of dataset learned: all embedded data on disk consumed.\n"
					<< "[!] Automatically saving model checkpoint...\n"
					<< std::flush;

				ai.save_weights(weights_path.string());
				parser.save_vocabulary(vocab_path, input);
				std::cout << "[OK] Model checkpoint saved to "
					  << weights_path.filename().string() << "\n";
				break;
			}
		}

		// When the RAM buffer is exhausted, the offset resets to 0.
		// Pull the next massive chunk from disk into RAM.
		if (portion.offset == 0) {
			if (!portion.ram_buffer_token_ids.empty()) {
				// Completed pass over in-memory token IDs
				break;
			}
			optimizer.disk_to_ram(portion);
			if (portion.ram_buffer_tokens.size() <
			    portion.all_elements + portion.target_elements) {
				break;
			}
		}
	}
}

// After the tests, put the live brain back on its saved checkpoint, or into a
// pristine chaotic state when no usable checkpoint exists yet.
void restore_brain(AI& ai, Elements& elements, const std::filesystem::path& weights_path) {
	if (std::filesystem::exists(weights_path) && ai.load_weights(weights_path.string())) {
		std::cout << "[OK] Live brain restored from checkpoint: "
			  << weights_path.filename().string() << "\n";
		return;
	}

	ai.reset_weights(elements);
	if (ai.upload_weights()) {
		std::cout
			<< "[OK] No checkpoint found: live brain reset to a pristine chaotic state.\n";
	} else {
		std::cerr << "[!] Could not upload the reset weights to VRAM.\n";
	}
}

// embedded_path is the temporary file the tests generate; it is removed at the end.
bool run_tests(const std::filesystem::path& embedded_path) {
	bool all_ok = true;

	std::cout << "--- test_path AI ---\n";
	all_ok &= test_path();

	std::cout << "--- test_ifstream AI ---\n";
	all_ok &= test_ifstream();

	std::cout << "--- test_embeddings AI ---\n";
	all_ok &= test_embeddings();

	std::cout << "--- test_tokenization AI ---\n";
	all_ok &= test_tokenization();

	std::cout << "--- test_optimizer AI ---\n";
	all_ok &= test_optimizer();

	std::cout << "--- test_cuda_transfer AI ---\n";
	all_ok &= test_cuda_transfer();

	std::cout << "--- test_transfer AI ---\n";
	all_ok &= test_transfer();

	std::cout << "--- test_ai AI ---\n";
	all_ok &= test_ai();

	std::cout << "--- test_inference AI ---\n";
	all_ok &= test_inference();

	std::cout << "--- test_identifier AI ---\n";
	all_ok &= test_identifier();

	std::cout << "--- test_decompressor AI ---\n";
	all_ok &= test_decompressor();

	// The tests are done with the generated embedding stream
	std::error_code cleanup_ec;
	if (std::filesystem::remove(embedded_path, cleanup_ec)) {
		std::cout << "\n[OK] Test artifact removed: " << embedded_path.filename().string()
			  << "\n";
	}

	std::cout << "\n============================================\n";
	if (all_ok) {
		std::cout << "  ALL TEST SUITES PASSED (100% GREEN)       \n";
	} else {
		std::cerr << "  ONE OR MORE TEST SUITES FAILED            \n";
	}
	std::cout << "============================================\n\n";

	return all_ok;
}

// embedded_data.bin is only a bridge between the raw assets and the GPU. Once the
// session ends, the knowledge lives in the saved weights and vocabulary, so the
// bridge is deleted. Runs on every exit path; calling run() twice is harmless.
struct EmbeddedDataCleanup {
	Optimizer& optimizer;
	Portion& portion;
	const std::filesystem::path& path;

	void run() {
		// Windows cannot delete a file that is still open, and the next session
		// must start reading from the top of a fresh file.
		optimizer.close_stream();
		portion.ram_buffer_tokens.clear();
		portion.ram_buffer_token_ids.clear();
		portion.offset = 0;

		std::error_code ec;
		if (std::filesystem::remove(path, ec)) {
			std::cout << "[OK] Temporary embedding stream automatically removed: "
				  << path.filename().string() << "\n";
		}
	}

	~EmbeddedDataCleanup() {
		run();
	}
};

// Executes a full training pipeline session (interactive or CLI)
bool execute_training_session(AI& ai,
			      Optimizer& optimizer,
			      Portion& portion,
			      Elements& ai_elements,
			      Parser& parser,
			      Input& input,
			      Test& test,
			      const std::filesystem::path& weights_path,
			      const std::filesystem::path& vocab_path,
			      const std::filesystem::path& embedded_path,
			      const std::filesystem::path& asset_dir,
			      const std::filesystem::path& core_dir,
			      AppMode mode,
			      bool developer_mode,
			      size_t max_steps,
			      bool is_interactive,
			      bool auto_archive,
			      bool auto_purge,
			      uint32_t min_frequency = 3) {
	parser.developer_mode = developer_mode;

	if (std::filesystem::exists(vocab_path)) {
		std::cout << "[..] Pre-existing vocabulary found at "
			  << vocab_path.filename().string()
			  << ". Loading vocabulary checkpoint...\n";
		parser.load_vocabulary(vocab_path, input);
	}

	if (mode == AppMode::BulkTrain) {
		std::error_code ec;
		if (!std::filesystem::exists(asset_dir, ec)) {
			std::filesystem::create_directories(asset_dir, ec);
		}
		std::cout << "\n========================================================\n"
			  << "              AXIOM BULK TRAINING MODE                  \n"
			  << "========================================================\n"
			  << "Target Directory : " << asset_dir.string() << "\n\n";

		std::cout << "[..] Scanning and tokenizing all assets in "
			  << asset_dir.filename().string() << " (min frequency >= "
			  << min_frequency << ")...\n";
		const size_t processed = parser.bulk_tokenize_directory(asset_dir, input, 0, min_frequency);
		if (processed == 0 || input.tokenized_data.empty()) {
			std::cerr << "\n[!] No training files could be parsed from "
				  << asset_dir.string() << "\n"
				  << "[i] Instructions:\n"
				  << "    1. Place training files into the asset directory:\n"
				  << "       - Text (.txt, .md, .csv, code files)\n"
				  << "       - Images (.jpeg, .png, .bmp, .webp)\n"
				  << "       - Videos (.mp4, .mkv, .avi)\n"
				  << "       - Voice/Audio (.ogg, .wav, .mp3)\n"
				  << "       - Documents (.pdf) & 3D models (.obj, .stl, etc.)\n"
				  << "    2. Then run Bulk Training again.\n";
			return false;
		}
		std::cout << "\n[OK] Bulk ingestion complete: " << input.tokenized_data.size()
			  << " tokens collected across " << processed << " files.\n";
	} else {
		std::cout << "[..] Parsing and tokenizing multimodal data...\n";
		parser.tokenization(input, test);
		if (input.tokenized_data.empty()) {
			std::cerr << "[!] No data could be tokenized from the specified input.\n";
			return false;
		}
	}

	// Repeat sequence if shorter than one full context window + target to enable continuous training
	const size_t min_training_tokens = config::context_window_size + 1;
	if (input.tokenized_data.size() < min_training_tokens) {
		std::cout
			<< "[..] Dataset sequence (" << input.tokenized_data.size()
			<< " tokens) is shorter than context window ("
			<< config::context_window_size
			<< " tokens). Repeating sequence to enable continuous autoregressive training...\n";
		const size_t original_size = input.tokenized_data.size();
		const size_t target_tokens = config::context_window_size * 8;
		input.tokenized_data.reserve(target_tokens);
		while (input.tokenized_data.size() < target_tokens) {
			for (size_t i = 0;
			     i < original_size && input.tokenized_data.size() < target_tokens;
			     ++i) {
				const uint32_t token_value = input.tokenized_data[i];
				input.tokenized_data.push_back(token_value);
			}
		}
	}

	EmbeddedDataCleanup embedded_cleanup {optimizer, portion, embedded_path};

	// Multimodal pipeline preview: demonstrate transformation of initial data items
	const size_t preview_count = std::min<size_t>(3, input.tokenized_data.size());
	if (preview_count > 0) {
		parser.build_embedding_table(input);
		std::cout << "\n--- Multimodal Tokenization & Embedding Pipeline Preview ---\n";
		for (size_t i = 0; i < preview_count; ++i) {
			const uint32_t token_id = input.tokenized_data[i];
			const std::string raw_item =
				(developer_mode && i < test.test_input.size())
					? test.test_input[i]
					: parser.token_to_string(input, token_id);

			std::cout << "  \"" << raw_item << "\" assigned id: " << token_id
				  << " -> Tokenized into -> " << token_id
				  << " -> tokenized embedded into -> [";

			// Query actual master embedding table from AI VRAM/host state
			if (token_id < ai.vocab_size && !ai.host_embeddings.empty()) {
				for (size_t d = 0; d < ai.embedding_dim; ++d) {
					std::cout
						<< std::fixed << std::setprecision(4)
						<< ai.host_embeddings[token_id * ai.embedding_dim +
								      d]
						<< (d + 1 < ai.embedding_dim ? ", " : "");
				}
			} else {
				auto it = input.embedding_table.find(token_id);
				if (it != input.embedding_table.end()) {
					for (size_t d = 0; d < it->second.size(); ++d) {
						std::cout
							<< std::fixed << std::setprecision(4)
							<< static_cast<float>(it->second[d])
							<< (d + 1 < it->second.size() ? ", " : "");
					}
				}
			}
			std::cout << "]\n";
		}
		std::cout << "------------------------------------------------------------\n\n"
			  << std::defaultfloat << std::flush;
	}

	parser.save_vocabulary(vocab_path, input);
	std::cout << "[OK] Vocabulary prepared: "
		  << (input.main_dictionary.size() + input.image_dictionary.size() +
		      input.video_dictionary.size() + input.voice_dictionary.size())
		  << " tokens mapped.\n"
		  << std::flush;

	if (input.next_text_id > 4) {
		ai.active_vocab_size = input.next_text_id;
	}

	// Phase 4.1: Invert Host-to-Device Dataflow
	// Stream token IDs directly via portion.ram_buffer_token_ids for 512-byte PCIe transfers and CUDA gather
	portion.ram_buffer_token_ids = std::move(input.tokenized_data);
	portion.offset = 0;

	// Training Mode
	if (developer_mode) {
		std::cout << "--- Training AI ---\n" << std::flush;
	}

	std::cout << "[..] Allocating pinned memory and preparing PCIe token stream...\n"
		  << std::flush;
	if (!portion.allocate_pinned()) {
		std::cerr << "[!] Failed to allocate pinned host memory.\n";
		return false;
	}

	if (portion.ram_buffer_token_ids.size() < config::context_window_size + 1) {
		std::cerr << "[!] Not enough token data to fill one context window.\n";
		portion.free_pinned();
		return false;
	}

	// If pre-existing weights exist on disk, load them into VRAM before training
	if (std::filesystem::exists(weights_path)) {
		std::cout << "[..] Pre-existing weights found at "
			  << weights_path.filename().string() << ". Loading checkpoint...\n"
			  << std::flush;
		ai.load_weights(weights_path.string());
	}

	print_hardware_telemetry(portion);

	run_training_loop(ai,
			  optimizer,
			  portion,
			  ai_elements,
			  parser,
			  input,
			  weights_path,
			  vocab_path,
			  max_steps);

	portion.free_pinned();
	embedded_cleanup.run();

	if (mode == AppMode::BulkTrain) {
		std::cout << "\n========================================================\n"
			  << "          BULK TRAINING COMPLETED SUCCESSFULLY          \n"
			  << "========================================================\n"
			  << "[OK] Model weights safely saved to: "
			  << weights_path.filename().string() << "\n";

		const auto& bulk_files = parser.get_last_bulk_files();
		if (!bulk_files.empty() && (auto_archive || auto_purge)) {
			if (auto_archive) {
				const std::filesystem::path archive_dir = asset_dir / "archive";
				std::error_code ec;
				std::filesystem::create_directories(archive_dir, ec);
				size_t archived_count = 0;
				for (const auto& fpath : bulk_files) {
					if (std::filesystem::exists(fpath, ec)) {
						std::filesystem::path dest =
							archive_dir / fpath.filename();
						if (std::filesystem::exists(dest, ec)) {
							std::string stem = fpath.stem().string();
							std::string ext =
								fpath.extension().string();
							dest = archive_dir /
							       (stem + "_" +
								std::to_string(
									std::chrono::system_clock::now()
										.time_since_epoch()
										.count()) +
								ext);
						}
						std::filesystem::rename(fpath, dest, ec);
						if (ec) {
							ec.clear();
							std::filesystem::copy_file(
								fpath,
								dest,
								std::filesystem::copy_options::
									overwrite_existing,
								ec);
							if (!ec) {
								std::filesystem::remove(fpath, ec);
								++archived_count;
							}
						} else {
							++archived_count;
						}
					}
				}
				std::cout << "[OK] Automatically archived " << archived_count
					  << " file(s) to " << archive_dir.string() << "\n";
			} else if (auto_purge) {
				size_t purged_count = 0;
				for (const auto& fpath : bulk_files) {
					std::error_code ec;
					if (std::filesystem::remove(fpath, ec)) {
						++purged_count;
					}
				}
				std::cout << "[OK] Automatically purged " << purged_count
					  << " file(s) from " << asset_dir.string() << "\n";
			}
		} else if (is_interactive && !bulk_files.empty()) {
			std::cout << "\nRaw Asset Management (" << bulk_files.size()
				  << " source file" << (bulk_files.size() == 1 ? "" : "s") << " in "
				  << asset_dir.filename().string() << "/):\n"
				  << "  [1] Keep files in asset_sample/ (Default)\n"
				  << "  [2] Archive processed files to asset_sample/archive/\n"
				  << "  [3] Purge (Delete) processed files from asset_sample/\n"
				  << "> ";

			int cleanup_choice = read_choice();
			if (cleanup_choice <= 0 || cleanup_choice > 3) {
				cleanup_choice = 1;
			}

			if (cleanup_choice == 2) {
				const std::filesystem::path archive_dir =
					(core_dir / config::asset_folder) / "archive";
				std::error_code ec;
				std::filesystem::create_directories(archive_dir, ec);
				size_t archived_count = 0;
				for (const auto& fpath : bulk_files) {
					if (std::filesystem::exists(fpath, ec)) {
						std::filesystem::path dest =
							archive_dir / fpath.filename();
						if (std::filesystem::exists(dest, ec)) {
							std::string stem = fpath.stem().string();
							std::string ext =
								fpath.extension().string();
							dest = archive_dir /
							       (stem + "_" +
								std::to_string(
									std::chrono::system_clock::now()
										.time_since_epoch()
										.count()) +
								ext);
						}
						std::filesystem::rename(fpath, dest, ec);
						if (ec) {
							ec.clear();
							std::filesystem::copy_file(
								fpath,
								dest,
								std::filesystem::copy_options::
									overwrite_existing,
								ec);
							if (!ec) {
								std::filesystem::remove(fpath, ec);
								++archived_count;
							}
						} else {
							++archived_count;
						}
					}
				}
				std::cout << "[OK] Successfully archived " << archived_count
					  << " file(s) to " << archive_dir.string() << "\n";
			} else if (cleanup_choice == 3) {
				std::cout << "\n[!] WARNING: This will permanently delete "
					  << bulk_files.size() << " file(s) from "
					  << (core_dir / config::asset_folder).string() << "!\n"
					  << "Are you sure you want to proceed? (y/N): ";
				char confirm = 0;
				std::string confirm_line;
				if (std::getline(std::cin, confirm_line)) {
					strip_utf8_bom(confirm_line);
					size_t first = confirm_line.find_first_not_of(" \t\r\n");
					if (first != std::string::npos) {
						confirm = confirm_line[first];
					}
				}
				if (confirm == 'y' || confirm == 'Y') {
					size_t purged_count = 0;
					for (const auto& fpath : bulk_files) {
						std::error_code ec;
						if (std::filesystem::remove(fpath, ec)) {
							++purged_count;
						}
					}
					std::cout << "[OK] Successfully purged " << purged_count
						  << " file(s) from asset_sample/.\n";
				} else {
					std::cout
						<< "[i] Purge cancelled. Preserving all files in asset_sample/.\n";
				}
			} else {
				std::cout << "[i] Keeping all processed files in asset_sample/.\n";
			}
		} else {
			std::cout << "[i] You may now remove files from 'asset_sample',\n"
				  << "    add new training data, and run Bulk Training again\n"
				  << "    to continuously fine-tune the model.\n";
		}
		std::cout << "========================================================\n";
	}

	return true;
}

// main function
int main(int argc, char* argv[]) {
	std::ios::sync_with_stdio(false);
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif

	std::string primary_mode_arg;
	std::filesystem::path custom_vocab_path;
	std::filesystem::path custom_weights_path;
	std::filesystem::path custom_data_dir;
	float learning_rate = config::learning_rate;
	bool auto_archive = false;
	bool auto_purge = false;
	size_t max_steps = 0;
	uint32_t min_frequency = 3;

	for (int i = 1; i < argc; ++i) {
		std::string a = argv[i];
				if (a == "--serve") {
			uint16_t port = 8080;
			if (i + 1 < argc && argv[i + 1][0] != '-') {
				try {
					port = static_cast<uint16_t>(std::stoi(argv[++i]));
				} catch (...) {}
			}
			AI engine;
			std::filesystem::path vocab_p = "models/qwen3b_vocab.axiom";
			if (!std::filesystem::exists(vocab_p)) vocab_p = "models/qwen_vocab.axiom";
			std::filesystem::path weights_p = "models/qwen3b.axiom";
			if (!std::filesystem::exists(weights_p) && std::filesystem::exists("core/models/qwen3b.axiom")) {
				weights_p = "core/models/qwen3b.axiom";
			}
			if (!custom_vocab_path.empty()) vocab_p = custom_vocab_path;
			if (!custom_weights_path.empty()) weights_p = custom_weights_path;

			engine.load_vocabulary(vocab_p.string());
			engine.load_axiom_model_mmap(weights_p.string().c_str());
			run_axiom_openai_server(engine, port);
			return 0;
		}
		if (a == "--test") {
			return run_tests(locate_core_dir() / config::database_folder /
					 config::embedded_file)
				       ? 0
				       : 1;
		}
		if (a == "--help" || a == "-h") {
			std::cout
				<< "========================================================\n"
				<< "   PROJECT AXIOM v1.0 — Native C++23/CUDA AI Engine     \n"
				<< "========================================================\n"
				<< "Usage: app.exe [OPTION] [FLAGS]\n\n"
				<< "Execution Modes:\n"
				<< "  (no argument)       Interactive console menu\n"
				<< "  --chat              Speak directly with Axiom (Interactive Qwen 28-Layer GQA)\n"
				<< "  --serve [port]      Start native OpenAI-compatible chat server (default: 8080)\n"
				<< "  --use               Run direct forward inference ('Use it')\n"
				<< "  --train [N]         Train on sample data / prompt (optional N steps limit)\n"
				<< "  --bulk [N]          Bulk train on all files in asset directory (optional N steps)\n"
				<< "  --benchmark         Run hardware saturation & telemetry benchmark\n"
				<< "  --test              Run all automated test suites\n"
				<< "  --help, -h          Show this operator manual\n\n"
				<< "Configuration Flags:\n"
				<< "  --weights <path>    Specify custom neural weights checkpoint (default: weights.axiom)\n"
				<< "  --vocab <path>      Specify custom vocabulary dictionary (default: vocab.axiom)\n"
				<< "  --lr <rate>         Set optimizer learning rate (default: 0.001)\n"
				<< "  --data <dir>        Set custom bulk training directory (default: asset_sample)\n"
				<< "  --min-count <N>     Set minimum token frequency threshold for bulk ingestion (default: 3)\n"
				<< "  --archive           Automatically archive processed files after bulk training\n"
				<< "  --purge             Automatically purge processed files after bulk training\n\n";
			return 0;
		}
		if (a == "--weights") {
			if (i + 1 < argc) {
				custom_weights_path = argv[++i];
			} else {
				std::cerr << "Error: --weights requires a file path argument.\n";
				return 1;
			}
		} else if (a.rfind("--weights=", 0) == 0) {
			custom_weights_path = a.substr(10);
		} else if (a == "--vocab") {
			if (i + 1 < argc) {
				custom_vocab_path = argv[++i];
			} else {
				std::cerr << "Error: --vocab requires a file path argument.\n";
				return 1;
			}
		} else if (a.rfind("--vocab=", 0) == 0) {
			custom_vocab_path = a.substr(8);
		} else if (a == "--lr" || a == "--learning-rate") {
			if (i + 1 < argc) {
				try {
					learning_rate = std::stof(argv[++i]);
					if (learning_rate <= 0.0f)
						throw std::invalid_argument("non-positive");
				} catch (...) {
					std::cerr
						<< "Error: --lr requires a positive float value.\n";
					return 1;
				}
			} else {
				std::cerr << "Error: --lr requires a float value.\n";
				return 1;
			}
		} else if (a.rfind("--lr=", 0) == 0) {
			try {
				learning_rate = std::stof(a.substr(5));
				if (learning_rate <= 0.0f)
					throw std::invalid_argument("non-positive");
			} catch (...) {
				std::cerr << "Error: --lr requires a positive float value.\n";
				return 1;
			}
		} else if (a == "--data" || a == "--asset-dir") {
			if (i + 1 < argc) {
				custom_data_dir = argv[++i];
			} else {
				std::cerr << "Error: --data requires a directory path argument.\n";
				return 1;
			}
		} else if (a.rfind("--data=", 0) == 0) {
			custom_data_dir = a.substr(7);
		} else if (a.rfind("--asset-dir=", 0) == 0) {
			custom_data_dir = a.substr(12);
		} else if (a == "--min-count" || a == "--min-freq" || a == "--min-frequency") {
			if (i + 1 < argc) {
				try {
					min_frequency = static_cast<uint32_t>(std::stoul(argv[++i]));
				} catch (...) {
					std::cerr << "Error: --min-count requires an integer value.\n";
					return 1;
				}
			} else {
				std::cerr << "Error: --min-count requires an integer value.\n";
				return 1;
			}
		} else if (a.rfind("--min-count=", 0) == 0) {
			try {
				min_frequency = static_cast<uint32_t>(std::stoul(a.substr(12)));
			} catch (...) {
				std::cerr << "Error: --min-count requires an integer value.\n";
				return 1;
			}
		} else if (a == "--archive") {
			auto_archive = true;
		} else if (a == "--purge") {
			auto_purge = true;
		} else if (a == "--chat") {
			primary_mode_arg = "--chat";
		} else if (a == "--use" || a == "--benchmark") {
			primary_mode_arg = a;
		} else if (a == "--bulk" || a == "--train-bulk" || a == "--train") {
			primary_mode_arg = a;
			if (i + 1 < argc && argv[i + 1][0] != '-') {
				try {
					max_steps = std::stoull(argv[i + 1]);
					++i;
				} catch (...) {
					// Non-numeric argument, retain max_steps = 0
				}
			}
		} else {
			std::cerr
				<< "Unknown argument: " << a << "\n"
				<< "Usage: app.exe [--use | --train | --bulk | --benchmark | --test | --weights <path> | --vocab <path> | --lr <rate> | --data <dir> | --min-count <N> | --archive | --purge | --help]\n";
			return 1;
		}
	}

	const bool is_interactive = primary_mode_arg.empty() && custom_vocab_path.empty() &&
				    custom_weights_path.empty() && custom_data_dir.empty();

	AppMode mode = AppMode::Train;
	bool developer_mode = false;

	if (primary_mode_arg == "--chat") {
		mode = AppMode::Chat;
		developer_mode = true;
	} else if (primary_mode_arg == "--use") {
		mode = AppMode::Use;
		developer_mode = true;
	} else if (primary_mode_arg == "--benchmark") {
		mode = AppMode::Benchmark;
		developer_mode = true;
	} else if (primary_mode_arg == "--bulk" || primary_mode_arg == "--train-bulk") {
		mode = AppMode::BulkTrain;
		developer_mode = false;
	} else if (primary_mode_arg == "--train") {
		mode = AppMode::Train;
		developer_mode = true;
	} else if ((!custom_vocab_path.empty() || !custom_weights_path.empty()) &&
		   primary_mode_arg.empty()) {
		mode = AppMode::Use;
		developer_mode = true;
	}

	// Everything below is configured here and handed down to the backend
	const std::filesystem::path core_dir = locate_core_dir();
	const std::filesystem::path embedded_path =
		core_dir / config::database_folder / config::embedded_file;

	// Dedicated training directory (keeps training artifacts isolated from LLM models)
	const std::filesystem::path training_dir = config::training_folder;
	std::error_code dir_ec;
	std::filesystem::create_directories(training_dir, dir_ec);

	// 1. LLM Model Paths (strictly used by Chat and Server)
	std::filesystem::path llm_weights_path = "models/qwen3b.axiom";
	if (!std::filesystem::exists(llm_weights_path) && std::filesystem::exists(core_dir / "models" / "qwen3b.axiom")) {
		llm_weights_path = core_dir / "models" / "qwen3b.axiom";
	}
	if (!custom_weights_path.empty()) {
		llm_weights_path = custom_weights_path;
	}

	std::filesystem::path llm_vocab_path = "models/qwen3b_vocab.axiom";
	if (!std::filesystem::exists(llm_vocab_path) && std::filesystem::exists("models/qwen_vocab.axiom")) {
		llm_vocab_path = "models/qwen_vocab.axiom";
	} else if (!std::filesystem::exists(llm_vocab_path) && std::filesystem::exists(core_dir / "models" / "qwen3b_vocab.axiom")) {
		llm_vocab_path = core_dir / "models" / "qwen3b_vocab.axiom";
	}
	if (!custom_vocab_path.empty()) {
		llm_vocab_path = custom_vocab_path;
	}

	// 2. Training Paths (strictly used by Training sessions and Perceptron)
	std::filesystem::path training_weights_path = training_dir / config::weights_file;
	std::filesystem::path training_vocab_path = training_dir / config::vocab_file;
	if (!custom_weights_path.empty()) {
		training_weights_path = custom_weights_path;
	}
	if (!custom_vocab_path.empty()) {
		training_vocab_path = custom_vocab_path;
	}

	const std::filesystem::path asset_dir =
		!custom_data_dir.empty()
			? (std::filesystem::exists(custom_data_dir)
				   ? custom_data_dir
				   : (std::filesystem::exists(core_dir / custom_data_dir)
					      ? (core_dir / custom_data_dir)
					      : custom_data_dir))
			: (core_dir / config::asset_folder);

	const std::vector<std::filesystem::path> asset_dirs = {asset_dir};

	if (!is_interactive && mode == AppMode::Chat) {
		AI engine;
		run_chat_engine(engine, llm_weights_path, llm_vocab_path);
		return 0;
	}

	Parser parser(developer_mode, embedded_path, asset_dirs);
	Input input;
	input.embedding_dim = config::token_dim;
	Test test = make_sample_data();

	// Instantiate the required objects for the AI
	Portion portion(config::ram_buffer_size, config::token_dim, config::context_window_size);
	Optimizer optimizer(embedded_path, learning_rate);
	Elements ai_elements;
	AI ai(portion.all_elements,
	      portion.target_elements,
	      config::weight_min,
	      config::weight_max,
	      ai_elements);

	if (!ai.upload_weights()) {
		std::cerr << "[!] Failed to upload the AI weights to VRAM.\n";
		if (is_interactive) {
			pause_exit();
		}
		return 1;
	}

	if (!is_interactive) {
		if (mode == AppMode::Chat) {
			run_chat_engine(ai, llm_weights_path, llm_vocab_path);
			return 0;
		}
		if (mode == AppMode::Server) {
			run_axiom_openai_server(ai, 8080);
			return 0;
		}
		if (mode == AppMode::Test) {
			const bool tests_ok = run_tests(embedded_path);
			restore_brain(ai, ai_elements, training_weights_path);
			return tests_ok ? 0 : 1;
		}
		if (mode == AppMode::Use) {
			run_inference_engine(ai, parser, input, training_weights_path, training_vocab_path);
			return 0;
		}
		if (mode == AppMode::Benchmark) {
			run_hardware_benchmark(ai, parser, input);
			return 0;
		}
		if (mode == AppMode::Train || mode == AppMode::BulkTrain) {
			const bool ok = execute_training_session(ai,
								 optimizer,
								 portion,
								 ai_elements,
								 parser,
								 input,
								 test,
								 training_weights_path,
								 training_vocab_path,
								 embedded_path,
								 asset_dir,
								 core_dir,
								 mode,
								 developer_mode,
								 max_steps,
								 false,
								 auto_archive,
								 auto_purge,
								 min_frequency);
			return ok ? 0 : 1;
		}
		return 0;
	}

	// Interactive Menu Loop
	while (true) {
		Input session_input;
		session_input.embedding_dim = config::token_dim;
		AppMode selected_mode = collect_user_input(session_input);

		if (selected_mode == AppMode::Exit) {
			stop_background_axiom_server();
			break;
		}

		if (selected_mode == AppMode::Chat) {
			run_chat_engine(ai, llm_weights_path, llm_vocab_path);
			pause_menu();
			continue;
		}

		if (selected_mode == AppMode::Server) {
			if (is_background_axiom_server_running()) {
				std::cout << "\n=========================================\n"
					  << "      PROJECT AXIOM SERVER CONTROLLER    \n"
					  << "=========================================\n"
					  << "Status: Currently RUNNING on http://127.0.0.1:8080\n"
					  << "[1] Stop Background Server\n"
					  << "[2] Keep Running & Return to Menu\n"
					  << "> ";
				int s_choice = read_choice();
				if (s_choice == 1) {
					stop_background_axiom_server();
					std::cout << "\n[OK] Background server stopped cleanly.\n\n";
				}
			} else {
				std::cout << "\n=========================================\n"
					  << "      PROJECT AXIOM SERVER CONTROLLER    \n"
					  << "=========================================\n"
					  << "[1] Start Background Server (Recommended: chat in app while Cline/ComfyUI connect)\n"
					  << "[2] Start Foreground Server (Live stream request logs, press 'q' to stop)\n"
					  << "[3] Back to Main Menu\n"
					  << "> ";
				int s_choice = read_choice();
				if (s_choice == 1) {
					if (start_background_axiom_server(ai, 8080)) {
						std::cout << "\n[OK] Project Axiom server running in background on http://127.0.0.1:8080\n"
							  << "[OK] Cline and ComfyUI can now connect continuously!\n"
							  << "[OK] You can select Option [1] anytime to chat directly in this app.\n\n";
					}
				} else if (s_choice == 2) {
					run_axiom_openai_server(ai, 8080);
				}
			}
			pause_menu();
			continue;
		}

		if (selected_mode == AppMode::Test) {
			run_tests(embedded_path);
			restore_brain(ai, ai_elements, training_weights_path);
			pause_menu();
			continue;
		}

		if (selected_mode == AppMode::Benchmark) {
			run_hardware_benchmark(ai, parser, session_input);
			pause_menu();
			continue;
		}

		if (selected_mode == AppMode::Use) {
			run_inference_engine(ai, parser, session_input, training_weights_path, training_vocab_path);
			pause_menu();
			continue;
		}

		if (selected_mode == AppMode::Train || selected_mode == AppMode::BulkTrain) {
			Test empty_test;
			execute_training_session(ai,
						 optimizer,
						 portion,
						 ai_elements,
						 parser,
						 session_input,
						 empty_test,
						 training_weights_path,
						 training_vocab_path,
						 embedded_path,
						 asset_dir,
						 core_dir,
						 selected_mode,
						 false,
						 max_steps,
						 true,
						 auto_archive,
						 auto_purge);
			pause_menu();
			continue;
		}
	}

	stop_background_axiom_server();
	return 0;
}
