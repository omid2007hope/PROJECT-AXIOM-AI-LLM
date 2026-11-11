#
# Copyright (c) 2026 Omid Teimory. All Rights Reserved.
#
# NOTICE: This file contains proprietary source code licensed exclusively under the
# terms of the LICENSE file in the root directory of this repository.
# You may view, download, and compile this code solely to run the unmodified product.
# Copying, pasting, modifying, redistributing, creating derivative works, or using
# this code in any other project or AI training system is strictly prohibited.
# Removal or alteration of this notice is a violation of 17 U.S.C. § 1202 and EU law.
#

mkdir test_dir\src
mkdir test_dir\include
echo #pragma once > test_dir\include\hdr.hpp
echo #include ^<iostream^> >> test_dir\include\hdr.hpp
echo inline void p() { std::cout ^<^< __FILE__ ^<^< std::endl; } >> test_dir\include\hdr.hpp
echo #include "../include/hdr.hpp" > test_dir\src\main.cpp
echo int main() { p(); return 0; } >> test_dir\src\main.cpp
g++ -Itest_dir\include test_dir\src\main.cpp -o test_dir\test_path.exe
.\test_dir\test_path.exe
