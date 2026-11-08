/**
 * @name ifj26
 * @authors Tomáš Zajac (xzajact00)
 * @file sematic.c
 */

#include <stdio.h>
// #include <stdlib.h>
#include "sematic.h"
#include "error.h"
#include "ast.h"

static void analyzeNode(astNode *node);

static void analyzeNodeList(astNodeList nodeList)
{
    for (size_t i = 0; i < nodeList.count; i++)
    {
        analyzeNode(nodeList.items[i]);
    }
}

void semanticAnalyze(astNode *root)
{
    if (root == NULL || root->type == AST_PROGRAM)
    {
        error(INTERNAL_ERROR, "Invalid ast root");
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
        analyzeNode(node->data.whileLoop.cond);
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
        analyzeNode(node->data.ifStmt.cond);
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
        analyzeNode(node->data.binary.left);
        analyzeNode(node->data.binary.right);
        break;
    case AST_UNARY_OP:
        analyzeNode(node->data.unary.operand);
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
