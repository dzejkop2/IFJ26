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
	AST_INTERP,
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
	TYPE_ANY
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

/**
 * @brief Union that is in every node of the AST,
 *		  each AST node needs different data
 */
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

	struct {
		astNodeList parts;
	} interp;
} nodeData;

/**
 * @brief A node in the abstract syntax tree (AST)
 */
struct astNode {
	astNodeType type;
	astDataType dataType;
	int line;
	int col;
	int scopeId;
	bool intToDouble;
	nodeData data;
};

void astListInit(astNodeList *list);
void astListAppend(astNodeList *list, astNode *node);
void astListFree(astNodeList *list);

astNode *astNewNode(astNodeType type, int line, int col);
void astFree(astNode *node);

astNode *astNewProgram(char *importName, int line, int col);
astNode *astNewFuncDef(char *name, bool isMain, int line, int col);
astNode *astNewExtension(astNode *receiver, int line, int col);
astNode *astNewParamList(int line, int col);
astNode *astNewParam(char *name, astDataType type, int line, int col);

astNode *astNewBlock(int line, int col);
astNode *astNewVarDecl(char *name, astDataType declType, astNode *value,
					   bool isMutable, int line, int col);
astNode *astNewAssign(astNode *target, astNode *value, int line, int col);
astNode *astNewReturn(astNode *value, int line, int col);
astNode *astNewWhile(astNode *cond, astNode *body, int line, int col);
astNode *astNewFor(char *iterName, int line, int col);
astNode *astNewIf(astNode *cond, astNode *thenBody, astNode *elseBody, int line,
				  int col);
astNode *astNewExprStmt(astNode *expr, int line, int col);

astNode *astNewIntLit(long long value, int line, int col);
astNode *astNewDoubleLit(double value, int line, int col);
astNode *astNewStringLit(char *value, int line, int col);
astNode *astNewBoolLit(bool value, int line, int col);
astNode *astNewUnitLit(int line, int col);
astNode *astNewIdentifier(char *name, int line, int col);
astNode *astNewBinary(specTokenType op, astNode *left, astNode *right, int line,
					  int col);
astNode *astNewUnary(specTokenType op, astNode *operand, int line, int col);
astNode *astNewCall(astNode *callee, int line, int col);
astNode *astNewMember(astNode *object, char *name, int line, int col);
astNode *astNewInterp(int line, int col);

astDataType astTypeFromToken(specTokenType specType);

#endif
