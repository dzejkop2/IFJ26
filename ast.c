/**
 * @name ifj26
 * @authors Jakub Geleta (xgeletj00)
 * @file ast.c
 *
 * AI useage:
 * 	"Make tests for simple AST and check for correct filling"
 *  "Make tests that check if every node is being created correctly"
 * 	"Create stub functions for implementation"
 */

#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "error.h"

#define LIST_INIT_CAPACITY 4

/**
 * @brief Init an AST node list
 * @param list The list to init
 */
void astListInit(astNodeList *list) {
	list->items = NULL;
	list->count = 0;
	list->capacity = 0;
}

/**
 * @brief Append a node to an AST node list
 * @param list The list to append to
 * @param node The node to append
 */
void astListAppend(astNodeList *list, astNode *node) {
	if (list->count == list->capacity) {
		size_t newCapacity = list->capacity;
		if (newCapacity == 0)
			newCapacity = LIST_INIT_CAPACITY;
		else
			newCapacity *= 2;

		astNode **newCapTemp =
			realloc(list->items, newCapacity * sizeof(astNode *));
		if (newCapTemp == NULL)
			error(INTERNAL_ERROR, "AST node list realloc memory failed");

		list->items = newCapTemp;
		list->capacity = newCapacity;
	}

	list->items[list->count] = node;
	list->count++;
}

/**
 * @brief Free an AST node list
 * @param list The list to free
 */
void astListFree(astNodeList *list) {
	free(list->items);
	astListInit(list);
}

static void freeNodeList(astNodeList *list) {
	for (size_t i = 0; i < list->count; i++) {
		astFree(list->items[i]);
	}

	astListFree(list);
}

/**
 * @brief Creates a new node
 * @param type Specifies the type of the AST node
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new node
 */
astNode *astNewNode(astNodeType type, int line, int col) {
	astNode *node = calloc(1, sizeof(astNode));
	if (node == NULL)
		error(INTERNAL_ERROR, "AST node malloc memory failed");

	node->type = type;
	node->dataType = TYPE_NONE;
	node->line = line;
	node->col = col;
	return node;
}

/**
 * @brief Frees the AST
 * @param node Root node of AST
 */
void astFree(astNode *node) {
	if (node == NULL)
		return;

	switch (node->type) {
	case AST_PROGRAM:
		free(node->data.program.importName);
		freeNodeList(&node->data.program.defs);
		break;
	case AST_FUN_DEF:
		free(node->data.funcDef.name);
		freeNodeList(&node->data.funcDef.paramLists);
		astFree(node->data.funcDef.body);
		break;
	case AST_EXTENSION:
		astFree(node->data.extension.receiver);
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
		astFree(node->data.varDecl.value);
		break;
	case AST_ASSIGN:
		astFree(node->data.assign.target);
		astFree(node->data.assign.value);
		break;
	case AST_RETURN:
		astFree(node->data.ret.value);
		break;
	case AST_WHILE:
		astFree(node->data.whileLoop.cond);
		astFree(node->data.whileLoop.body);
		break;
	case AST_FOR:
		free(node->data.forLoop.iterName);
		astFree(node->data.forLoop.from);
		astFree(node->data.forLoop.to);
		astFree(node->data.forLoop.step);
		freeNodeList(&node->data.forLoop.guards);
		astFree(node->data.forLoop.body);
		break;
	case AST_EXPR_STMT:
		astFree(node->data.exprStmt.expr);
		break;
	case AST_IF:
		astFree(node->data.ifStmt.cond);
		astFree(node->data.ifStmt.thenBody);
		astFree(node->data.ifStmt.elseBody);
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
		astFree(node->data.binary.left);
		astFree(node->data.binary.right);
		break;
	case AST_UNARY_OP:
		astFree(node->data.unary.operand);
		break;
	case AST_CALL:
		astFree(node->data.call.callee);
		freeNodeList(&node->data.call.args);
		break;
	case AST_MEMBER:
		astFree(node->data.member.object);
		free(node->data.member.name);
		break;
	case AST_INTERP:
		freeNodeList(&node->data.interp.parts);
		break;
	}

	free(node);
}

/**
 * @brief Creates a node of type Program
 * @param importName Value of the import string
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Program node
 */
astNode *astNewProgram(char *importName, int line, int col) {
	astNode *node = astNewNode(AST_PROGRAM, line, col);

	node->data.program.importName = importName;
	astListInit(&node->data.program.defs);

	return node;
}

/**
 * @brief Creates a node of type Function Definition
 * @param name Name of the function being defined
 * @param isMain Is the function a main function (@main)
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Function Definition node
 */
astNode *astNewFuncDef(char *name, bool isMain, int line, int col) {
	astNode *node = astNewNode(AST_FUN_DEF, line, col);

	node->data.funcDef.name = name;
	astListInit(&node->data.funcDef.paramLists);
	node->data.funcDef.retType = TYPE_NONE;
	node->data.funcDef.body = NULL;
	node->data.funcDef.isMain = isMain;

	return node;
}

/**
 * @brief Creates a node of type Extension
 * @param receiver A node that the extension is being called on
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Extension node
 */
astNode *astNewExtension(astNode *receiver, int line, int col) {
	astNode *node = astNewNode(AST_EXTENSION, line, col);

	node->data.extension.receiver = receiver;
	astListInit(&node->data.extension.funDefs);

	return node;
}

/**
 * @brief Creates a node of type Parameter list
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Parameter list node
 */
astNode *astNewParamList(int line, int col) {
	astNode *node = astNewNode(AST_PARAM_LIST, line, col);

	astListInit(&node->data.paramList.params);

	return node;
}

/**
 * @brief Creates a node of type Parameter
 * @param name Name of the parameter
 * @param type Type of the parameter
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Parameter node
 */
astNode *astNewParam(char *name, astDataType type, int line, int col) {
	astNode *node = astNewNode(AST_PARAM, line, col);

	node->data.param.name = name;
	node->data.param.type = type;

	return node;
}

/**
 * @brief Creates a node of type Code Block
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Code Block node
 */
astNode *astNewBlock(int line, int col) {
	astNode *node = astNewNode(AST_BLOCK, line, col);

	astListInit(&node->data.block.stmts);

	return node;
}

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
					   bool isMutable, int line, int col) {
	astNode *node = astNewNode(AST_VAR_DECL, line, col);

	node->data.varDecl.name = name;
	node->data.varDecl.declType = declType;
	node->data.varDecl.value = value;
	node->data.varDecl.isMutable = isMutable;

	return node;
}

/**
 * @brief Creates a node of type Assignment
 * @param target Which node is being assigned to
 * @param value Node that holds the value being assigned
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Assignment node
 */
astNode *astNewAssign(astNode *target, astNode *value, int line, int col) {
	astNode *node = astNewNode(AST_ASSIGN, line, col);

	node->data.assign.target = target;
	node->data.assign.value = value;

	return node;
}

/**
 * @brief Creates a node of type Return
 * @param value Node that holds the value being returned
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Return node
 */
astNode *astNewReturn(astNode *value, int line, int col) {
	astNode *node = astNewNode(AST_RETURN, line, col);

	node->data.ret.value = value;

	return node;
}

/**
 * @brief Creates a node of type While
 * @param cond Node holding the condition of the loop
 * @param body Node holding the body of the loop
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new While node
 */
astNode *astNewWhile(astNode *cond, astNode *body, int line, int col) {
	astNode *node = astNewNode(AST_WHILE, line, col);

	node->data.whileLoop.cond = cond;
	node->data.whileLoop.body = body;

	return node;
}

/**
 * @brief Creates a node of type For
 * @param iterName Iterator name
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new For node
 */
astNode *astNewFor(char *iterName, int line, int col) {
	astNode *node = astNewNode(AST_FOR, line, col);

	node->data.forLoop.iterName = iterName;
	node->data.forLoop.from = NULL;
	node->data.forLoop.to = NULL;
	node->data.forLoop.step = NULL;
	node->data.forLoop.isInclusive = false;
	astListInit(&node->data.forLoop.guards);
	node->data.forLoop.body = NULL;

	return node;
}

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
				  int col) {
	astNode *node = astNewNode(AST_IF, line, col);

	node->data.ifStmt.cond = cond;
	node->data.ifStmt.thenBody = thenBody;
	node->data.ifStmt.elseBody = elseBody;

	return node;
}

/**
 * @brief Creates a node of type Expression statement
 * @param expr Node holding the expression
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Expression statement node
 */
astNode *astNewExprStmt(astNode *expr, int line, int col) {
	astNode *node = astNewNode(AST_EXPR_STMT, line, col);

	node->data.exprStmt.expr = expr;

	return node;
}

/**
 * @brief Creates a node of type Integer literal
 * @param value value of the integer
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Integer literal node
 */
astNode *astNewIntLit(long long value, int line, int col) {
	astNode *node = astNewNode(AST_LIT_INT, line, col);

	node->data.intValue = value;

	return node;
}

/**
 * @brief Creates a node of type Double literal
 * @param value value of the double
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Double literal node
 */
astNode *astNewDoubleLit(double value, int line, int col) {
	astNode *node = astNewNode(AST_LIT_DOUBLE, line, col);

	node->data.doubleValue = value;

	return node;
}

/**
 * @brief Creates a node of type String literal
 * @param value value of the string
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new String literal node
 */
astNode *astNewStringLit(char *value, int line, int col) {
	astNode *node = astNewNode(AST_LIT_STRING, line, col);

	node->data.stringValue = value;

	return node;
}

/**
 * @brief Creates a node of type Boolean literal
 * @param value value of the boolean
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Boolean literal node
 */
astNode *astNewBoolLit(bool value, int line, int col) {
	astNode *node = astNewNode(AST_LIT_BOOL, line, col);

	node->data.boolValue = value;

	return node;
}

/**
 * @brief Creates a node of type Unit literal
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Unit literal node
 */
astNode *astNewUnitLit(int line, int col) {
	return astNewNode(AST_LIT_UNIT, line, col);
}

/**
 * @brief Creates a node of type Identifier
 * @param name Name of the identifier
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Identifier node
 */
astNode *astNewIdentifier(char *name, int line, int col) {
	astNode *node = astNewNode(AST_IDENTIFIER, line, col);

	node->data.identifier.name = name;

	return node;
}

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
					  int col) {
	astNode *node = astNewNode(AST_BINARY_OP, line, col);

	node->data.binary.op = op;
	node->data.binary.left = left;
	node->data.binary.right = right;

	return node;
}

/**
 * @brief Creates a node of type Unary operation
 * @param op Operation being performed
 * @param operand Operand the operation is being performed on
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Unary operation node
 */
astNode *astNewUnary(specTokenType op, astNode *operand, int line, int col) {
	astNode *node = astNewNode(AST_UNARY_OP, line, col);

	node->data.unary.op = op;
	node->data.unary.operand = operand;

	return node;
}

/**
 * @brief Creates a node of type Call
 * @param callee Function being called
 * @param line Specifies a line in the input file
 * @param col Specifies a col in the input
 * @return Pointer to the new Call node
 */
astNode *astNewCall(astNode *callee, int line, int col) {
	astNode *node = astNewNode(AST_CALL, line, col);

	node->data.call.callee = callee;
	astListInit(&node->data.call.args);

	return node;
}

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
astNode *astNewMember(astNode *object, char *name, int line, int col) {
	astNode *node = astNewNode(AST_MEMBER, line, col);

	node->data.member.object = object;
	node->data.member.name = name;

	return node;
}

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
astNode *astNewInterp(int line, int col) {
	astNode *node = astNewNode(AST_INTERP, line, col);

	astListInit(&node->data.interp.parts);

	return node;
}

/**
 * @brief Helper function for getting type from a token that
 * 		  scanner returned
 * @param specType Token type returned from the scanner
 * @return Matching AST data type, TYPE_NONE if the token is not a type
 */
astDataType astTypeFromToken(specTokenType specType) {
	switch (specType) {
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
