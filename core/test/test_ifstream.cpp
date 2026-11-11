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
#include <fstream>
#include <iostream>

bool test_ifstream() {
	std::cout << "\n=== Binary Asset Stream Test Suite ===\n";
	int s_pass = 0, s_fail = 0;

	Config c;
	const std::vector<std::string> assets = {"Photo.jpeg", "Video.mp4", "Voice.ogg"};
	const std::filesystem::path asset_dir = test_core_dir() / c.asset_folder;

	for (const auto& file_name : assets) {
		std::filesystem::path p = asset_dir / file_name;
		if (!std::filesystem::exists(p)) {
			std::filesystem::path fb1 = test_core_dir() / "test_sandbox_assets" / file_name;
			std::filesystem::path fb2 = test_core_dir() / "training_data" / "Data" / file_name;
			if (std::filesystem::exists(fb1)) p = fb1;
			else if (std::filesystem::exists(fb2)) p = fb2;
		}
		std::ifstream f(p, std::ios::binary | std::ios::ate);
		bool opened = f.is_open();
		test_check(opened, (file_name + " opened successfully").c_str(), s_pass, s_fail);

		if (opened) {
			size_t sz = f.tellg();
			test_check(sz > 0, (file_name + " file size > 0 bytes").c_str(), s_pass, s_fail);
		}
	}

	std::cout << "=== Binary Asset Stream Test Suite: " << s_pass << " passed, " << s_fail << " failed ===\n";
	return s_fail == 0;
}
