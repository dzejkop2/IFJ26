/**
 * @name ifj26
 * @authors Tomáš Zajac (xzajact00)
 * @file semantic.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "semantic.h"

static int scopeIdCounter = 0;

static void analyzeNodeList(astNodeList nodeList, Scope *scope);
static void analyzeNode(astNode *node, Scope *scope);
static void analyzeBinaryOp(astNode *node, Scope *scope);
static void analyzeUnaryOp(astNode *node, Scope *scope);
static void analyzeNode(astNode *node, Scope *scope);
static void analyzeIdentifier(astNode *node, Scope *scope);
static void analyzeVarDecl(astNode *node, Scope *scope);
static void analyzeBlock(astNode *node, Scope *parentScope);
static void analyzeCall(astNode *node, Scope *scope);
static void analyzeFunction(astNode *node, Scope *scope, astNode *receiver);
static void registerFunction(astNode *node, Scope *scope, astNode *receiver);
static void analyzeAssign(astNode *node, Scope *scope);
static void analyzeFor(astNode *node, Scope *scope);
static void analyzeWhile(astNode *node, Scope *scope);
static void registerExtension(astNode *node, Scope *scope);
static void analyzeExtension(astNode *node, Scope *scope);
static char *makeExtensionPrefix(astDataType type, const char *name);
static void analyzeMember(astNode *node, Scope *scope);
static AVLNode *lookupExtension(astNode *node, Scope *scope);
static void analyzeReturn(astNode *node, Scope *scope);
static void analyzeInterp(astNode *node, Scope *scope);

static void analyzeNodeList(astNodeList nodeList, Scope *scope) {
	for (size_t i = 0; i < nodeList.count; i++) {
		analyzeNode(nodeList.items[i], scope);
	}
}

void semanticAnalyze(astNode *root) {
	if (root == NULL || root->type != AST_PROGRAM) {
		error(INTERNAL_ERROR, "Root node is not a program");
	}

	Scope globalScope = {.symbols = initTree(),
						 .parent = NULL,
						 .id = scopeIdCounter++,
						 .depth = 0};

	analyzeNode(root, &globalScope);
}

static void analyzeNode(astNode *node, Scope *scope) {
	if (node == NULL) {
		return;
	}

	switch (node->type) {
	case AST_PROGRAM:

		for (size_t i = 0; i < node->data.program.defs.count; i++) {
			astNode *def = node->data.program.defs.items[i];

			if (def->type == AST_FUN_DEF) {
				registerFunction(def, scope, NULL);
			} else if (def->type == AST_EXTENSION) {
				registerExtension(def, scope);
			}
		}
		analyzeNodeList(node->data.program.defs, scope);
		break;

	case AST_FUN_DEF:
		analyzeFunction(node, scope, NULL);
		break;
	case AST_EXTENSION:
		analyzeExtension(node, scope);

		break;
	case AST_PARAM_LIST:
		analyzeNodeList(node->data.paramList.params, scope);
		break;

	case AST_PARAM:
		break;

	case AST_BLOCK:
		analyzeBlock(node, scope);
		break;
	case AST_VAR_DECL:
		analyzeVarDecl(node, scope);
		break;
	case AST_ASSIGN:
		analyzeAssign(node, scope);
		break;
	case AST_RETURN:
		analyzeReturn(node, scope);
		break;

	case AST_WHILE:
		analyzeWhile(node, scope);
		break;
	case AST_FOR:
		analyzeFor(node, scope);

		break;
	case AST_IF:
		analyzeNode(node->data.ifStmt.cond, scope);
		if (node->data.ifStmt.cond->dataType != TYPE_BOOL) {
			error(SEMANTIC_TYPE_ERROR, "If condition must be of type bool");
		}
		analyzeNode(node->data.ifStmt.thenBody, scope);
		analyzeNode(node->data.ifStmt.elseBody, scope);
		break;
	case AST_EXPR_STMT:
		analyzeNode(node->data.exprStmt.expr, scope);
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
		analyzeIdentifier(node, scope);
		break;
	case AST_BINARY_OP:
		analyzeNode(node->data.binary.left, scope);
		analyzeNode(node->data.binary.right, scope);
		analyzeBinaryOp(node, scope);
		break;
	case AST_UNARY_OP:
		analyzeNode(node->data.unary.operand, scope);
		analyzeUnaryOp(node, scope);
		break;
	case AST_CALL:
		analyzeCall(node, scope);
		break;
	case AST_MEMBER:
		analyzeMember(node, scope);
		break;
	case AST_INTERP:
		analyzeInterp(node, scope);
		break;
	default:
		error(INTERNAL_ERROR, "Unknown AST node type");
		break;
	}
}

static AVLNode *searchSymbolInScope(Scope *scope, char *name) {
	Scope *currentScope = scope;
	if (currentScope == NULL) {
		return NULL;
	}
	while (currentScope != NULL) {
		AVLNode *symbol = searchNode(currentScope->symbols, name);
		if (symbol != NULL) {
			return symbol;
		}
		currentScope = currentScope->parent;
	}

	return NULL;
}

static void analyzeAssign(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_ASSIGN) {
		error(INTERNAL_ERROR, "Invalid assignment node");
		return;
	}

	analyzeNode(node->data.assign.target, scope);
	analyzeNode(node->data.assign.value, scope);

	if (node->data.assign.target->dataType !=
			node->data.assign.value->dataType &&
		!(node->data.assign.target->dataType == TYPE_DOUBLE &&
		  node->data.assign.value->dataType == TYPE_INT)) {
		error(SEMANTIC_TYPE_ERROR, "Assignment type mismatch");
	}
	AVLNode *symbol = searchSymbolInScope(
		scope, node->data.assign.target->data.identifier.name);
	if (symbol->params.is_mutable == false) {
		error(SEMANTIC_UNSUPPORTED_MODIFICATON_ERROR,
			  "Assignment to immutable variable");
	}
}

static void analyzeBlock(astNode *node, Scope *parentScope) {
	if (node == NULL || node->type != AST_BLOCK) {
		error(INTERNAL_ERROR, "Invalid block node");
		return;
	}

	Scope blockScope = {.symbols = initTree(),
						.parent = parentScope,
						.id = scopeIdCounter++,
						.depth = parentScope->depth + 1};

	analyzeNodeList(node->data.block.stmts, &blockScope);
}

static void analyzeVarDecl(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_VAR_DECL) {
		error(INTERNAL_ERROR, "Invalid variable declaration");
		return;
	}

	astNode *valueNode = node->data.varDecl.value;
	astDataType declType = node->data.varDecl.declType;

	if (valueNode == NULL) {
		error(SEMANTIC_TYPE_ERROR, "Variable declaration without value");
		return;
	}
	analyzeNode(node->data.varDecl.value, scope);
	if (valueNode->dataType == TYPE_NONE) {
		error(SEMANTIC_TYPE_ERROR, "Variable declaration with TYPE_NONE value");
		return;
	}

	if (declType != TYPE_NONE && valueNode->dataType != declType &&
		!(declType == TYPE_DOUBLE && valueNode->dataType == TYPE_INT)) {

		error(SEMANTIC_TYPE_ERROR, "Variable declaration type mismatch");
		return;
	}

	AVLNode *existingSymbol =
		searchNode(scope->symbols, node->data.varDecl.name);

	if (existingSymbol != NULL) {
		error(SEMANTIC_REDEFINITION_ERROR, "Variable redeclaration");
		return;
	}
	paramInfo info = {.name = node->data.varDecl.name,
					  .type = declType,
					  .node_type = VARIABLE,
					  .is_mutable = node->data.varDecl.isMutable,
					  .param_count = 0,
					  .param_list = NULL,
					  .scope = scope->id,
					  .depth = scope->depth};

	scope->symbols = insertNode(scope->symbols, info);
	node->dataType = declType;

	return;
}

static void analyzeIdentifier(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_IDENTIFIER) {
		error(INTERNAL_ERROR, "Invalid identifier node");
		return;
	}

	AVLNode *symbol = searchSymbolInScope(scope, node->data.identifier.name);
	if (symbol == NULL) {
		error(SEMANTIC_UNDEFINITION_ERROR,
			  "Identifier used before declaration wasd");
		return;
	}

	node->dataType = symbol->params.type;
}

static void analyzeFunction(astNode *node, Scope *scope, astNode *receiver) {
	Scope functionScope = {.symbols = initTree(),
						   .parent = scope,
						   .id = scopeIdCounter++,
						   .depth = scope->depth + 1,
						   .closingFunction = node};

	if (receiver != NULL) {
		if (receiver->type != AST_PARAM ||
			receiver->data.param.type == TYPE_NONE) {
			error(SEMANTIC_TYPE_ERROR, "Invalid extension receiver");
		}

		paramInfo info = {.name = receiver->data.param.name,
						  .type = receiver->data.param.type,
						  .node_type = VARIABLE,
						  .is_mutable = false,
						  .param_count = 0,
						  .param_list = NULL,
						  .scope = functionScope.id,
						  .depth = functionScope.depth};

		functionScope.symbols = insertNode(functionScope.symbols, info);
	}
	// Register function parameters
	for (size_t i = 0; i < node->data.funcDef.paramLists.count; i++) {

		astNode *list = node->data.funcDef.paramLists.items[i];

		for (size_t j = 0; j < list->data.paramList.params.count; j++) {

			astNode *param = list->data.paramList.params.items[j];

			paramInfo info = {.name = param->data.param.name,
							  .type = param->data.param.type,
							  .node_type = VARIABLE,
							  .is_mutable = false,
							  .param_count = 0,
							  .param_list = NULL,
							  .scope = functionScope.id,
							  .depth = functionScope.depth};

			if (searchNode(functionScope.symbols, info.name) != NULL) {
				error(SEMANTIC_REDEFINITION_ERROR,
					  "Duplicate function parameter");
			}

			functionScope.symbols = insertNode(functionScope.symbols, info);
		}
	}

	// Analyze function body
	analyzeNode(node->data.funcDef.body, &functionScope);

	while (functionScope.symbols != NULL) {
		AVLNode *symbol = functionScope.symbols;
		if (symbol->left != NULL) {
			AVLNode *left = symbol->left;
			symbol->left = left->right;
			left->right = symbol;
			functionScope.symbols = left;
		} else {
			functionScope.symbols = symbol->right;
			for (size_t i = 0; i < symbol->params.param_count; i++) {
				free(symbol->params.param_list[i].name);
			}
			free(symbol->params.param_list);
			free(symbol->params.name);
			free(symbol);
		}
	}
}

static void analyzeCall(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_CALL) {
		error(INTERNAL_ERROR, "Invalid function call node");
		return;
	}

	astNode *callee = node->data.call.callee;

	if (node->data.call.callee->type != AST_IDENTIFIER &&
		node->data.call.callee->type != AST_MEMBER) {
		error(INTERNAL_ERROR, "Unsupported callee type");
		return;
	}
	AVLNode *callFunc = NULL;
	if (callee->type == AST_IDENTIFIER) {
		callFunc = searchSymbolInScope(
			scope, node->data.call.callee->data.identifier.name);
	} else if (callee->type == AST_MEMBER) {

		// Extension method
		callFunc = lookupExtension(callee, scope);

	} else {
		error(INTERNAL_ERROR, "Unsupported callee type");
		return;
	}

	if (callFunc == NULL) {
		error(SEMANTIC_UNDEFINITION_ERROR, "Function used before declaration");
		return;
	}

	if (callFunc->params.node_type != FUNCTION) {
		error(SEMANTIC_TYPE_ERROR, "Identifier is not a function");
		return;
	}

	analyzeNodeList(node->data.call.args, scope);
	fprintf(stderr, "DEBUG: analyzeCall reached\n");

	if (node->data.call.args.count != callFunc->params.param_count) {
		error(SEMANTIC_TYPE_ERROR, "Function call argument count mismatch");
		return;
	}

	if (node->data.call.args.count == 0) {
		node->dataType = callFunc->params.type;
		return;
	}

	for (size_t i = 0; i < node->data.call.args.count; i++) {

		if (node->data.call.args.items[i]->dataType !=
			callFunc->params.param_list[i].type) {
			error(SEMANTIC_TYPE_ERROR, "Function call argument type mismatch");
			return;
		}
	}

	node->dataType = callFunc->params.type;

	return;
}

static void analyzeBinaryOp(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_BINARY_OP) {
		error(INTERNAL_ERROR, "Invalid binary operation");
		return;
	}

	analyzeNode(node->data.binary.left, scope);
	analyzeNode(node->data.binary.right, scope);

	astDataType leftType = node->data.binary.left->dataType;
	astDataType rightType = node->data.binary.right->dataType;
	switch (node->data.binary.op) {
	case op_add:
		if (leftType == TYPE_STRING && rightType == TYPE_STRING) {
			node->dataType = TYPE_STRING;
		} else if (leftType == TYPE_INT && rightType == TYPE_INT) {
			node->dataType = TYPE_INT;
		} else if ((leftType == TYPE_DOUBLE && rightType == TYPE_DOUBLE) ||
				   (leftType == TYPE_DOUBLE && rightType == TYPE_INT) ||
				   (leftType == TYPE_INT && rightType == TYPE_DOUBLE)) {
			node->dataType = TYPE_DOUBLE;
		} else {
			error(SEMANTIC_TYPE_ERROR, "Invalid types for addition");
		}
		break;
	case op_sub:
	case op_mul:
	case op_div:
		if (leftType == TYPE_INT && rightType == TYPE_INT) {
			node->dataType = TYPE_INT;
		} else if ((leftType == TYPE_DOUBLE && rightType == TYPE_DOUBLE) ||
				   (leftType == TYPE_DOUBLE && rightType == TYPE_INT) ||
				   (leftType == TYPE_INT && rightType == TYPE_DOUBLE)) {
			node->dataType = TYPE_DOUBLE;
		} else {
			error(SEMANTIC_TYPE_ERROR,
				  "Invalid types for arithmetic operation");
		}
		break;

	case op_equal:
	case op_notEqual:
		if (((leftType == TYPE_INT || leftType == TYPE_DOUBLE) &&
			 (rightType == TYPE_INT || rightType == TYPE_DOUBLE)) ||
			leftType == rightType) {
			node->dataType = TYPE_BOOL;
		} else {
			error(SEMANTIC_TYPE_ERROR,
				  "Invalid types for comparison operation");
		}
		break;
	case op_biggerThen:
	case op_lowerThen:
	case op_lowerEqual:
	case op_biggerEqual:
		if ((leftType == TYPE_INT || leftType == TYPE_DOUBLE) &&
			(rightType == TYPE_INT || rightType == TYPE_DOUBLE)) {
			node->dataType = TYPE_BOOL;
		} else {
			error(SEMANTIC_TYPE_ERROR,
				  "Invalid types for comparison operation");
		}
		break;
	case op_and:
	case op_or:
		if (leftType == TYPE_BOOL && rightType == TYPE_BOOL) {
			node->dataType = TYPE_BOOL;
		} else {
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

static void analyzeUnaryOp(astNode *node, Scope *scope) {

	if (node == NULL || node->type != AST_UNARY_OP) {
		error(INTERNAL_ERROR, "Invalid unary operation");
		return;
	}

	analyzeNode(node->data.unary.operand, scope);

	astDataType operandType = node->data.unary.operand->dataType;

	switch (node->data.unary.op) {
	case op_not:
		if (operandType == TYPE_BOOL) {
			node->dataType = TYPE_BOOL;
		} else {
			error(SEMANTIC_TYPE_ERROR,
				  "Invalid type for logical NOT operation");
		}
		break;
	case op_sub:
		if (operandType == TYPE_INT || operandType == TYPE_DOUBLE) {
			node->dataType =
				operandType; // Result type is the same as operand type
		} else {
			error(SEMANTIC_TYPE_ERROR,
				  "Invalid type for unary negation operation");
		}
		break;
	case op_add:
		if (operandType != TYPE_INT && operandType != TYPE_DOUBLE) {
			error(SEMANTIC_TYPE_ERROR, "Invalid type for unary plus operation");
		} else {
			node->dataType =
				operandType; // Result type is the same as operand type
		}
		break;
	default:
		// Handle other unary operations as needed
		break;
	}
}

static void registerFunction(astNode *node, Scope *scope, astNode *receiver) {
	if (node == NULL || node->type != AST_FUN_DEF) {
		error(INTERNAL_ERROR, "Invalid function definition");
	}
	char *symbolName = node->data.funcDef.name;
	char *extensionKey = NULL;

	if (receiver != NULL) {
		extensionKey = makeExtensionPrefix(receiver->data.param.type,
										   node->data.funcDef.name);

		symbolName = extensionKey;
	}
	// Check duplicate function declaration in current scope.
	if (searchNode(scope->symbols, symbolName) != NULL) {
		error(SEMANTIC_REDEFINITION_ERROR, "Function already defined");
	}

	// Count parameters across parameter lists.
	size_t paramCount = 0;

	for (size_t i = 0; i < node->data.funcDef.paramLists.count; i++) {
		astNode *list = node->data.funcDef.paramLists.items[i];

		if (list == NULL || list->type != AST_PARAM_LIST) {
			error(INTERNAL_ERROR, "Invalid parameter list");
		}

		paramCount += list->data.paramList.params.count;
	}

	argInfo *args = NULL;

	if (paramCount > 0) {
		args = calloc(paramCount, sizeof(argInfo));

		if (args == NULL) {
			error(INTERNAL_ERROR, "Memory allocation failed");
		}
	}

	// Copy parameter descriptions from AST.
	size_t index = 0;

	for (size_t i = 0; i < node->data.funcDef.paramLists.count; i++) {
		astNode *list = node->data.funcDef.paramLists.items[i];

		for (size_t j = 0; j < list->data.paramList.params.count; j++) {
			astNode *paramNode = list->data.paramList.params.items[j];

			if (paramNode == NULL || paramNode->type != AST_PARAM) {
				error(INTERNAL_ERROR, "Invalid parameter");
			}
			if (paramNode->data.param.name == NULL) {
				error(INTERNAL_ERROR, "Parameter name is NULL");
			}
			if (paramNode->data.param.type == TYPE_NONE) {
				error(INTERNAL_ERROR, "Parameter type is TYPE_NONE");
			}

			args[index].name = paramNode->data.param.name;
			args[index].type = paramNode->data.param.type;
			args[index].is_mutable = false;

			index++;
		}
	}

	paramInfo info = {.name = symbolName,
					  .type = node->data.funcDef.retType,
					  .node_type = FUNCTION,
					  .is_mutable = false,
					  .param_count = paramCount,
					  .param_list = args,
					  .scope = scope->id,
					  .depth = scope->depth};

	scope->symbols = insertNode(scope->symbols, info);

	// insertNode() currently makes a deep copy of parameter data.
	free(args);
	free(extensionKey);
}

static void analyzeFor(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_FOR) {
		error(INTERNAL_ERROR, "Invalid for loop node");
		return;
	}
	astNode *fromNode = node->data.forLoop.from;
	astNode *toNode = node->data.forLoop.to;
	astNode *stepNode = node->data.forLoop.step;
	astNode *bodyNode = node->data.forLoop.body;

	if (fromNode == NULL || toNode == NULL || bodyNode == NULL ||
		node->data.forLoop.iterName == NULL) {
		error(INTERNAL_ERROR, "Invalid for loop structure");
		return;
	}

	analyzeNode(fromNode, scope);
	analyzeNode(toNode, scope);

	if (fromNode->dataType != TYPE_INT || toNode->dataType != TYPE_INT) {
		error(SEMANTIC_TYPE_ERROR,
			  "For loop 'from' and 'to' expressions must be of type int");
	}
	if (stepNode != NULL) {
		analyzeNode(stepNode, scope);
		if (stepNode->dataType != TYPE_INT) {
			error(SEMANTIC_TYPE_ERROR,
				  "For loop 'step' expression must be of type int");
		}
		// stepNode->data.intValue = 1;
		if (stepNode->data.intValue <= 0) {
			error(SEMANTIC_TYPE_ERROR,
				  "For loop 'step' expression cannot be zero");
		}
	}

	Scope forScope = {.symbols = initTree(),
					  .parent = scope,
					  .id = scopeIdCounter++,
					  .depth = scope->depth + 1};

	paramInfo iterInfo = {.name = node->data.forLoop.iterName,
						  .type = TYPE_INT,
						  .node_type = VARIABLE,
						  .is_mutable = false,
						  .param_count = 0,
						  .param_list = NULL,
						  .scope = forScope.id,
						  .depth = forScope.depth};

	forScope.symbols = insertNode(forScope.symbols, iterInfo);

	for (size_t guardIndex = 0; guardIndex < node->data.forLoop.guards.count;
		 guardIndex++) {
		astNode *guardNode = node->data.forLoop.guards.items[guardIndex];
		if (guardNode == NULL) {
			error(INTERNAL_ERROR, "Invalid for loop guard");
		}
		analyzeNode(guardNode, &forScope);
		if (guardNode->dataType != TYPE_BOOL) {
			error(SEMANTIC_TYPE_ERROR,
				  "For loop guard expressions must be of type bool");
		}
	}
	analyzeNode(bodyNode, &forScope);
}

static void analyzeWhile(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_WHILE) {
		error(INTERNAL_ERROR, "Invalid while loop node");
		return;
	}

	if (node->data.whileLoop.cond == NULL ||
		node->data.whileLoop.body == NULL) {
		error(INTERNAL_ERROR, "Invalid while loop structure");
		return;
	}

	analyzeNode(node->data.whileLoop.cond, scope);
	if (node->data.whileLoop.cond->dataType != TYPE_BOOL) {
		error(SEMANTIC_TYPE_ERROR, "While condition must be of type bool");
	}
	Scope whileScope = {.symbols = initTree(),
						.parent = scope,
						.id = scopeIdCounter++,
						.depth = scope->depth + 1};

	paramInfo iterInfo = {.name = node->data.forLoop.iterName,
						  .type = TYPE_INT,
						  .node_type = VARIABLE,
						  .is_mutable = false,
						  .param_count = 0,
						  .param_list = NULL,
						  .scope = whileScope.id,
						  .depth = whileScope.depth};

	whileScope.symbols = insertNode(whileScope.symbols, iterInfo);

	analyzeNode(node->data.whileLoop.body, &whileScope);
}

static void registerExtension(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_EXTENSION) {
		error(INTERNAL_ERROR, "Invalid extension node");
		return;
	}

	astNode *receiver = node->data.extension.receiver;

	if (receiver == NULL || receiver->type != AST_PARAM ||
		receiver->data.param.type == TYPE_NONE) {
		error(SEMANTIC_TYPE_ERROR, "Invalid extension receiver");
	}

	for (size_t i = 0; i < node->data.extension.funDefs.count; i++) {
		astNode *method = node->data.extension.funDefs.items[i];

		registerFunction(method, scope, receiver);
	}
}

static void analyzeExtension(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_EXTENSION) {
		error(INTERNAL_ERROR, "Invalid extension node");
		return;
	}

	if (node->data.extension.receiver == NULL) {
		error(INTERNAL_ERROR, "Invalid extension structure");
		return;
	}

	astNode *receiver = node->data.extension.receiver;

	for (size_t i = 0; i < node->data.extension.funDefs.count; i++) {
		astNode *method = node->data.extension.funDefs.items[i];

		analyzeFunction(method, scope, receiver);
	}
}

static char *makeExtensionPrefix(astDataType type, const char *name) {
	int len = snprintf(NULL, 0, "#EXT:%d:%s", (int)type, name);

	if (len < 0) {
		error(INTERNAL_ERROR, "Cannot create extension prefix string");
	}

	char *key = malloc((size_t)len + 1);

	if (key == NULL) {
		error(INTERNAL_ERROR, "Memory allocation failed");
	}

	snprintf(key, (size_t)len + 1, "#EXT:%d:%s", (int)type, name);

	return key;
}

static void analyzeMember(astNode *node, Scope *scope) {
	AVLNode *method = lookupExtension(node, scope);

	if (method == NULL) {
		return;
	}

	node->dataType = method->params.type;
	return;
}

static AVLNode *lookupExtension(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_MEMBER) {
		error(INTERNAL_ERROR, "Invalid member node");
		return NULL;
	}

	astNode *object = node->data.member.object;

	if (object == NULL || node->data.member.name == NULL) {
		error(INTERNAL_ERROR, "Invalid member structure");
		return NULL;
	}

	// 1. Analyze the object
	analyzeNode(object, scope);

	if (object->dataType == TYPE_NONE) {
		error(SEMANTIC_TYPE_ERROR, "Unknown member object type");
		return NULL;
	}

	// 3. Generate extension key
	char *key = makeExtensionPrefix(object->dataType, node->data.member.name);

	// 4. Search extension method
	AVLNode *symbol = searchSymbolInScope(scope, key);

	free(key);

	if (symbol == NULL) {
		error(SEMANTIC_UNDEFINITION_ERROR, "Undefined extension method");
		return NULL;
	}

	if (object->dataType != symbol->params.type) {
		error(SEMANTIC_TYPE_ERROR, "");
		return NULL;
	}

	if (symbol->params.node_type != FUNCTION) {
		error(SEMANTIC_TYPE_ERROR, "Member is not a function");
		return NULL;
	}

	return symbol;
}

static void analyzeReturn(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_RETURN) {
		error(INTERNAL_ERROR, "Invalid return node");
		return;
	}

	astNode *function = NULL;
	Scope *current = scope;
	while (current != NULL) {

		if (current->closingFunction != NULL) {
			function = current->closingFunction;
			break;
		}
		current = current->parent;
	}
	if (function == NULL) {
		error(10, "Return outside function");
		return;
	}

	astDataType expectedType = function->data.funcDef.retType;

	if (expectedType == TYPE_NONE) {
		error(SEMANTIC_TYPE_ERROR,
			  "Function with return must declare return type");
		return;
	}

	astNode *value = node->data.ret.value;

	// Return without expression
	if (value == NULL) {
		if (expectedType != TYPE_UNIT) {
			error(SEMANTIC_TYPE_ERROR, "Missing return value");
			return;
		}

		node->dataType = TYPE_UNIT;
		return;
	}

	// Analyze returned expression
	analyzeNode(value, scope);

	astDataType actualType = value->dataType;

	if (actualType == TYPE_NONE) {
		error(SEMANTIC_TYPE_ERROR, "Invalid return expression");
		return;
	}

	// Unit function discards the returned value
	if (expectedType == TYPE_UNIT) {
		node->dataType = TYPE_UNIT;
		return;
	}

	// Check type compatibility
	if (actualType != expectedType &&
		!(expectedType == TYPE_DOUBLE && actualType == TYPE_INT)) {

		error(SEMANTIC_TYPE_ERROR, "Return type mismatch");
		return;
	}

	node->dataType = TYPE_UNIT;
}

static void analyzeInterp(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_INTERP) {
		error(INTERNAL_ERROR, "Invalid interpolation node");
		return;
	}

	for (size_t i = 0; i < node->data.interp.parts.count; i++) {
		astNode *part = node->data.interp.parts.items[i];

		if (part == NULL) {
			error(INTERNAL_ERROR, "Invalid interpolation part");
			return;
		}

		analyzeNode(part, scope);

		if (part->dataType == TYPE_NONE) {
			error(SEMANTIC_TYPE_ERROR, "Invalid interpolation expression");
			return;
		}
	}

	node->dataType = TYPE_STRING;
}
