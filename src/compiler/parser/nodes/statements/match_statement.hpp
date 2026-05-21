#pragma once
#include <optional>
#include <string>

#include "block_statement.hpp"
#include "../expressions/expression.hpp"

struct MatchCase {
    std::string pattern_name_;
    std::optional<std::string> bound_variable_;
    std::unique_ptr<BlockStatement> body_;
};

class MatchStatement : public IStatement {
public:
    std::unique_ptr<IExpression> expression_;
    std::vector<MatchCase> match_cases_;

    explicit MatchStatement(
        std::unique_ptr<IExpression> expression,
        std::vector<MatchCase> match_cases,
        std::int32_t line,
        std::int32_t column) :
            expression_(std::move(expression)),
            match_cases_(std::move(match_cases)),
            IStatement(line, column) {}

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    virtual ~MatchStatement() = default;
};