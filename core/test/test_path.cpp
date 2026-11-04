#include "test_header.hpp"
#include <filesystem>

bool test_path() {
	std::cout << "\n=== Path Resolution Test Suite ===\n";
	int s_pass = 0, s_fail = 0;

	std::filesystem::path core_dir = test_core_dir();
	test_check(std::filesystem::exists(core_dir), "Core directory exists", s_pass, s_fail);
	test_check(std::filesystem::is_directory(core_dir), "Core is valid directory", s_pass, s_fail);

	std::filesystem::path asset_dir = core_dir / "asset_sample";
	test_check(std::filesystem::exists(asset_dir), "asset_sample folder exists", s_pass, s_fail);

	std::filesystem::path models_dir = std::filesystem::exists("models") ? "models" : core_dir.parent_path() / "models";
	test_check(std::filesystem::exists(models_dir), "models folder exists", s_pass, s_fail);

	std::filesystem::path training_dir = std::filesystem::exists("training") ? "training" : core_dir.parent_path() / "training";
	test_check(std::filesystem::exists(training_dir), "training folder exists", s_pass, s_fail);

	std::cout << "=== Path Resolution Test Suite: " << s_pass << " passed, " << s_fail << " failed ===\n";
	return s_fail == 0;
}
