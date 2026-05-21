#pragma once

class TypeNode;
class ProgramNode;
class ImportDeclaration;
class FunctionDeclaration;
class StructDeclaration;
class EnumDeclaration;
class DataTransferObjectDeclaration;
class QueryDeclaration;
class ClientDeclaration;
class ConsumerDeclaration;
class ServiceDeclaration;
class DatabaseDeclaration;

class ExpressionStatement;
class VariableDeclarationStatement;
class ReturnStatement;
class IfStatement;
class MatchStatement;
class PrintStatement;
class BlockStatement;
class PublishStatement;

class AssignmentExpression;
class BinaryExpression;
class UnaryExpression;
class CallExpression;
class MemberAccessExpression;
class CastExpression;
class BoolLiteralExpression;
class NullLiteralExpression;
class IntLIteralExpression;
class DoubleLiteralExpression;
class StringLiteralExpression;
class IdentifierExpression;
class StructInstantiationExpression;

class IAstVisitor {
public:
    virtual ~IAstVisitor() = default;

    virtual void visit(ProgramNode& node) = 0;
    virtual void visit(ImportDeclaration& node) = 0;
    virtual void visit(FunctionDeclaration& node) = 0;
    virtual void visit(StructDeclaration& node) = 0;
    virtual void visit(EnumDeclaration& node) = 0;
    virtual void visit(DataTransferObjectDeclaration& node) = 0;
    virtual void visit(QueryDeclaration& node) = 0;
    virtual void visit(ClientDeclaration& node) = 0;
    virtual void visit(ConsumerDeclaration& node) = 0;
    virtual void visit(ServiceDeclaration& node) = 0;
    virtual void visit(DatabaseDeclaration& node) = 0;

    virtual void visit(ExpressionStatement& node) = 0;
    virtual void visit(VariableDeclarationStatement& node) = 0;
    virtual void visit(ReturnStatement& node) = 0;
    virtual void visit(IfStatement& node) = 0;
    virtual void visit(MatchStatement& node) = 0;
    virtual void visit(PrintStatement& node) = 0;
    virtual void visit(BlockStatement& node) = 0;
    virtual void visit(PublishStatement& node) = 0;

    virtual void visit(AssignmentExpression& node) = 0;
    virtual void visit(BinaryExpression& node) = 0;
    virtual void visit(UnaryExpression& node) = 0;
    virtual void visit(CallExpression& node) = 0;
    virtual void visit(MemberAccessExpression& node) = 0;
    virtual void visit(CastExpression& node) = 0;
    virtual void visit(BoolLiteralExpression& node) = 0;
    virtual void visit(NullLiteralExpression& node) = 0;
    virtual void visit(IntLIteralExpression& node) = 0;
    virtual void visit(DoubleLiteralExpression& node) = 0;
    virtual void visit(StringLiteralExpression& node) = 0;
    virtual void visit(IdentifierExpression& node) = 0;
    virtual void visit(StructInstantiationExpression& node) = 0;
};