// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell
#include <stdexcept>
#include <util.h>

static void
require(bool value)
{
    if (!value)
        throw std::runtime_error("GLSL vector boundary regression");
}
int
main()
{
    glsl::vector<int> values;
    require(values.begin() == values.end());
    const auto& constant = values;
    require(constant.begin() == constant.end());
    require(values.erase(values.begin(), values.end()) == values.end());
    values.insert(values.begin(), 7);
    require(values.front() == 7 && values.back() == 7 && constant.back() == 7);
    require(values.end() == values.begin() + 1 && constant.end() == constant.begin() + 1);
    values.push_back(9);
    require(values.back() == 9);
    require(values.erase(values.begin()) == values.begin() && values.front() == 9);
    require(values.erase(values.begin()) == values.end() && values.empty());
    int input[] = { 1, 2, 3 };
    values.insert(values.begin(), input, input + 3);
    require(values.size() == 3 && values.back() == 3);
    require(values.erase(values.begin() + 1, values.end()) == values.end());
    require(values.size() == 1 && values.front() == 1);
    require(values.erase(values.begin(), values.end()) == values.end() && values.empty());
    values.reserve(8);
    require(values.begin() == values.end());
    values.push_back(42);
    values.pop_back();
    require(values.begin() == values.end());
}
