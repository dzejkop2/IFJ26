/**
 * @name ifj26
 * @authors Tomáš Zajac (xzajact00)
 * @file semantic.c
 */

#include <stdio.h>
// #include <stdlib.h>
#include "semantic.h"
#include "error.h"
#include "ast.h"
#include "scanner.h"

static void analyzeNode(astNode *node);
static void analyzeBinaryOp(astNode *node);
static void analyzeUnaryOp(astNode *node);
static void analyzeExpr(astNode *node);

static void analyzeNodeList(astNodeList nodeList)
{
    for (size_t i = 0; i < nodeList.count; i++)
    {
        analyzeNode(nodeList.items[i]);
    }
}

void semanticAnalyze(astNode *root)
{
    if (root == NULL || root->type != AST_PROGRAM)
    {
        error(INTERNAL_ERROR, "Root node is not a program");
    }
    analyzeNode(root);
}

static void analyzeNode(astNode *node)
{
    if (node == NULL)
    {
        return;
    }

    switch (node->type)
    {
    case AST_PROGRAM:
        analyzeNodeList(node->data.program.defs);
        break;

    case AST_FUN_DEF:
        analyzeNode(node->data.funDef.body);
        break;
    case AST_EXTENSION:
        analyzeNodeList(node->data.extension.funDefs);

        break;
    case AST_PARAM_LIST:
        analyzeNodeList(node->data.paramList.params);
        break;

    case AST_PARAM:
        // No specific analysis needed for parameters
        break;

    case AST_BLOCK:
        analyzeNodeList(node->data.block.stmts);
        break;
    case AST_VAR_DECL:
        analyzeNode(node->data.varDecl.value);
        break;
    case AST_ASSIGN:
        analyzeNode(node->data.assign.target);
        analyzeNode(node->data.assign.value);
        break;
    case AST_RETURN:
        analyzeNode(node->data.ret.value);
        break;

    case AST_WHILE:
        analyzeExpr(node->data.whileLoop.cond);
        analyzeNode(node->data.whileLoop.body);
        break;
    case AST_FOR:
        analyzeNode(node->data.forLoop.from);
        analyzeNode(node->data.forLoop.to);
        analyzeNode(node->data.forLoop.step);
        analyzeNode(node->data.forLoop.body);
        analyzeNodeList(node->data.forLoop.guards);
        break;
    case AST_IF:
        analyzeExpr(node->data.ifStmt.cond);
        analyzeNode(node->data.ifStmt.thenBody);
        analyzeNode(node->data.ifStmt.elseBody);
        break;
    case AST_EXPR_STMT:
        analyzeNode(node->data.exprStmt.expr);
        break;
    case AST_LIT_INT:
        node->dataType = TYPE_INT;
        break;
    case AST_LIT_DOUBLE:
        node->dataType = TYPE_DOUBLE;
        break;
    case AST_LIT_STRING:
        node->dataType = TYPE_STRING;
        break;
    case AST_LIT_BOOL:
        node->dataType = TYPE_BOOL;
        break;
    case AST_LIT_UNIT:
        node->dataType = TYPE_UNIT;
        break;
    case AST_IDENTIFIER:
        // No specific analysis needed for literals and identifiers
        break;
    case AST_BINARY_OP:
    case AST_UNARY_OP:
        analyzeExpr(node);
        break;
    case AST_CALL:
        analyzeNode(node->data.call.callee);
        analyzeNodeList(node->data.call.args);
        break;
    case AST_MEMBER:
        analyzeNode(node->data.member.object);
        break;
    default:
        error(INTERNAL_ERROR, "Unknown AST node type");
        break;
    }
}

static void analyzeExpr(astNode *node)
{
    if (node == NULL)
    {
        return;
    }

    switch (node->type)
    {
    case AST_BINARY_OP:
        analyzeExpr(node->data.binary.left);
        analyzeExpr(node->data.binary.right);
        analyzeBinaryOp(node);
        break;
    case AST_UNARY_OP:
        analyzeExpr(node->data.unary.operand);
        analyzeUnaryOp(node);
        break;
    case AST_CALL:
        analyzeExpr(node->data.call.callee);
        analyzeNodeList(node->data.call.args);
        break;
    case AST_MEMBER:
        analyzeExpr(node->data.member.object);
        break;
    case AST_LIT_INT:

        node->dataType = TYPE_INT;
        break;
    case AST_LIT_DOUBLE:
        node->dataType = TYPE_DOUBLE;
        break;
    case AST_LIT_STRING:
        node->dataType = TYPE_STRING;
        break;
    case AST_LIT_BOOL:
        node->dataType = TYPE_BOOL;
        break;
    case AST_LIT_UNIT:
        node->dataType = TYPE_UNIT;
        break;
    default:
        // No specific analysis needed for other expression types
        break;
    }
}

static void analyzeBinaryOp(astNode *node)
{
    if (node == NULL || node->type != AST_BINARY_OP)
    {
        error(INTERNAL_ERROR, "Invalid binary operation");
        return;
    }

    analyzeExpr(node->data.binary.left);
    analyzeExpr(node->data.binary.right);

    astDataType leftType = node->data.binary.left->dataType;
    astDataType rightType = node->data.binary.right->dataType;
    switch (node->data.binary.op)
    {
    case op_add:
        if (leftType == TYPE_STRING && rightType == TYPE_STRING)
        {
            node->dataType = TYPE_STRING;
        }
        else if (leftType == TYPE_INT && rightType == TYPE_INT)
        {
            node->dataType = TYPE_INT;
        }
        else if ((leftType == TYPE_DOUBLE && rightType == TYPE_DOUBLE) ||
                 (leftType == TYPE_DOUBLE && rightType == TYPE_INT) ||
                 (leftType == TYPE_INT && rightType == TYPE_DOUBLE))
        {
            node->dataType = TYPE_DOUBLE;
        }
        else
        {
            error(SEMANTIC_TYPE_ERROR, "Invalid types for addition");
        }
        break;
    case op_sub:
    case op_mul:
    case op_div:
        if (leftType == TYPE_INT && rightType == TYPE_INT)
        {
            node->dataType = TYPE_INT;
        }
        else if ((leftType == TYPE_DOUBLE && rightType == TYPE_DOUBLE) ||
                 (leftType == TYPE_DOUBLE && rightType == TYPE_INT) ||
                 (leftType == TYPE_INT && rightType == TYPE_DOUBLE))
        {
            node->dataType = TYPE_DOUBLE;
        }
        else
        {
            error(SEMANTIC_TYPE_ERROR, "Invalid types for arithmetic operation");
        }
        break;

    case op_equal:
    case op_notEqual:
        if (((leftType == TYPE_INT || leftType == TYPE_DOUBLE) &&
             (rightType == TYPE_INT || rightType == TYPE_DOUBLE)) ||
            leftType == rightType)
        {
            node->dataType = TYPE_BOOL;
        }
        else
        {
            error(SEMANTIC_TYPE_ERROR, "Invalid types for comparison operation");
        }
        break;
    case op_biggerThen:
    case op_lowerThen:
    case op_lowerEqual:
    case op_biggerEqual:
        if ((leftType == TYPE_INT || leftType == TYPE_DOUBLE) &&
            (rightType == TYPE_INT || rightType == TYPE_DOUBLE))
        {
            node->dataType = TYPE_BOOL;
        }
        else
        {
            error(SEMANTIC_TYPE_ERROR, "Invalid types for comparison operation");
        }
        break;
    case op_and:
    case op_or:
        if (leftType == TYPE_BOOL && rightType == TYPE_BOOL)
        {
            node->dataType = TYPE_BOOL;
        }
        else
        {
            error(SEMANTIC_TYPE_ERROR, "Invalid types for logical operation");
        }
        break;
    default:
        // Handle other binary operations as needed
        break;
    }

    // Here you can implement type checking for binary operations
    // For example, if both operands are integers, the result is an integer
}

static void analyzeUnaryOp(astNode *node)
{
    if (node == NULL || node->type != AST_UNARY_OP)
    {
        error(INTERNAL_ERROR, "Invalid unary operation");
        return;
    }

    analyzeExpr(node->data.unary.operand);

    astDataType operandType = node->data.unary.operand->dataType;

    switch (node->data.unary.op)
    {
    case op_not:
        if (operandType == TYPE_BOOL)
        {
            node->dataType = TYPE_BOOL;
        }
        else
        {
            error(SEMANTIC_TYPE_ERROR, "Invalid type for logical NOT operation");
        }
        break;
    case op_sub:
        if (operandType == TYPE_INT || operandType == TYPE_DOUBLE)
        {
            node->dataType = operandType; // Result type is the same as operand type
        }
        else
        {
            error(SEMANTIC_TYPE_ERROR, "Invalid type for unary negation operation");
        }
        break;
    case op_add:
        if (operandType != TYPE_INT && operandType != TYPE_DOUBLE)
        {
            error(SEMANTIC_TYPE_ERROR, "Invalid type for unary plus operation");
        }
        else
        {
            node->dataType = operandType; // Result type is the same as operand type
        }
        break;
    default:
        // Handle other unary operations as needed
        break;
    }
}