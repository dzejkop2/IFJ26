/**
 * @name ifj26
 * @authors Jakub Geleta (xgeletj00)
 * @file ast.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "error.h"

#define LIST_INIT_CAPACITY 4

void ast_listInit(astNodeList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void ast_listAppend(astNodeList *list, astNode *node) {
    if (list->count == list->capacity) {
        size_t newCapacity = list->capacity;
        if(newCapacity == 0) 
            newCapacity = LIST_INIT_CAPACITY;
        else 
            newCapacity *= 2;

        astNode **newCapTemp = realloc(list->items, newCapacity * sizeof(astNode *));
        if (newCapTemp == NULL)
            error(INTERNAL_ERROR, "AST node list realloc memory failed");

        list->items = newCapTemp;
        list->capacity = newCapacity;
    }

    list->items[list->count] = node;
    list->count++;
}

void ast_listFree(astNodeList *list) {
    free(list->items);
    ast_listInit(list);
}

static void freeNodeList(astNodeList *list) {
    for (size_t i = 0; i < list->count; i++) {
        ast_free(list->items[i]);
    }

    ast_listFree(list);
}

astNode *ast_newNode(astNodeType type, int line, int col) {
    astNode *node = calloc(1, sizeof(astNode));
    if (node == NULL)
        error(INTERNAL_ERROR, "AST node malloc memory failed");

    node->type = type;
    node->dataType = TYPE_NONE;
    node->line = line;
    node->col = col;
    return node;
}

void ast_free(astNode *node) {
    if (node == NULL)
        return;

    switch (node->type) {
        case AST_PROGRAM:
            free(node->data.program.importName);
            freeNodeList(&node->data.program.defs);
            break;
        case AST_FUN_DEF:
            free(node->data.funDef.name);
            freeNodeList(&node->data.funDef.paramLists);
            ast_free(node->data.funDef.body);
            break;
        case AST_EXTENSION:
            ast_free(node->data.extension.receiver);
            freeNodeList(&node->data.extension.funDefs);
            break;
        case AST_PARAM_LIST:
            freeNodeList(&node->data.paramList.params);
            break;
        case AST_PARAM:
            free(node->data.param.name);
            break;

        case AST_BLOCK:
            freeNodeList(&node->data.block.stmts);
            break;
        case AST_VAR_DECL:
            free(node->data.varDecl.name);
            ast_free(node->data.varDecl.value);
            break;
        case AST_ASSIGN:
            ast_free(node->data.assign.target);
            ast_free(node->data.assign.value);
            break;
        case AST_RETURN:
            ast_free(node->data.ret.value);
            break;
        case AST_WHILE:
            ast_free(node->data.whileLoop.cond);
            ast_free(node->data.whileLoop.body);
            break;
        case AST_FOR:
            free(node->data.forLoop.iterName);
            ast_free(node->data.forLoop.from);
            ast_free(node->data.forLoop.to);
            ast_free(node->data.forLoop.step);
            freeNodeList(&node->data.forLoop.guards);
            ast_free(node->data.forLoop.body);
            break;
        case AST_EXPR_STMT:
            ast_free(node->data.exprStmt.expr);
            break;
        case AST_IF:
            ast_free(node->data.ifStmt.cond);
            ast_free(node->data.ifStmt.thenBody);
            ast_free(node->data.ifStmt.elseBody);
            break;

        case AST_LIT_STRING:
            free(node->data.stringValue);
            break;
        case AST_LIT_INT:
        case AST_LIT_DOUBLE:
        case AST_LIT_BOOL:
        case AST_LIT_UNIT:
            break;
        case AST_IDENTIFIER:
            free(node->data.identifier.name);
            break;
        case AST_BINARY_OP:
            ast_free(node->data.binary.left);
            ast_free(node->data.binary.right);
            break;
        case AST_UNARY_OP:
            ast_free(node->data.unary.operand);
            break;
        case AST_CALL:
            ast_free(node->data.call.callee);
            freeNodeList(&node->data.call.args);
            break;
        case AST_MEMBER:
            ast_free(node->data.member.object);
            free(node->data.member.name);
            break;
    }

    free(node);
}

astNode *ast_newProgram(char *importName, int line, int col) {
    astNode *node = ast_newNode(AST_PROGRAM, line, col);

    node->data.program.importName = importName;
    ast_listInit(&node->data.program.defs);

    return node;
}

astNode *ast_newFunDef(char *name, bool isMain, int line, int col) {
    astNode *node = ast_newNode(AST_FUN_DEF, line, col);

    node->data.funDef.name = name;
    ast_listInit(&node->data.funDef.paramLists);
    node->data.funDef.retType = TYPE_NONE;
    node->data.funDef.body = NULL;
    node->data.funDef.isMain = isMain;

    return node;
}

astNode *ast_newExtension(astNode *receiver, int line, int col) {
    astNode *node = ast_newNode(AST_EXTENSION, line, col);
    
    node->data.extension.receiver = receiver;
    ast_listInit(&node->data.extension.funDefs);
    
    return node;
}

astNode *ast_newParamList(int line, int col) {
    astNode *node = ast_newNode(AST_PARAM_LIST, line, col);

    ast_listInit(&node->data.paramList.params);
    
    return node;
}

astNode *ast_newParam(char *name, astDataType type, int line, int col) {
    astNode *node = ast_newNode(AST_PARAM, line, col);

    node->data.param.name = name;
    node->data.param.type = type;

    return node;
}

astNode *ast_newBlock(int line, int col) {
    astNode *node = ast_newNode(AST_BLOCK, line, col);

    ast_listInit(&node->data.block.stmts);

    return node;
}

astNode *ast_newVarDecl(char *name, astDataType declType, astNode *value, bool isMutable, int line, int col) {
    astNode *node = ast_newNode(AST_VAR_DECL, line, col);

    node->data.varDecl.name = name;
    node->data.varDecl.declType = declType;
    node->data.varDecl.value = value;
    node->data.varDecl.isMutable = isMutable;

    return node;
}

astNode *ast_newAssign(astNode *target, astNode *value, int line, int col) {
    astNode *node = ast_newNode(AST_ASSIGN, line, col);

    node->data.assign.target = target;
    node->data.assign.value = value;
    
    return node;
}

astNode *ast_newReturn(astNode *value, int line, int col) {
    astNode *node = ast_newNode(AST_RETURN, line, col);

    node->data.ret.value = value;
 
    return node;
}

astNode *ast_newWhile(astNode *cond, astNode *body, int line, int col) {
    astNode *node = ast_newNode(AST_WHILE, line, col);

    node->data.whileLoop.cond = cond;
    node->data.whileLoop.body = body;
    
    return node;
}

astNode *ast_newFor(char *iterName, int line, int col) {
    astNode *node = ast_newNode(AST_FOR, line, col);

    node->data.forLoop.iterName = iterName;
    node->data.forLoop.from = NULL;
    node->data.forLoop.to = NULL;
    node->data.forLoop.step = NULL;
    node->data.forLoop.isInclusive = false;
    ast_listInit(&node->data.forLoop.guards);
    node->data.forLoop.body = NULL;
    
    return node;
}

astNode *ast_newIf(astNode *cond, astNode *thenBody, astNode *elseBody, int line, int col) {
    astNode *node = ast_newNode(AST_IF, line, col);

    node->data.ifStmt.cond = cond;
    node->data.ifStmt.thenBody = thenBody;
    node->data.ifStmt.elseBody = elseBody;
    
    return node;
}

astNode *ast_newExprStmt(astNode *expr, int line, int col) {
    astNode *node = ast_newNode(AST_EXPR_STMT, line, col);
    
    node->data.exprStmt.expr = expr;
    
    return node;
}

astNode *ast_newIntLit(long long value, int line, int col) {
    astNode *node = ast_newNode(AST_LIT_INT, line, col);

    node->data.intValue = value;
    
    return node;
}

astNode *ast_newDoubleLit(double value, int line, int col) {
    astNode *node = ast_newNode(AST_LIT_DOUBLE, line, col);

    node->data.doubleValue = value;
    
    return node;
}

astNode *ast_newStringLit(char *value, int line, int col) {
    astNode *node = ast_newNode(AST_LIT_STRING, line, col);

    node->data.stringValue = value;
    
    return node;
}

astNode *ast_newBoolLit(bool value, int line, int col) {
    astNode *node = ast_newNode(AST_LIT_BOOL, line, col);

    node->data.boolValue = value;
    
    return node;
}

astNode *ast_newUnitLit(int line, int col) {
    return ast_newNode(AST_LIT_UNIT, line, col);
}

astNode *ast_newIdentifier(char *name, int line, int col) {
    astNode *node = ast_newNode(AST_IDENTIFIER, line, col);

    node->data.identifier.name = name;
    
    return node;
}

astNode *ast_newBinary(specTokenType op, astNode *left, astNode *right, int line, int col) {
    astNode *node = ast_newNode(AST_BINARY_OP, line, col);

    node->data.binary.op = op;
    node->data.binary.left = left;
    node->data.binary.right = right;
    
    return node;
}

astNode *ast_newUnary(specTokenType op, astNode *operand, int line, int col) {
    astNode *node = ast_newNode(AST_UNARY_OP, line, col);

    node->data.unary.op = op;
    node->data.unary.operand = operand;
    
    return node;
}

astNode *ast_newCall(astNode *callee, int line, int col) {
    astNode *node = ast_newNode(AST_CALL, line, col);

    node->data.call.callee = callee;
    ast_listInit(&node->data.call.args);
    
    return node;
}

astNode *ast_newMember(astNode *object, char *name, int line, int col) {
    astNode *node = ast_newNode(AST_MEMBER, line, col);

    node->data.member.object = object;
    node->data.member.name = name;
    
    return node;
}

astDataType ast_typeFromToken(specTokenType specType) {
    switch (specType){
        case dt_int: 
            return TYPE_INT;
        case dt_double:
            return TYPE_DOUBLE;
        case dt_string:
            return TYPE_STRING;
        case dt_unit:
            return TYPE_UNIT;
        case dt_bool:
            return TYPE_BOOL;
        default:
            return TYPE_NONE;
    }
}
