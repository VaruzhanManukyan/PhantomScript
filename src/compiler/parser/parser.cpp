#include <cassert>
#include <iostream>

#include "parser.hpp"
#include "exceptions/parser_exception.hpp"
#include "nodes/statements/expression_statement.hpp"
#include "nodes/statements/variable_declaration_statement.hpp"
#include "nodes/statements/return_statement.hpp"
#include "nodes/statements/if_statement.hpp"
#include "nodes/statements/for_statement.hpp"
#include "nodes/statements/while_statement.hpp"
#include "nodes/statements/for_statement.hpp"
#include "nodes/statements/for_in_statement.hpp"
#include "nodes/statements/break_statement.hpp"
#include "nodes/statements/continue_statement.hpp"
#include "nodes/statements/match_statement.hpp"
#include "nodes/statements/publish_statement.hpp"
#include "nodes/statements/print_statement.hpp"
#include "nodes/expressions/assignment_expression.hpp"
#include "nodes/expressions/binary_expression.hpp"
#include "nodes/expressions/unary_expression.hpp"
#include "nodes/expressions/call_expression.hpp"
#include "nodes/expressions/member_access_expression.hpp"
#include "nodes/expressions/index_access_expression.hpp"
#include "nodes/expressions/cast_expression.hpp"
#include "nodes/expressions/bool_literal_expression.hpp"
#include "nodes/expressions/null_literal_expression.hpp"
#include "nodes/expressions/int_literal_expression.hpp"
#include "nodes/expressions/double_literal_expression.hpp"
#include "nodes/expressions/string_literal_expression.hpp"
#include "nodes/expressions/array_literal_expression.hpp"
#include "nodes/expressions/identifier_expression.hpp"
#include "nodes/expressions/struct_instantiation_expression.hpp"

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

inline bool Parser::is_at_end() const {
    return peek().type == TokenType::END_OF_FILE;
}

inline const Token& Parser::peek() const {
    return tokens_[current_];
}

inline const Token& Parser::previous() const {
    assert(current_ > 0);
    return tokens_[current_ - 1];
}

inline const Token& Parser::advance() {
    if (!is_at_end()) {
        current_++;
    }
    return previous();
}

inline bool Parser::check(TokenType type) const {
    if (is_at_end()) {
        return false;
    }
    return peek().type == type;
}

inline bool Parser::match(std::initializer_list<TokenType> types) {
    for (const TokenType& type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

inline const Token& Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        return advance();
    }
    throw ParseException(message + " at line " + std::to_string(peek().line));
}

inline void Parser::synchronize() {
    advance();

    while (!is_at_end()) {
        if (previous().type == TokenType::SEMICOLON) {
            return;
        }

        switch (peek().type) {
            case TokenType::FN:
            case TokenType::STRUCT:
            case TokenType::ENUM:
            case TokenType::CONTRACT:
            case TokenType::EVENT:
            case TokenType::SERVICE:
            case TokenType::DATABASE:
            case TokenType::QUERY:
            case TokenType::CONSUMER:
            case TokenType::CLIENT:
            case TokenType::MUT:
            case TokenType::CONST:
            case TokenType::IF:
            case TokenType::RETURN:
            case TokenType::MATCH:
            case TokenType::PRINT:
                return;
            default:
                break;
        }

        advance();
    }
}

std::unique_ptr<ProgramNode> Parser::parse() {
    std::vector<std::unique_ptr<IDeclaration>> declarations;
    bool has_errors = false;

    while (!is_at_end()) {
        try {
            declarations.push_back(parse_declaration());
        }
        catch (const ParseException& error) {
            std::cerr << "[Syntax Error]: " << error.what() << '\n';
            has_errors = true;

            synchronize();
        }
    }

    return std::make_unique<ProgramNode>(std::move(declarations), 0, 0);
}

inline std::int32_t Parser::parse_int_literal(const Token& token) {
    try {
        return std::stoi(token.lexeme);
    }
    catch (const std::out_of_range&) {
        throw ParseException("Integer literal too large at line " + std::to_string(token.line));
    }
    catch (const std::invalid_argument&) {
        throw ParseException("Invalid integer literal at line " + std::to_string(token.line));
    }
}

inline double Parser::parse_double_literal(const Token& token) {
    try {
        return std::stod(token.lexeme);
    }
    catch (const std::out_of_range&) {
        throw ParseException("Double literal too large at line " + std::to_string(token.line));
    }
    catch (const std::invalid_argument&) {
        throw ParseException("Invalid double literal at line " + std::to_string(token.line));
    }
}

inline std::unique_ptr<TypeNode> Parser::parse_type() {
    if (!match({
        TokenType::IDENTIFIER, TokenType::INT, TokenType::DOUBLE,
        TokenType::STRING, TokenType::BOOL, TokenType::VOID,
        TokenType::OPTION, TokenType::RESULT, TokenType::ARRAY,
        TokenType::LIST, TokenType::MAP, TokenType::SET
    })) {
        throw ParseException("Expect type name at line " + std::to_string(peek().line) +
                             ", column " + std::to_string(peek().column));
    }

    const Token& base_type = previous();

    std::unique_ptr<TypeNode> type_node = std::make_unique<TypeNode>(
        base_type.lexeme,
        base_type.line,
        base_type.column
    );

    if (match({TokenType::LBRACKET})) {
        do {
            type_node->generics_.push_back(parse_type());
        }
        while (match({TokenType::COMMA}));
        consume(TokenType::RBRACKET, "Expect ']' after generic types.");
    }

    return type_node;
}

inline std::unique_ptr<IDeclaration> Parser::parse_declaration() {
    if (match({TokenType::IMPORT})) {
        return parse_import();
    }
    if (match({TokenType::FN})) {
        return parse_function();
    }
    if (match({TokenType::STRUCT})) {
        return parse_struct();
    }
    if (match({TokenType::ENUM})) {
        return parse_enum();
    }
    if (match({TokenType::CONTRACT})) {
        return parse_contract_or_event(false);
    }
    if (match({TokenType::EVENT})) {
        return parse_contract_or_event(true);
    }
    if (match({TokenType::SERVICE})) {
        return parse_service();
    }
    if (match({TokenType::DATABASE})) {
        return parse_database();
    }
    if (match({TokenType::QUERY})) {
        return parse_query();
    }
    if (match({TokenType::CONSUMER})) {
        return parse_consumer();
    }
    if (match({TokenType::CLIENT})) {
        return parse_client();
    }

    throw ParseException("Unexpected token at top level: '" + peek().lexeme +
        "' at line " + std::to_string(peek().line));
}

inline std::unique_ptr<ImportDeclaration> Parser::parse_import() {
    Token import_token = previous();

    Token path_token = consume(TokenType::STRING_LITERAL, "Expect string literal for import path.");
    consume(TokenType::SEMICOLON, "Expect ';' after import path.");

    return std::make_unique<ImportDeclaration>(
        path_token.lexeme,
        import_token.line,
        import_token.column
    );
}

inline std::unique_ptr<RouteNode> Parser::parse_route() {
    std::unique_ptr<RouteNode> route = std::make_unique<RouteNode>();

    if (!match({TokenType::GET, TokenType::POST, TokenType::PUT, TokenType::DELETE, TokenType::PATCH})) {
        throw ParseException("Expect HTTP method.");
    }
    const Token& method = previous();
    route->http_method_ = method.lexeme;

    route->path_ = consume(TokenType::STRING_LITERAL, "Expect route path string.").lexeme;
    consume(TokenType::ARROW, "Expect '->' mapping to handler.");
    route->handler_name_ = consume(TokenType::IDENTIFIER, "Expect handler function name.").lexeme;
    consume(TokenType::SEMICOLON, "Expect ';' after route.");

    return route;
}

inline std::unique_ptr<FunctionDeclaration> Parser::parse_function() {
    const Token& name = consume(TokenType::IDENTIFIER, "Expect function name.");
    consume(TokenType::LPAREN, "Expect '(' after function name.");

    std::vector<FunctionParameter> parameters;
    if (!check(TokenType::RPAREN)) {
        do {
            bool is_body = match({TokenType::BODY});
            const Token& param_name = consume(TokenType::IDENTIFIER, "Expect parameter name.");
            consume(TokenType::COLON, "Expect ':' after parameter name.");
            std::unique_ptr<TypeNode> param_type = parse_type();
            parameters.push_back({param_name.lexeme, std::move(param_type), is_body});
        }
        while (match({TokenType::COMMA}));
    }
    consume(TokenType::RPAREN, "Expect ')' after parameters.");

    consume(TokenType::ARROW, "Expect '->' before return type.");
    std::unique_ptr<TypeNode> return_type = parse_type();

    const Token& lbrace = consume(TokenType::LBRACE, "Expect '{' before function body.");
    std::unique_ptr<BlockStatement> body = std::make_unique<BlockStatement>(
        parse_block(), lbrace.line, lbrace.column);

    return std::make_unique<FunctionDeclaration>(
        name.lexeme,
        std::move(parameters),
        std::move(return_type),
        std::move(body),
        name.line,
        name.column
    );
}

inline std::unique_ptr<StructDeclaration> Parser::parse_struct() {
    const Token& name = consume(TokenType::IDENTIFIER, "Expect struct name.");
    consume(TokenType::LBRACE, "Expect '{' before struct body.");

    std::vector<StructField> fields;
    while (!check(TokenType::RBRACE) && !is_at_end()) {
        const Token& field_name = consume(TokenType::IDENTIFIER, "Expect field name.");
        consume(TokenType::COLON, "Expect ':' after field name.");
        std::unique_ptr<TypeNode> field_type = parse_type();
        consume(TokenType::SEMICOLON, "Expect ';' after field type.");

        fields.push_back({field_name.lexeme, std::move(field_type)});
    }

    consume(TokenType::RBRACE, "Expect '}' after struct body.");
    std::unique_ptr<StructDeclaration> struct_declaration = std::make_unique<StructDeclaration>(name.line, name.column);
    struct_declaration->name_ = name.lexeme;
    struct_declaration->fields_ = std::move(fields);

    return struct_declaration;
}

inline std::unique_ptr<EnumDeclaration> Parser::parse_enum() {
    const Token& name = consume(TokenType::IDENTIFIER, "Expect enum name.");
    consume(TokenType::LBRACE, "Expect '{' before enum body.");

    std::vector<std::string> variants;
    if (!check(TokenType::RBRACE)) {
        do {
            const Token& variant = consume(TokenType::IDENTIFIER, "Expect enum variant name.");
            variants.push_back(variant.lexeme);
        }
        while (match({TokenType::COMMA}) && !check(TokenType::RBRACE));
    }

    consume(TokenType::RBRACE, "Expect '}' after enum body.");

    std::unique_ptr<EnumDeclaration> enum_declaration = std::make_unique<EnumDeclaration>(name.line, name.column);
    enum_declaration->name_ = name.lexeme;
    enum_declaration->variants_ = std::move(variants);

    return enum_declaration;
}

inline std::unique_ptr<DataTransferObjectDeclaration> Parser::parse_contract_or_event(bool is_event) {
    const Token& name = consume(TokenType::IDENTIFIER, "Expect name.");
    consume(TokenType::LBRACE, "Expect '{' before block body.");

    std::vector<StructField> fields;
    while (!check(TokenType::RBRACE) && !is_at_end()) {
        Token field_name = consume(TokenType::IDENTIFIER, "Expect field name.");
        consume(TokenType::COLON, "Expect ':' after field name.");
        std::unique_ptr<TypeNode> field_type = parse_type();
        consume(TokenType::SEMICOLON, "Expect ';' after field type.");

        fields.push_back({field_name.lexeme, std::move(field_type)});
    }

    consume(TokenType::RBRACE, "Expect '}' after contract/event body.");

    std::unique_ptr<DataTransferObjectDeclaration> dto = std::make_unique<DataTransferObjectDeclaration>(name.line, name.column);
    dto->is_event_ = is_event;
    dto->name_ = name.lexeme;
    dto->fields_ = std::move(fields);

    return dto;
}

inline std::unique_ptr<QueryDeclaration> Parser::parse_query() {
    const Token& name = consume(TokenType::IDENTIFIER, "Expect query name.");
    consume(TokenType::LPAREN, "Expect '(' after query name.");

    std::vector<FunctionParameter> parameters;
    if (!check(TokenType::RPAREN)) {
        do {
            const Token& param_name = consume(TokenType::IDENTIFIER, "Expect parameter name.");
            consume(TokenType::COLON, "Expect ':' after parameter name.");
            std::unique_ptr<TypeNode> param_type = parse_type();
            parameters.push_back({param_name.lexeme, std::move(param_type), false});
        }
        while (match({TokenType::COMMA}));
    }
    consume(TokenType::RPAREN, "Expect ')' after parameters.");

    consume(TokenType::ARROW, "Expect '->' before return type.");
    std::unique_ptr<TypeNode> return_type = parse_type();

    consume(TokenType::LBRACE, "Expect '{' before query body.");
    consume(TokenType::SQL, "Expect 'sql' block inside query.");
    consume(TokenType::COLON, "Expect ':' after 'sql'.");

    std::string sql_query = "";
    while (!check(TokenType::SEMICOLON) && !check(TokenType::RBRACE) && !is_at_end()) {
        sql_query += advance().lexeme + " ";
    }
    if (match({TokenType::SEMICOLON})) {
        sql_query += ";";
    }

    consume(TokenType::RBRACE, "Expect '}' after query body.");
    std::unique_ptr<QueryDeclaration> query = std::make_unique<QueryDeclaration>(name.line, name.column);
    query->name_ = name.lexeme;
    query->parameters_ = std::move(parameters);
    query->return_type_ = std::move(return_type);
    query->sql_query_ = std::move(sql_query);
    return query;
}

inline std::unique_ptr<ClientDeclaration> Parser::parse_client() {
    const Token& name = consume(TokenType::IDENTIFIER, "Expect client name.");
    consume(TokenType::LBRACE, "Expect '{' before client body.");

    std::unique_ptr<ClientDeclaration> client = std::make_unique<ClientDeclaration>(name.line, name.column);
    client->name_ = name.lexeme;

    while (!check(TokenType::RBRACE) && !is_at_end()) {
        if (match({TokenType::IDENTIFIER}) && previous().lexeme == "request") {
            ClientRequestNode request;
            request.http_method_ = advance().lexeme;
            request.path_ = consume(TokenType::STRING_LITERAL, "Expect path string.").lexeme;

            consume(TokenType::ARROW, "Expect '->' after path.");
            request.local_function_name_ = consume(TokenType::IDENTIFIER, "Expect local function name.").lexeme;

            consume(TokenType::LPAREN, "Expect '(' after function name.");
            if (!check(TokenType::RPAREN)) {
                do {
                    Token param_name = consume(TokenType::IDENTIFIER, "Expect parameter name.");
                    consume(TokenType::COLON, "Expect ':' after parameter name.");
                    request.parameters_.push_back({param_name.lexeme, parse_type(), false});
                }
                while (match({TokenType::COMMA}));
            }
            consume(TokenType::RPAREN, "Expect ')' after parameters.");

            consume(TokenType::ARROW, "Expect '->' before return type.");
            request.return_type_ = parse_type();
            consume(TokenType::SEMICOLON, "Expect ';' after request declaration.");

            client->requests_.push_back(std::move(request));
        } else if (match({TokenType::IDENTIFIER})) {
            const Token& key = previous();
            consume(TokenType::COLON, "Expect ':' after key.");

            if (key.lexeme == "host") {
                client->host_ = consume(TokenType::STRING_LITERAL, "Expect string host.").lexeme;
            } else if (key.lexeme == "port") {
                client->port_ = parse_int_literal(consume(TokenType::INT_LITERAL, "Expect integer for port."));
            }

            consume(TokenType::SEMICOLON, "Expect ';'.");
        } else {
            throw ParseException("Unexpected token in client block: " + peek().lexeme);
        }
    }
    consume(TokenType::RBRACE, "Expect '}' after client body.");
    return client;
}

inline std::unique_ptr<ConsumerDeclaration> Parser::parse_consumer() {
    const Token& event_name = consume(TokenType::IDENTIFIER, "Expect event name after 'consumer'.");
    consume(TokenType::ARROW, "Expect '->' to bind consumer handler.");
    const Token& handler = consume(TokenType::IDENTIFIER, "Expect handler function name.");
    consume(TokenType::SEMICOLON, "Expect ';' after consumer declaration.");

    std::unique_ptr<ConsumerDeclaration> consumer = std::make_unique<ConsumerDeclaration>(event_name.line, event_name.column);
    consumer->event_name_ = event_name.lexeme;
    consumer->handler_function_name_ = handler.lexeme;
    return consumer;
}

inline std::unique_ptr<ServiceDeclaration> Parser::parse_service() {
    const Token& name = consume(TokenType::IDENTIFIER, "Expect service name.");
    consume(TokenType::LBRACE, "Expect '{' before service body.");

    std::unique_ptr<ServiceDeclaration> service = std::make_unique<ServiceDeclaration>(name.line, name.column);
    service->name_ = name.lexeme;

    while (!check(TokenType::RBRACE) && !is_at_end()) {
        if (match({TokenType::SERVER})) {
            consume(TokenType::LBRACE, "Expect '{' before server config.");
            while (!check(TokenType::RBRACE) && !is_at_end()) {
                Token key = consume(TokenType::IDENTIFIER, "Expect server config key (host, port).");
                consume(TokenType::COLON, "Expect ':' after key.");
                if (key.lexeme == "host") {
                    service->server_host_ = consume(TokenType::STRING_LITERAL, "Expect string host.").lexeme;
                } else if (key.lexeme == "port") {
                    service->server_port_ = parse_int_literal(consume(TokenType::INT_LITERAL, "Expect integer port."));
                }
                consume(TokenType::SEMICOLON, "Expect ';'.");
            }
            consume(TokenType::RBRACE, "Expect '}' after server config.");
        } else if (match({TokenType::DATABASE})) {
            consume(TokenType::IDENTIFIER, "Expect database name after 'database' inside service.");
            consume(TokenType::SEMICOLON, "Expect ';' after database reference.");
        } else if (match({TokenType::ROUTE})) {
            service->flat_routes_.push_back(parse_route());
        } else if (match({TokenType::GROUP})) {
            std::unique_ptr<GroupNode> group = std::make_unique<GroupNode>();
            group->base_name_ = consume(TokenType::STRING_LITERAL, "Expect base path for group.").lexeme;
            consume(TokenType::LBRACE, "Expect '{' before group routes.");

            while (!check(TokenType::RBRACE) && !is_at_end()) {
                consume(TokenType::ROUTE, "Expect 'route' inside group.");
                group->routes_.push_back(parse_route());
            }
            consume(TokenType::RBRACE, "Expect '}' after group routes.");
            service->route_groups_.push_back(std::move(group));
        } else if (match({TokenType::PUBLISH})) {
            const Token& event_name = consume(TokenType::IDENTIFIER, "Expect event name to publish.");
            consume(TokenType::SEMICOLON, "Expect ';' after publish declaration.");
            service->published_events_.push_back(event_name.lexeme);
        } else {
            throw ParseException("Unexpected token in service block: " + peek().lexeme);
        }
    }

    consume(TokenType::RBRACE, "Expect '}' after service body.");
    return service;
}

inline std::unique_ptr<DatabaseDeclaration> Parser::parse_database() {
    const Token& name = consume(TokenType::IDENTIFIER, "Expect database name.");
    consume(TokenType::LBRACE, "Expect '{' before database config.");

    std::unique_ptr<DatabaseDeclaration> database = std::make_unique<DatabaseDeclaration>(name.line, name.column);
    database->name_ = name.lexeme;

    while (!check(TokenType::RBRACE) && !is_at_end()) {
        const Token& key = consume(TokenType::IDENTIFIER, "Expect config key (engine, host, port, name).");
        consume(TokenType::COLON, "Expect ':' after config key.");

        if (key.lexeme == "engine") {
            database->engine_ = consume(TokenType::STRING_LITERAL, "Expect string for engine.").lexeme;
        } else if (key.lexeme == "host") {
            database->host_ = consume(TokenType::STRING_LITERAL, "Expect string for host.").lexeme;
        } else if (key.lexeme == "port") {
            database->port_ = parse_int_literal(consume(TokenType::INT_LITERAL, "Expect integer for port."));
        } else if (key.lexeme == "name") {
            database->db_name_ = consume(TokenType::STRING_LITERAL, "Expect string for db name.").lexeme;
        } else {
            throw ParseException("Unknown database config key: " + key.lexeme);
        }
        consume(TokenType::SEMICOLON, "Expect ';' after config value.");
    }

    consume(TokenType::RBRACE, "Expect '}' after database config.");
    return database;
}

inline std::unique_ptr<IStatement> Parser::parse_statement() {
    if (match({TokenType::MUT})) {
        return parse_variable_declaration(true);
    }
    if (match({TokenType::CONST})) {
        return parse_variable_declaration(false);
    }
    if (match({TokenType::IF})) {
        return parse_if_statement();
    }
    if (match({TokenType::WHILE})) {
        return parse_while_statement();
    }
    if (match({TokenType::FOR})) {
        if (check(TokenType::LPAREN)) {
            return parse_for_statement();
        }
        return parse_for_in_statement();
    }
    if (match({TokenType::BREAK})) {
        return parse_break_statement();
    }
    if (match({TokenType::CONTINUE})) {
        return parse_continue_statement();
    }
    if (match({TokenType::RETURN})) {
        return parse_return_statement();
    }
    if (match({TokenType::MATCH})) {
        return parse_match_statement();
    }
    if (match({TokenType::PRINT})) {
        return parse_print_statement();
    }
    if (match({TokenType::PUBLISH})) {
        return parse_publish_statement();
    }
    if (match({TokenType::LBRACE})) {
        const Token& lbrace = previous();
        return std::make_unique<BlockStatement>(parse_block(), lbrace.line, lbrace.column);
    }

    std::unique_ptr<IExpression> expression = parse_expression();
    const std::int32_t expr_line   = expression->line_;
    const std::int32_t expr_column = expression->column_;
    consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    return std::make_unique<ExpressionStatement>(std::move(expression), expr_line, expr_column);
}

inline std::vector<std::unique_ptr<IStatement>> Parser::parse_block() {
    std::vector<std::unique_ptr<IStatement>> statements;
    while (!check(TokenType::RBRACE) && !is_at_end()) {
        statements.push_back(parse_statement());
    }
    consume(TokenType::RBRACE, "Expect '}' after block.");
    return statements;
}

inline std::unique_ptr<IStatement> Parser::parse_variable_declaration(bool is_mutable) {
    const Token& name = consume(TokenType::IDENTIFIER, "Expect variable name.");
    consume(TokenType::COLON, "Expect ':' after variable name.");
    std::unique_ptr<TypeNode> type = parse_type();

    std::unique_ptr<IExpression> initializer = nullptr;
    if (match({TokenType::EQ})) {
        initializer = parse_expression();
    }

    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");
    return std::make_unique<VariableDeclarationStatement>(
        is_mutable,
        name.lexeme,
        std::move(type),
        std::move(initializer),
        name.line,
        name.column);
}

inline std::unique_ptr<IStatement> Parser::parse_return_statement() {
    const Token& keyword = previous();

    std::unique_ptr<IExpression> value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = parse_expression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after return value.");

    std::unique_ptr<ReturnStatement> statement = std::make_unique<ReturnStatement>(keyword.line, keyword.column);
    statement->expression_ = std::move(value);
    return statement;
}

inline std::unique_ptr<IStatement> Parser::parse_if_statement() {
    const Token& keyword = previous();

    consume(TokenType::LPAREN, "Expect '(' after 'if'.");
    std::unique_ptr<IExpression> condition = parse_expression();
    consume(TokenType::RPAREN, "Expect ')' after if condition.");

    const Token& then_brace = consume(TokenType::LBRACE, "Expect '{' before if body.");
    std::unique_ptr<BlockStatement> than_branch = std::make_unique<BlockStatement>(
        parse_block(), then_brace.line, then_brace.column);

    std::unique_ptr<IStatement> else_branch = nullptr;
    if (match({TokenType::ELSE})) {
        if (match({TokenType::IF})) {
            else_branch = parse_if_statement();
        } else {
            const Token& else_brace = consume(TokenType::LBRACE, "Expect '{' before else body.");
            else_branch = std::make_unique<BlockStatement>(
                parse_block(), else_brace.line, else_brace.column);
        }
    }

    return std::make_unique<IfStatement>(
        std::move(condition),
        std::move(than_branch),
        std::move(else_branch),
        keyword.line,
        keyword.column);
}

std::unique_ptr<IStatement> Parser::parse_while_statement() {
    const Token& keyword = previous();

    consume(TokenType::LPAREN, "Expect '(' after 'while'.");
    std::unique_ptr<IExpression> condition = parse_expression();
    consume(TokenType::RPAREN, "Expect ')' after while condition.");

    const Token& lbrace = consume(TokenType::LBRACE, "Expect '{' before while body.");
    std::unique_ptr<BlockStatement> body = std::make_unique<BlockStatement>(
        parse_block(), lbrace.line, lbrace.column);

    return std::make_unique<WhileStatement>(
        std::move(condition),
        std::move(body),
        keyword.line, keyword.column);
}

std::unique_ptr<IStatement> Parser::parse_for_statement() {
    const Token& keyword = previous();
    consume(TokenType::LPAREN, "Expect '(' after 'for'.");

    std::unique_ptr<IStatement> init = nullptr;
    if (match({TokenType::MUT})) {
        init = parse_variable_declaration(true);
    } else if (match({TokenType::CONST})) {
        init = parse_variable_declaration(false);
    } else if (!check(TokenType::SEMICOLON)) {
        auto expr = parse_expression();
        const std::int32_t el = expr->line_, ec = expr->column_;
        consume(TokenType::SEMICOLON, "Expect ';' after for init expression.");
        init = std::make_unique<ExpressionStatement>(std::move(expr), el, ec);
    } else {
        consume(TokenType::SEMICOLON, "Expect ';' in for loop.");
    }

    std::unique_ptr<IExpression> condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        condition = parse_expression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after for condition.");

    std::unique_ptr<IExpression> increment = nullptr;
    if (!check(TokenType::RPAREN)) {
        increment = parse_expression();
    }
    consume(TokenType::RPAREN, "Expect ')' after for clauses.");

    const Token& lbrace = consume(TokenType::LBRACE, "Expect '{' before for body.");
    std::unique_ptr<BlockStatement> body = std::make_unique<BlockStatement>(
        parse_block(), lbrace.line, lbrace.column);

    return std::make_unique<ForStatement>(
        std::move(init),
        std::move(condition),
        std::move(increment),
        std::move(body),
        keyword.line, keyword.column);
}

std::unique_ptr<IStatement> Parser::parse_for_in_statement() {
    const Token& keyword = previous();

    const Token& var = consume(TokenType::IDENTIFIER, "Expect variable name after 'for'.");
    consume(TokenType::IN, "Expect 'in' after loop variable.");
    std::unique_ptr<IExpression> iterable = parse_expression();

    const Token& lbrace = consume(TokenType::LBRACE, "Expect '{' before for body.");
    std::unique_ptr<BlockStatement> body = std::make_unique<BlockStatement>(
        parse_block(), lbrace.line, lbrace.column);

    return std::make_unique<ForInStatement>(
        var.lexeme,
        std::move(iterable),
        std::move(body),
        keyword.line, keyword.column);
}

inline std::unique_ptr<IStatement> Parser::parse_break_statement() {
    const Token& keyword = previous();
    consume(TokenType::SEMICOLON, "Expect ';' after 'break'.");
    return std::make_unique<BreakStatement>(keyword.line, keyword.column);
}

inline std::unique_ptr<IStatement> Parser::parse_continue_statement() {
    const Token& keyword = previous();
    consume(TokenType::SEMICOLON, "Expect ';' after 'continue'.");
    return std::make_unique<ContinueStatement>(keyword.line, keyword.column);
}

inline std::unique_ptr<IStatement> Parser::parse_match_statement() {
    const Token& keyword = previous();

    std::unique_ptr<IExpression> expression = parse_expression();
    consume(TokenType::LBRACE, "Expect '{' before match cases.");

    std::vector<MatchCase> cases;
    while (!check(TokenType::RBRACE) && !is_at_end()) {
        if (!match({TokenType::IDENTIFIER, TokenType::OK, TokenType::ERR, TokenType::SOME})) {
            throw ParseException("Expect match pattern at line " + std::to_string(peek().line));
        }

        std::string pattern_name = previous().lexeme;
        if (match({TokenType::DOT})) {
            const Token& variant = consume(TokenType::IDENTIFIER, "Expect enum variant after '.'.");
            pattern_name += "." + variant.lexeme;
        }

        std::optional<std::string> bound_variable = std::nullopt;
        if (match({TokenType::LPAREN})) {
            bound_variable = consume(TokenType::IDENTIFIER, "Expect bound variable name.").lexeme;
            consume(TokenType::RPAREN, "Expect ')' after bound variable.");
        }

        consume(TokenType::FAT_ARROW, "Expect '=>' after match pattern.");
        const Token& case_brace = consume(TokenType::LBRACE, "Expect '{' before case body.");
        std::unique_ptr<BlockStatement> body = std::make_unique<BlockStatement>(
            parse_block(), case_brace.line, case_brace.column);

        cases.push_back({pattern_name, bound_variable, std::move(body)});
    }

    consume(TokenType::RBRACE, "Expect '}' after match cases.");
    return std::make_unique<MatchStatement>(
        std::move(expression),
        std::move(cases),
        keyword.line,
        keyword.column);
}

inline std::unique_ptr<IStatement> Parser::parse_publish_statement() {
    const Token& keyword = previous();

    const Token& event_name = consume(TokenType::IDENTIFIER, "Expect event name after 'publish'.");
    consume(TokenType::LBRACE, "Expect '{' before publish body.");

    std::vector<StructInstantiationExpression::FieldInit> fields;
    while (!check(TokenType::RBRACE) && !is_at_end()) {
        Token field_name = consume(TokenType::IDENTIFIER, "Expect field name.");
        consume(TokenType::COLON, "Expect ':' after field name.");

        std::unique_ptr<IExpression> value = parse_expression();
        consume(TokenType::SEMICOLON, "Expect ';' after field value.");

        fields.push_back({field_name.lexeme, std::move(value)});
    }

    consume(TokenType::RBRACE, "Expect '}' after publish body.");
    consume(TokenType::SEMICOLON, "Expect ';' after publish statement.");

    return std::make_unique<PublishStatement>(
        event_name.lexeme,
        std::move(fields),
        keyword.line,
        keyword.column);
}

inline std::unique_ptr<IStatement> Parser::parse_print_statement() {
    const Token& keyword = previous();

    std::unique_ptr<IExpression> value = parse_expression();
    consume(TokenType::SEMICOLON, "Expect ';' after print expression.");
    return std::make_unique<PrintStatement>(
        std::move(value),
        keyword.line,
        keyword.column);
}

inline std::unique_ptr<IExpression> Parser::parse_expression() {
    return parse_assignment();
}

inline std::unique_ptr<IExpression> Parser::parse_assignment() {
    std::unique_ptr<IExpression> expression = parse_logical_or();

    if (match({
        TokenType::EQ, TokenType::PLUS_EQ, TokenType::MINUS_EQ,
        TokenType::STAR_EQ, TokenType::SLASH_EQ, TokenType::PERCENT_EQ
    })) {
        const Token& equals = previous();
        std::unique_ptr<IExpression> value = parse_assignment();
        return std::make_unique<AssignmentExpression>(
            std::move(expression),
            equals.lexeme,
            std::move(value),
            equals.line,
            equals.column);
    }

    return expression;
}

inline std::unique_ptr<IExpression> Parser::parse_logical_or() {
    std::unique_ptr<IExpression> expression = parse_logical_and();
    while (match({TokenType::OR_OR})) {
        const Token& operator_ = previous();
        std::unique_ptr<IExpression> right = parse_logical_and();
        expression = std::make_unique<BinaryExpression>(
            std::move(expression),
            operator_.lexeme,
            std::move(right),
            operator_.line,
            operator_.column);
    }
    return expression;
}

inline std::unique_ptr<IExpression> Parser::parse_logical_and() {
    std::unique_ptr<IExpression> expression = parse_equality();
    while (match({TokenType::AND_AND})) {
        const Token& operator_ = previous();
        std::unique_ptr<IExpression> right = parse_equality();
        expression = std::make_unique<BinaryExpression>(
            std::move(expression),
            operator_.lexeme,
            std::move(right),
            operator_.line,
            operator_.column);
    }
    return expression;
}

inline std::unique_ptr<IExpression> Parser::parse_equality() {
    std::unique_ptr<IExpression> expression = parse_comparison();
    while (match({TokenType::EQ_EQ, TokenType::NG_EQ})) {
        const Token& operator_ = previous();
        std::unique_ptr<IExpression> right = parse_comparison();
        expression = std::make_unique<BinaryExpression>(
            std::move(expression),
            operator_.lexeme,
            std::move(right),
            operator_.line,
            operator_.column);
    }
    return expression;
}

inline std::unique_ptr<IExpression> Parser::parse_comparison() {
    std::unique_ptr<IExpression> expression = parse_term();
    while (match({TokenType::GT, TokenType::GT_EQ, TokenType::LT, TokenType::LT_EQ})) {
        const Token& operator_ = previous();
        std::unique_ptr<IExpression> right = parse_term();
        expression = std::make_unique<BinaryExpression>(
            std::move(expression),
            operator_.lexeme,
            std::move(right),
            operator_.line,
            operator_.column);
    }
    return expression;
}

inline std::unique_ptr<IExpression> Parser::parse_term() {
    std::unique_ptr<IExpression> expression = parse_factor();
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        const Token& operator_ = previous();
        std::unique_ptr<IExpression> right = parse_factor();
        expression = std::make_unique<BinaryExpression>(
            std::move(expression),
            operator_.lexeme,
            std::move(right),
            operator_.line,
            operator_.column);
    }
    return expression;
}

inline std::unique_ptr<IExpression> Parser::parse_factor() {
    std::unique_ptr<IExpression> expression = parse_unary();
    while (match({TokenType::STAR, TokenType::SLASH, TokenType::PERCENT})) {
        const Token& operator_ = previous();
        std::unique_ptr<IExpression> right = parse_unary();
        expression = std::make_unique<BinaryExpression>(
            std::move(expression),
            operator_.lexeme,
            std::move(right),
            operator_.line,
            operator_.column);
    }
    return expression;
}

inline std::unique_ptr<IExpression> Parser::parse_unary() {
    if (match({TokenType::NG, TokenType::MINUS})) {
        const Token& operator_ = previous();
        std::unique_ptr<IExpression> right = parse_unary();
        return std::make_unique<UnaryExpression>(
            operator_.lexeme,
            std::move(right),
            operator_.line,
            operator_.column);
    }
    return parse_call();
}

inline std::unique_ptr<IExpression> Parser::parse_call() {
    std::unique_ptr<IExpression> expression = parse_primary();

    while (true) {
        if (match({TokenType::LPAREN})) {
            const Token& lparen = previous();
            std::vector<std::unique_ptr<IExpression>> args;
            if (!check(TokenType::RPAREN)) {
                do {
                    args.push_back(parse_expression());
                }
                while (match({TokenType::COMMA}));
            }
            consume(TokenType::RPAREN, "Expect ')' after arguments.");
            expression = std::make_unique<CallExpression>(
                std::move(expression),
                std::move(args),
                lparen.line,
                lparen.column);
        } else if (match({TokenType::DOT})) {
            const Token& name = consume(TokenType::IDENTIFIER, "Expect property name after '.'.");
            expression = std::make_unique<MemberAccessExpression>(
                std::move(expression),
                name.lexeme,
                name.line,
                name.column);
        } else if (match({TokenType::AS})) {
            const Token& as_token = previous();
            std::unique_ptr<TypeNode> target_type = parse_type();
            expression = std::make_unique<CastExpression>(
                std::move(expression),
                std::move(target_type),
                as_token.line,
                as_token.column);
        } else if (match({TokenType::LBRACKET})) {
            const Token& lbracket = previous();
            std::unique_ptr<IExpression> index = parse_expression();
            consume(TokenType::RBRACKET, "Expect ']' after index expression.");
            expression = std::make_unique<IndexAccessExpression>(
                std::move(expression),
                std::move(index),
                lbracket.line,
                lbracket.column);
        } else {
            break;
        }
    }

    return expression;
}

inline std::unique_ptr<IExpression> Parser::parse_primary() {
    if (match({TokenType::BOOL_LITERAL})) {
        const Token& token = previous();
        return std::make_unique<BoolLiteralExpression>(
            token.lexeme == "true",
            token.line,
            token.column);
    }
    if (match({TokenType::NULL_LITERAL})) {
        const Token& token = previous();
        return std::make_unique<NullLiteralExpression>(
            token.line,
            token.column);
    }
    if (match({TokenType::INT_LITERAL})) {
        const Token& token = previous();
        return std::make_unique<IntLIteralExpression>(
            parse_int_literal(token),
            token.line,
            token.column);
    }
    if (match({TokenType::DOUBLE_LITERAL})) {
        const Token& token = previous();
        return std::make_unique<DoubleLiteralExpression>(
            parse_double_literal(token),
            token.line,
            token.column);
    }
    if (match({TokenType::STRING_LITERAL})) {
        const Token& token = previous();
        return std::make_unique<StringLiteralExpression>(
            token.lexeme,
            token.line,
            token.column);
    }
    if (match({TokenType::IDENTIFIER})) {
        const Token& ident_token = previous();
        if (check(TokenType::LBRACE)) {
            return parse_struct_instantiation(ident_token.lexeme, ident_token.line, ident_token.column);
        }
        return std::make_unique<IdentifierExpression>(
            ident_token.lexeme,
            ident_token.line,
            ident_token.column);
    }
    if (match({TokenType::LBRACKET})) {
        const Token& lbracket = previous();
        std::vector<std::unique_ptr<IExpression>> elements;

        if (!check(TokenType::RBRACKET)) {
            do {
                elements.push_back(parse_expression());
            } while (match({TokenType::COMMA}));
        }

        consume(TokenType::RBRACKET, "Expect ']' after array elements.");
        return std::make_unique<ArrayLiteralExpression>(
            std::move(elements),
            lbracket.line,
            lbracket.column);
    }
    if (match({TokenType::LPAREN})) {
        std::unique_ptr<IExpression> expression = parse_expression();
        consume(TokenType::RPAREN, "Expect ')' after expression.");
        return expression;
    }

    throw ParseException("Expect expression at line " + std::to_string(peek().line));
}

inline std::unique_ptr<IExpression> Parser::parse_struct_instantiation(const std::string& struct_name, std::int32_t line, std::int32_t column) {
    consume(TokenType::LBRACE, "Expect '{' before struct instantiation body.");
    std::vector<StructInstantiationExpression::FieldInit> fields;

    while (!check(TokenType::RBRACE) && !is_at_end()) {
        const Token& field_name = consume(TokenType::IDENTIFIER, "Expect field name.");
        consume(TokenType::COLON, "Expect ':' after field name.");

        std::unique_ptr<IExpression> value = parse_expression();
        consume(TokenType::SEMICOLON, "Expect ';' after field value.");

        fields.push_back({field_name.lexeme, std::move(value)});
    }

    consume(TokenType::RBRACE, "Expect '}' after struct instantiation body.");
    return std::make_unique<StructInstantiationExpression>(struct_name, std::move(fields), line, column);
}