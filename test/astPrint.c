/**
 * @name ifj26
 * @authors Jakub Geleta (xgeletj00)
 * @file astPrint.c
 */

#include <stdio.h>
#include "astPrint.h"

static const char *typeName(astDataType type) {
    switch (type) {
        case TYPE_NONE:
            return "NONE";
        case TYPE_INT:
            return "Int";
        case TYPE_DOUBLE:
            return "Double";
        case TYPE_STRING:
            return "String";
        case TYPE_UNIT:
            return "Unit";
        case TYPE_BOOL:
            return "Boolean";
    }
    return "?";
}

static const char *opName(specTokenType op) {
    switch (op) {
        case op_add:
            return "+";
        case op_sub:
            return "-";
        case op_mul:
            return "*";
        case op_div:
            return "/";
        case op_lowerThen:
            return "<";
        case op_lowerEqual:
            return "<=";
        case op_biggerThen:
            return ">";
        case op_biggerEqual:
            return ">=";
        case op_equal:
            return "==";
        case op_notEqual:
            return "!=";
        case op_and:
            return "&&";
        case op_or:
            return "||";
        case op_not:
            return "!";
        default:
            return "?";
    }
}

static void printIndent(int depth) {
    for (int i = 0; i < depth; i++) {
        fputs("  ", stderr);
    }
}

static void printEscaped(const char *str) {
    fputc('"', stderr);
    for (const char *c = str; c != NULL && *c != '\0'; c++) {
        switch (*c) {
            case '\n':
                fputs("\\n", stderr);
                break;
            case '\r':
                fputs("\\r", stderr);
                break;
            case '\t':
                fputs("\\t", stderr);
                break;
            case '"': 
                fputs("\\\"", stderr);
                break;
            case '\\':
                fputs("\\\\", stderr);
                break;
            default:
                fputc(*c, stderr);
                break;
        }
    }
    fputc('"', stderr);
}

static void printChild(const char *label, const astNode *child, int depth) {
    printIndent(depth);
    fprintf(stderr, "%s:\n", label);
    ast_print(child, depth + 1);
}

static void printList(const astNodeList *list, int depth) {
    for (size_t i = 0; i < list->count; i++) {
        ast_print(list->items[i], depth + 1);
    }
}

void ast_print(const astNode *node, int depth) {
    printIndent(depth);
    if (node == NULL) {
        fputs("(null)\n", stderr);
        return;
    }

    switch (node->type) {
        case AST_PROGRAM:
            fputs("PROGRAM import=", stderr);
            fputs(node->data.program.importName ? node->data.program.importName : "(null)", stderr);
            break;
        case AST_FUN_DEF:
            fprintf(stderr, "FUN_DEF name=%s main=%d ret=%s", node->data.funcDef.name,
                    node->data.funcDef.isMain, typeName(node->data.funcDef.retType));
            break;
        case AST_EXTENSION: 
            fputs("EXTENSION", stderr);
            break;
        case AST_PARAM_LIST:
            fputs("PARAM_LIST", stderr);
            break;
        case AST_PARAM:
            fprintf(stderr, "PARAM %s : %s", node->data.param.name, typeName(node->data.param.type));
            break;
        case AST_BLOCK:
            fputs("BLOCK", stderr);
            break;
        case AST_VAR_DECL:
            fprintf(stderr, "VAR_DECL %s %s : %s", node->data.varDecl.isMutable ? "var" : "val",
                    node->data.varDecl.name, typeName(node->data.varDecl.declType));
            break;
        case AST_ASSIGN:
            fputs("ASSIGN", stderr);
            break;
        case AST_RETURN:
            fputs("RETURN", stderr);
            break;
        case AST_WHILE:
            fputs("WHILE", stderr);
            break;
        case AST_FOR:
            fprintf(stderr, "FOR %s %s", node->data.forLoop.iterName ? node->data.forLoop.iterName : "(null)",
                    node->data.forLoop.isInclusive ? "to" : "until");
            break;
        case AST_EXPR_STMT:
            fputs("EXPR_STMT", stderr);
            break;
        case AST_IF:
            fputs("IF", stderr);
            break;
        case AST_LIT_INT:
            fprintf(stderr, "LIT_INT %lld", node->data.intValue);
            break;
        case AST_LIT_DOUBLE:
            fprintf(stderr, "LIT_DOUBLE %g", node->data.doubleValue);
            break;
        case AST_LIT_STRING:
            fputs("LIT_STRING ", stderr);
            printEscaped(node->data.stringValue);
            break;
        case AST_LIT_BOOL:
            fprintf(stderr, "LIT_BOOL %s", node->data.boolValue ? "true" : "false");
             break;
        case AST_LIT_UNIT:
            fputs("LIT_UNIT", stderr);
            break;
        case AST_IDENTIFIER:
            fprintf(stderr, "IDENTIFIER %s", node->data.identifier.name);
            break;
        case AST_BINARY_OP:
            fprintf(stderr, "BINARY %s", opName(node->data.binary.op));
            break;
        case AST_UNARY_OP:
            fprintf(stderr, "UNARY %s", opName(node->data.unary.op));
            break;
        case AST_CALL:
            fputs("CALL", stderr);
            break;
        case AST_MEMBER:
            fprintf(stderr, "MEMBER .%s", node->data.member.name);
            break;
    }

    if (node->dataType != TYPE_NONE) {
        fprintf(stderr, " type=%s", typeName(node->dataType));
    }
    fputc('\n', stderr);

    switch (node->type){
        case AST_PROGRAM:
            printList(&node->data.program.defs, depth);
            break;
        case AST_FUN_DEF:
            printList(&node->data.funcDef.paramLists, depth);
            ast_print(node->data.funcDef.body, depth + 1);
            break;
        case AST_EXTENSION:
            ast_print(node->data.extension.receiver, depth + 1);
            printList(&node->data.extension.funDefs, depth);
            break;
        case AST_PARAM_LIST:
            printList(&node->data.paramList.params, depth);
            break;
        case AST_PARAM:
            break;
        case AST_BLOCK:
            printList(&node->data.block.stmts, depth);
            break;
        case AST_VAR_DECL:
            ast_print(node->data.varDecl.value, depth + 1);
            break;
        case AST_ASSIGN:
            printChild("target", node->data.assign.target, depth + 1);
            printChild("value", node->data.assign.value, depth + 1);
            break;
        case AST_RETURN:
            ast_print(node->data.ret.value, depth + 1);
            break;
        case AST_WHILE:
            printChild("cond", node->data.whileLoop.cond, depth + 1);
            printChild("body", node->data.whileLoop.body, depth + 1);
            break;
        case AST_FOR:
            printChild("from", node->data.forLoop.from, depth + 1);
            printChild("to", node->data.forLoop.to, depth + 1);
            printChild("step", node->data.forLoop.step, depth + 1);
            printIndent(depth + 1);
            fputs("guards:\n", stderr);
            printList(&node->data.forLoop.guards, depth + 1);
            printChild("body", node->data.forLoop.body, depth + 1);
            break;
        case AST_EXPR_STMT:
            ast_print(node->data.exprStmt.expr, depth + 1);
            break;
        case AST_IF:
            printChild("cond", node->data.ifStmt.cond, depth + 1);
            printChild("then", node->data.ifStmt.thenBody, depth + 1);
            printChild("else", node->data.ifStmt.elseBody, depth + 1);
            break;
        case AST_LIT_INT:
        case AST_LIT_DOUBLE:
        case AST_LIT_STRING:
        case AST_LIT_BOOL:
        case AST_LIT_UNIT:
        case AST_IDENTIFIER:
            break;
        case AST_BINARY_OP:
            ast_print(node->data.binary.left, depth + 1);
            ast_print(node->data.binary.right, depth + 1);
            break;
        case AST_UNARY_OP:
            ast_print(node->data.unary.operand, depth + 1);
            break;
        case AST_CALL:
            printChild("callee", node->data.call.callee, depth + 1);
            printIndent(depth + 1);
            fputs("args:\n", stderr);
            printList(&node->data.call.args, depth + 1);
            break;
        case AST_MEMBER:
            ast_print(node->data.member.object, depth + 1);
            break;
    }
}
