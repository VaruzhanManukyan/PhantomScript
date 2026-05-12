#pragma once
#include <string>

#include "declaration.hpp"

struct ConsumerDeclaration : IDeclaration {
    std::string event_name_;
    std::string handler_function_name_;
};
