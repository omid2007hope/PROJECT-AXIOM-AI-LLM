/*
 * Project Axiom — Built from scratch. Engineered for the future.
 * Copyright (c) 2026 Omid Teimory. All rights reserved.
 */

#include "test_header.hpp"
#include "../include/config.hpp"
#include <filesystem>

bool test_path() {
	std::cout << "\n=== Path Resolution Test Suite ===\n";
	int s_pass = 0, s_fail = 0;

	Config c;
	std::filesystem::path core_dir = test_core_dir();
	test_check(std::filesystem::exists(core_dir), "Core directory exists", s_pass, s_fail);
	test_check(std::filesystem::is_directory(core_dir), "Core is valid directory", s_pass, s_fail);

	std::filesystem::path asset_dir = core_dir / c.asset_folder;
	test_check(std::filesystem::exists(asset_dir), (std::string(c.asset_folder) + " folder exists").c_str(), s_pass, s_fail);

	std::filesystem::path models_dir = std::filesystem::exists(c.models_folder) ? std::filesystem::path(c.models_folder) : core_dir.parent_path() / c.models_folder;
	test_check(std::filesystem::exists(models_dir), (std::string(c.models_folder) + " folder exists").c_str(), s_pass, s_fail);

	std::filesystem::path training_dir = std::filesystem::exists(c.training_folder) ? std::filesystem::path(c.training_folder) : core_dir.parent_path() / c.training_folder;
	test_check(std::filesystem::exists(training_dir), (std::string(c.training_folder) + " folder exists").c_str(), s_pass, s_fail);

	std::cout << "=== Path Resolution Test Suite: " << s_pass << " passed, " << s_fail << " failed ===\n";
	return s_fail == 0;
}
