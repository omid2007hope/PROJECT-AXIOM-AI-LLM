/*
 * Copyright (c) 2026 Omid Teimory. All Rights Reserved.
 *
 * NOTICE: This file contains proprietary source code licensed exclusively under the
 * terms of the LICENSE file in the root directory of this repository.
 * You may view, download, and compile this code solely to run the unmodified product.
 * Copying, pasting, modifying, redistributing, creating derivative works, or using
 * this code in any other project or AI training system is strictly prohibited.
 * Removal or alteration of this notice is a violation of 17 U.S.C. § 1202 and EU law.
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
