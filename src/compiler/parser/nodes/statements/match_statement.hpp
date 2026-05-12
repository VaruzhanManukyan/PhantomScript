#pragma once
#include <optional>
#include <string>

#include "block_statement.hpp"
#include "../expressions/expression.hpp"

struct MatchCase {
    std::string pattern_name_;
    std::optional<std::string> bound_variable_;
    std::optional<BlockStatement> body_;
};

struct MatchStatement : IStatement {
    std::unique_ptr<IExpression> expression_;
    std::vector<MatchCase> match_cases_;

    explicit MatchStatement(
        std::unique_ptr<IExpression> expression,
        std::vector<MatchCase> match_cases) :
            expression_(std::move(expression)),
            match_cases_(std::move(match_cases)) {}
    virtual ~MatchStatement() = default;
};