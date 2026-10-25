#!/bin/bash

conan install . --build=missing -pr=conanprofile.txt -s build_type=Debug
cmake --preset conan-debug
cmake --build --preset conan-debug
