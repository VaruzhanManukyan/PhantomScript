#pragma once
#include <memory>
#include <vector>

#include "../token/token.hpp"
#include "nodes/expressions/expression.hpp"
#include "nodes/declarations/declaration.hpp"
#include "nodes/declarations/import_declaration.hpp"
#include "nodes/declarations/function_declaration.hpp"
#include "nodes/declarations/struct_declaration.hpp"
#include "nodes/declarations/enum_declaration.hpp"
#include "nodes/declarations/data_transfer_object_declaration.hpp"
#include "nodes/declarations/service_declaration.hpp"
#include "nodes/declarations/query_declaration.hpp"
#include "nodes/declarations/consumer_declaration.hpp"
#include "nodes/declarations/client_declaration.hpp"
#include "nodes/statements/statement.hpp"
#include "nodes/declarations/program_node.hpp"

class Parser {
private:
    std::vector<Token> tokens_;
    std::size_t current_ = 0;

    bool is_at_end() const;
    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(std::initializer_list<TokenType> types);
    const Token& consume(TokenType type, const std::string& message);
    void synchronize();

    std::int32_t parse_int_literal(const Token& token);
    double parse_double_literal(const Token& token);

    std::unique_ptr<TypeNode> parse_type();

    std::unique_ptr<IDeclaration> parse_declaration();
    std::unique_ptr<ImportDeclaration> parse_import();
    std::unique_ptr<RouteNode> parse_route();
    std::unique_ptr<FunctionDeclaration> parse_function();
    std::unique_ptr<StructDeclaration> parse_struct();
    std::unique_ptr<EnumDeclaration> parse_enum();
    std::unique_ptr<DataTransferObjectDeclaration> parse_contract_or_event(bool is_event);
    std::unique_ptr<QueryDeclaration> parse_query();
    std::unique_ptr<ClientDeclaration> parse_client();
    std::unique_ptr<ConsumerDeclaration> parse_consumer();
    std::unique_ptr<ServiceDeclaration> parse_service();
    std::unique_ptr<DatabaseDeclaration> parse_database();

    std::unique_ptr<IStatement> parse_statement();
    std::vector<std::unique_ptr<IStatement>> parse_block();
    std::unique_ptr<IStatement> parse_variable_declaration(bool is_mutable);
    std::unique_ptr<IStatement> parse_return_statement();
    std::unique_ptr<IStatement> parse_if_statement();
    std::unique_ptr<IStatement> parse_while_statement();
    std::unique_ptr<IStatement> parse_for_statement();
    std::unique_ptr<IStatement> parse_for_in_statement();
    std::unique_ptr<IStatement> parse_break_statement();
    std::unique_ptr<IStatement> parse_continue_statement();
    std::unique_ptr<IStatement> parse_match_statement();
    std::unique_ptr<IStatement> parse_print_statement();
    std::unique_ptr<IStatement> parse_publish_statement();

    std::unique_ptr<IExpression> parse_expression();
    std::unique_ptr<IExpression> parse_assignment();
    std::unique_ptr<IExpression> parse_logical_or();
    std::unique_ptr<IExpression> parse_logical_and();
    std::unique_ptr<IExpression> parse_equality();
    std::unique_ptr<IExpression> parse_comparison();
    std::unique_ptr<IExpression> parse_term();
    std::unique_ptr<IExpression> parse_factor();
    std::unique_ptr<IExpression> parse_unary();
    std::unique_ptr<IExpression> parse_call();
    std::unique_ptr<IExpression> parse_primary();
    std::unique_ptr<IExpression> parse_struct_instantiation(
        const std::string& struct_name,
        std::int32_t line,
        std::int32_t column);
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<ProgramNode> parse();
};
