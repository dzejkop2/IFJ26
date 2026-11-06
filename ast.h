/**
 * @name ifj26
 * @authors Jakub Geleta (xgeletj00)
 * @file ast.h
 */

#ifndef AST_H
#define AST_H

#include <stddef.h>
#include <stdbool.h>
#include "scanner.h"

/**
 * @brief Type of nodes in the AST
 */
typedef enum {
    AST_PROGRAM,
    AST_FUN_DEF,
    AST_EXTENSION,
    AST_PARAM_LIST,
    AST_PARAM,

    AST_BLOCK,
    AST_VAR_DECL,
    AST_ASSIGN,
    AST_RETURN,
    AST_WHILE,
    AST_FOR,
    AST_EXPR_STMT,
    AST_IF,

    AST_LIT_INT,
    AST_LIT_DOUBLE,
    AST_LIT_STRING,
    AST_LIT_BOOL,
    AST_LIT_UNIT,
    AST_IDENTIFIER,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_CALL,
    AST_MEMBER,
} astNodeType;

/** 
 * @brief Data types for AST nodes 
 */
typedef enum {
    TYPE_NONE,
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_UNIT,
    TYPE_BOOL,
} astDataType;

typedef struct astNode astNode;

/**
 * @brief A list of AST nodes when a node can have multiple children, 
 *       for easier access (global defs, functions, ...)
 */
typedef struct {
    astNode **items;
    size_t count;
    size_t capacity;
} astNodeList;

typedef union nodeData {
    long long intValue;
    double doubleValue;
    char *stringValue;
    bool boolValue;

    struct {
        char *importName;
        astNodeList defs;
    } program;

    struct {
        char *name;
        astNodeList paramLists;
        astDataType retType;
        astNode *body;
        bool isMain;
    } funcDef;

    struct {
        astNode *receiver;
        astNodeList funDefs;
    } extension;

    struct {
        astNodeList params;
    } paramList;

    struct {
        char *name;
        astDataType type;
    } param;

    struct {
        astNodeList stmts;
    } block;

    struct {
        char *name;
        astDataType declType;
        astNode *value;
        bool isMutable;
    } varDecl;

    struct {
        astNode *target;
        astNode *value;
    } assign;

    struct {
        astNode *value;
    } ret;

    struct {
        astNode *cond;
        astNode *body;
    } whileLoop;

    struct {
        char *iterName;
        astNode *from;
        astNode *to;
        astNode *step;
        bool isInclusive;
        astNodeList guards;
        astNode *body;
    } forLoop;

    struct {
        astNode *cond;
        astNode *thenBody;
        astNode *elseBody;
    } ifStmt;

    struct {
        astNode *expr;
    } exprStmt;

    struct {
        char *name;
    } identifier;

    struct {
        specTokenType op;
        astNode *left;
        astNode *right;
    } binary;

    struct {
        specTokenType op;
        astNode *operand;
    } unary;

    struct {
        astNode *callee;
        astNodeList args;
    } call;

    struct {
        astNode *object;
        char *name;
    } member;
} nodeData;

/**
 * @brief A node in the abstract syntax tree (AST)
 */
struct astNode {
    astNodeType type;
    astDataType dataType;
    int line;
    int col;
    nodeData data;
};

/**
 * @brief Init an AST node list
 * @param list The list to init
 */
void ast_listInit(astNodeList *list);
/**
 * @brief Append a node to an AST node list
 * @param list The list to append to
 * @param node The node to append
 */
void ast_listAppend(astNodeList *list, astNode *node);
/**
 * @brief Free an AST node list
 * @param list The list to free
 */
void ast_listFree(astNodeList *list);

astNode *ast_newNode(astNodeType type, int line, int col);
void ast_free(astNode *node);

astNode *ast_newProgram(char *importName, int line, int col);
astNode *ast_newFuncDef(char *name, bool isMain, int line, int col);
astNode *ast_newExtension(astNode *receiver, int line, int col);
astNode *ast_newParamList(int line, int col);
astNode *ast_newParam(char *name, astDataType type, int line, int col);

astNode *ast_newBlock(int line, int col);
astNode *ast_newVarDecl(char *name, astDataType declType, astNode *value, bool isMutable, int line, int col);
astNode *ast_newAssign(astNode *target, astNode *value, int line, int col);
astNode *ast_newReturn(astNode *value, int line, int col);
astNode *ast_newWhile(astNode *cond, astNode *body, int line, int col);
astNode *ast_newFor(char *iterName, int line, int col);
astNode *ast_newIf(astNode *cond, astNode *thenBody, astNode *elseBody, int line, int col);
astNode *ast_newExprStmt(astNode *expr, int line, int col);

astNode *ast_newIntLit(long long value, int line, int col);
astNode *ast_newDoubleLit(double value, int line, int col);
astNode *ast_newStringLit(char *value, int line, int col);
astNode *ast_newBoolLit(bool value, int line, int col);
astNode *ast_newUnitLit(int line, int col);
astNode *ast_newIdentifier(char *name, int line, int col);
astNode *ast_newBinary(specTokenType op, astNode *left, astNode *right, int line, int col);
astNode *ast_newUnary(specTokenType op, astNode *operand, int line, int col);
astNode *ast_newCall(astNode *callee, int line, int col);
astNode *ast_newMember(astNode *object, char *name, int line, int col);

astDataType ast_typeFromToken(specTokenType specType);

#endif
