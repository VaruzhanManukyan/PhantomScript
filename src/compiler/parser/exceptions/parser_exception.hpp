#pragma once

#include <stdexcept>

struct ParseException : std::runtime_error {
    using std::runtime_error::runtime_error;
};