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

/**
 * @brief Init an AST node list
 * @param list The list to init
 */
void astListInit(astNodeList *list);
/**
 * @brief Append a node to an AST node list
 * @param list The list to append to
 * @param node The node to append
 */
void astListAppend(astNodeList *list, astNode *node);
/**
 * @brief Free an AST node list
 * @param list The list to free
 */
void astListFree(astNodeList *list);

/**
 * @brief Creates a new node
 * @param type Specifies the type of the AST node
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new node
 */
astNode *astNewNode(astNodeType type, int line, int col);
/**
 * @brief Frees the AST
 * @param node Root node of AST
 */
void astFree(astNode *node);

/**
 * @brief Creates a node of type Program
 * @param importName Value of the import string
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Program node
 */
astNode *astNewProgram(char *importName, int line, int col);
/**
 * @brief Creates a node of type Function Definition
 * @param name Name of the function being defined
 * @param isMain Is the function a main function (@main)
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Function Definition node
 */
astNode *astNewFuncDef(char *name, bool isMain, int line, int col);
/**
 * @brief Creates a node of type Extension
 * @param receiver A node that the extension is being called on
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Extension node
 */
astNode *astNewExtension(astNode *receiver, int line, int col);
/**
 * @brief Creates a node of type Parameter list
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Parameter list node
 */
astNode *astNewParamList(int line, int col);
/**
 * @brief Creates a node of type Parameter
 * @param name Name of the parameter
 * @param type Type of the parameter
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Parameter node
 */
astNode *astNewParam(char *name, astDataType type, int line, int col);

/**
 * @brief Creates a node of type Code Block
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Code Block node
 */
astNode *astNewBlock(int line, int col);
/**
 * @brief Creates a node of type Var Declaration
 * @param name Name of the declared variable
 * @param declType Type of the declared variable
 * @param value Node that holds the value
 * @param isMutable Specifies if the variable can be changed (var / val)
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Var Declaration node
 */
astNode *astNewVarDecl(char *name, astDataType declType, astNode *value,
					   bool isMutable, int line, int col);
/**
 * @brief Creates a node of type Assignment
 * @param target Which node is being assigned to
 * @param value Node that holds the value being assigned
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Assignment node
 */
astNode *astNewAssign(astNode *target, astNode *value, int line, int col);
/**
 * @brief Creates a node of type Return
 * @param value Node that holds the value being returned
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Return node
 */
astNode *astNewReturn(astNode *value, int line, int col);
/**
 * @brief Creates a node of type While
 * @param cond Node holding the condition of the loop
 * @param body Node holding the body of the loop
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new While node
 */
astNode *astNewWhile(astNode *cond, astNode *body, int line, int col);
/**
 * @brief Creates a node of type For
 * @param iterName Iterator name
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new For node
 */
astNode *astNewFor(char *iterName, int line, int col);
/**
 * @brief Creates a node of type If
 * @param cond Node holding the condition of the statement
 * @param thenBody Node holding the first body of the If
 * @param elseBody Node holding the else body of the If
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new If node
 */
astNode *astNewIf(astNode *cond, astNode *thenBody, astNode *elseBody, int line,
				  int col);
/**
 * @brief Creates a node of type Expression statement
 * @param expr Node holding the expression
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Expression statement node
 */
astNode *astNewExprStmt(astNode *expr, int line, int col);

/**
 * @brief Creates a node of type Integer literal
 * @param value value of the integer
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Integer literal node
 */
astNode *astNewIntLit(long long value, int line, int col);
/**
 * @brief Creates a node of type Double literal
 * @param value value of the double
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Double literal node
 */
astNode *astNewDoubleLit(double value, int line, int col);
/**
 * @brief Creates a node of type String literal
 * @param value value of the string
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new String literal node
 */
astNode *astNewStringLit(char *value, int line, int col);
/**
 * @brief Creates a node of type Boolean literal
 * @param value value of the boolean
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Boolean literal node
 */
astNode *astNewBoolLit(bool value, int line, int col);
/**
 * @brief Creates a node of type Unit literal
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Unit literal node
 */
astNode *astNewUnitLit(int line, int col);
/**
 * @brief Creates a node of type Identifier
 * @param name Name of the identifier
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Identifier node
 */
astNode *astNewIdentifier(char *name, int line, int col);
/**
 * @brief Creates a node of type Binary operation
 * @param op Operation being performed
 * @param left Left operand
 * @param right Right operand
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Binary operation node
 */
astNode *astNewBinary(specTokenType op, astNode *left, astNode *right, int line,
					  int col);
/**
 * @brief Creates a node of type Unary operation
 * @param op Operation being performed
 * @param operand Operand the operation is being performed on
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Unary operation node
 */
astNode *astNewUnary(specTokenType op, astNode *operand, int line, int col);
/**
 * @brief Creates a node of type Call
 * @param callee Function being called
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Call node
 */
astNode *astNewCall(astNode *callee, int line, int col);
/**
 * @brief Creates a node of type Member access
 * @param object Node whose member is being accessed
 * @param name Name of the member
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Member access node
 *
 * Node for the VALMEM extension
 */
astNode *astNewMember(astNode *object, char *name, int line, int col);
/**
 * @brief Creates a node of type Interpolated string
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Interpolated string node
 *
 * Special node for the INTERPOLATEDS extension,
 * Interpolated strings are being "built" as the
 * parser is getting tokens
 */
astNode *astNewInterp(int line, int col);

/**
 * @brief Helper function for getting type from a token that
 * 		  scanner returned
 * @param specType Token type returned from the scanner
 * @return Matching AST data type, TYPE_NONE if the token is not a type
 */
astDataType astTypeFromToken(specTokenType specType);

#endif
