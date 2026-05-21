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

class ClientDeclaration : public IDeclaration {
public:
    std::string name_;
    std::string host_;
    std::int32_t port_;
    std::vector<ClientRequestNode> requests_;

    explicit ClientDeclaration(
        std::int32_t line,
        std::int32_t column) :
            port_(0),
            IDeclaration(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~ClientDeclaration() = default;
};
