#include <iostream>

#pragma once

struct membuf : std::streambuf {
    membuf(char* begin, char* end, bool write = false);
};

bool wrapping_lt(uint32_t lhs, uint32_t rhs);

bool is_between_wrapped(uint32_t start, uint32_t x, uint32_t end);
