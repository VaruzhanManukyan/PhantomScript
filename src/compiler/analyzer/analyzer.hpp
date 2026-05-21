#pragma once
#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <unordered_map>

#include "ast_visitor.hpp"
#include "environment.hpp"
#include "exceptions/semantic_exception.hpp"

class SemanticAnalyzer : public IAstVisitor {
private:
    Environment env_;
    std::vector<SemanticException> errors_;
    std::shared_ptr<Type> current_expression_type_;
    std::shared_ptr<Type> current_function_return_type_;

    void report_error(const std::string& message, std::int32_t line = 1, std::int32_t column = 1);
    std::shared_ptr<Type> resolve_type_node(TypeNode* node);
    void check_cyclic_dependencies();
    bool dfs_cycle_check(
        const std::string& node,
        std::unordered_map<std::string, int>& state,
        const std::unordered_map<std::string, std::vector<std::string>>& graph
    );

public:
    SemanticAnalyzer() = default;

    bool analyze(ProgramNode& program);
    const std::vector<SemanticException>& get_errors() const { return errors_; }

    void visit(ProgramNode& node) override;
    void visit(ImportDeclaration& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(StructDeclaration& node) override;
    void visit(EnumDeclaration& node) override;
    void visit(DataTransferObjectDeclaration& node) override;
    void visit(QueryDeclaration& node) override;
    void visit(ClientDeclaration& node) override;
    void visit(ConsumerDeclaration& node) override;
    void visit(ServiceDeclaration& node) override;
    void visit(DatabaseDeclaration& node) override;

    void visit(ExpressionStatement& node) override;
    void visit(VariableDeclarationStatement& node) override;
    void visit(ReturnStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(MatchStatement& node) override;
    void visit(PrintStatement& node) override;
    void visit(BlockStatement& node) override;
    void visit(PublishStatement& node) override;

    void visit(AssignmentExpression& node) override;
    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(CallExpression& node) override;
    void visit(MemberAccessExpression& node) override;
    void visit(CastExpression& node) override;
    void visit(BoolLiteralExpression& node) override;
    void visit(NullLiteralExpression& node) override;
    void visit(IntLIteralExpression& node) override;
    void visit(DoubleLiteralExpression& node) override;
    void visit(StringLiteralExpression& node) override;
    void visit(IdentifierExpression& node) override;
    void visit(StructInstantiationExpression& node) override;
};