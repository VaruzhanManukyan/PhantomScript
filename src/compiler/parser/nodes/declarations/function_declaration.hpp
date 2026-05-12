#pragma once
#include <string>
#include <memory>

#include "declaration.hpp"
#include "../types/type_node.hpp"
#include "../statements/block_statement.hpp"

struct FunctionParameter {
    std::string name_;
    std::unique_ptr<TypeNode> type_;
    bool is_body_parameter_;
};

struct FunctionDeclaration : IDeclaration {
    std::string name_;
    std::vector<FunctionParameter> parameters_;
    std::unique_ptr<TypeNode> return_type_;
    std::unique_ptr<BlockStatement> body_;

    explicit FunctionDeclaration(
        std::string name,
        std::vector<FunctionDeclaration> parameters,
        std::unique_ptr<TypeNode> return_type,
        std::unique_ptr<BlockStatement> body) :
            name_(std::move(name)),
            parameters_(std::move(parameters)),
            return_type_(std::move(return_type)),
            body_(std::move(body)) {}
    virtual ~FunctionDeclaration() = default;
};