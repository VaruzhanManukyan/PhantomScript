#pragma once
#include <string>
#include <vector>
#include <cstdint>

#include "function_declaration.hpp"

struct ClientRequestNode {
    std::string http_method_;
    std::string path_;
    std::string local_function_name_;
    std::vector<FunctionParameter> parameters_;
    std::unique_ptr<TypeNode> return_type_;
};

struct ClientDeclaration : IDeclaration {
    std::string name_;
    std::string host_;
    std::int32_t port_;
    std::vector<ClientRequestNode> requests_;
};
