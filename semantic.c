/**
 * @name ifj26
 * @authors Tomáš Zajac (xzajact00)
 * @file semantic.c
 *
 * AI usage:
 * "Concsutation about about semantic analysis strucsture"
 * "Generation of test set for semantic analysis"
 * "Assistance with debugging code"
 */

#include "semantic.h"
#include <stdio.h>
#include <stdlib.h>

static int scopeIdCounter = 0;
static astNode *mainNode = NULL;
static astNode *rootNode = NULL;

static void analyzeNodeList(astNodeList nodeList, Scope *scope);
static void analyzeNode(astNode *node, Scope *scope);
static void analyzeBinaryOp(astNode *node, Scope *scope);
static void analyzeUnaryOp(astNode *node, Scope *scope);
static void analyzeIdentifier(astNode *node, Scope *scope);
static void analyzeVarDecl(astNode *node, Scope *scope);
static void analyzeBlock(astNode *node, Scope *parentScope);
static void analyzeCall(astNode *node, Scope *scope);
static void analyzeFunction(astNode *node, Scope *scope, astNode *receiver);
static void registerFunction(astNode *node, Scope *scope, astNode *receiver);
static void analyzeAssign(astNode *node, Scope *scope);
static void analyzeIf(astNode *node, Scope *scope);
static void analyzeFor(astNode *node, Scope *scope);
static void analyzeWhile(astNode *node, Scope *scope);
static void registerExtension(astNode *node, Scope *scope);
static void analyzeExtension(astNode *node, Scope *scope);
static char *makeExtensionPrefix(astDataType type, const char *name,
								 const astNode *node, Scope *scope);
static void analyzeMember(astNode *node, Scope *scope);
static AVLNode *lookupExtension(astNode *node, Scope *scope);
static void analyzeReturn(astNode *node, Scope *scope);
static void analyzeInterp(astNode *node, Scope *scope);
static void registerBuiltins(Scope *scope);
static void registerBuiltin(Scope *scope, char *name, astDataType returnType,
							size_t paramCount, argInfo *params);
static bool alwaysReturns(const astNode *node);
static void registerBuiltinExtension(Scope *scope, astDataType receiverType,
									 const char *name, astDataType returnType);
static bool lastStatementReturns(const astNode *body);
static void destroyFullAVL(Scope *scope);
static void analyzeFunctionLazy(astNode *definition, Scope *scope,
								astNode *receiver);

typedef struct SuspendedScope {
	Scope *scope;
	struct SuspendedScope *prev;
} SuspendedScope;

static SuspendedScope *suspendedScopes = NULL;

/**
 * @brief Reports a semantic error and releases the AST and symbol tables.
 * @param code Error exit code.
 * @param message Error message.
 * @param node AST node used to determine the source location.
 * @param scope Scope in which the error occurred.
 */
static void semanticError(int code, const char *message, const astNode *node,
						  Scope *scope) {
	char diagnostic[512];
	if (node != NULL) {
		snprintf(diagnostic, sizeof(diagnostic), "%s (line %d, column %d)",
				 message, node->line, node->col);
	} else {
		snprintf(diagnostic, sizeof(diagnostic),
				 "%s (source location unavailable)", message);
	}
	// free only after the message is built, node is part of the AST
	astFree(rootNode);
	rootNode = NULL;
	destroyFullAVL(scope);
	error(code, diagnostic);
}

/**
 * @brief Register built-in function into AVLtree with scope 0
 * @param scope Pointer to root scope
 * @param name Pointer to string that will be assigned as a name in AVLtree
 * @param returnType Return type of built-in function for AVL
 * @param paramCount Number of function parameters for AVL
 * @param params Array of argument informations of each function param
 */
static void registerBuiltin(Scope *scope, char *name, astDataType returnType,
							size_t paramCount, argInfo *params) {

	if (scope == NULL || name == NULL) {
		error(INTERNAL_ERROR, "Invalid builtin registration");
		return;
	}

	if (searchNode(scope->symbols, name) != NULL) {
		error(SEMANTIC_REDEFINITION_ERROR, "Builtin function already defined");
		return;
	}

	paramInfo info = {.name = name,
					  .type = returnType,
					  .node_type = FUNCTION,
					  .is_mutable = false,
					  .param_count = paramCount,
					  .param_list = params,
					  .scope = scope->id,
					  .depth = scope->depth};

	scope->symbols = insertNode(scope->symbols, info);
}

/**
 * @brief Register built-in extension into AVLtree with scope 0
 * @param scope Pointer to root scope
 * @param receiverType Datatype that is being extended
 * @param name Pointer to string that will be assigned as a name in AVLtree
 * @param returnType Return type of built-in extension for AVL
 */
static void registerBuiltinExtension(Scope *scope, astDataType receiverType,
									 const char *name, astDataType returnType) {
	char *key = makeExtensionPrefix(receiverType, name, NULL, scope);

	if (key == NULL) {
		error(INTERNAL_ERROR, "Cannot register builtin extension");
		return;
	}

	if (searchNode(scope->symbols, key) != NULL) {
		free(key);
		error(SEMANTIC_REDEFINITION_ERROR,
			  "Builtin extension already registered");
		return;
	}

	paramInfo info = {.name = key,
					  .type = returnType,
					  .node_type = FUNCTION,
					  .is_mutable = false,
					  .param_count = 0,
					  .param_list = NULL,
					  .scope = scope->id,
					  .depth = scope->depth,
					  .resolveState = TYPE_RESOLVED,
					  .definition = NULL,
					  .receiver = NULL};

	scope->symbols = insertNode(scope->symbols, info);

	free(key);
}

/**
 * @brief Registers all built-in extensions and functions into scope 0
 * @param scope Scope that we are inserting built-ins into
 */
static void registerBuiltins(Scope *scope) {
	registerBuiltin(
		scope, "print", TYPE_UNIT, 1,
		(argInfo[]){{.name = "value", .type = TYPE_ANY, .is_mutable = false}});

	registerBuiltin(scope, "readLine", TYPE_STRING, 0, NULL);

	registerBuiltin(scope, "readInt", TYPE_INT, 0, NULL);

	registerBuiltin(scope, "readDouble", TYPE_DOUBLE, 0, NULL);

	registerBuiltin(scope, "readBoolean", TYPE_BOOL, 0, NULL);

	registerBuiltinExtension(scope, TYPE_DOUBLE, "toInt", TYPE_INT);

	registerBuiltinExtension(scope, TYPE_DOUBLE, "floor", TYPE_DOUBLE);

	registerBuiltinExtension(scope, TYPE_STRING, "length", TYPE_INT);

	registerBuiltinExtension(scope, TYPE_INT, "toString", TYPE_STRING);

	registerBuiltinExtension(scope, TYPE_DOUBLE, "toString", TYPE_STRING);

	registerBuiltinExtension(scope, TYPE_BOOL, "toString", TYPE_STRING);

	registerBuiltinExtension(scope, TYPE_STRING, "toString", TYPE_STRING);

	registerBuiltinExtension(scope, TYPE_UNIT, "toString", TYPE_STRING);

	registerBuiltin(
		scope, "subStr", TYPE_STRING, 3,
		(argInfo[]){{.name = "s", .type = TYPE_STRING, .is_mutable = false},
					{.name = "i", .type = TYPE_INT, .is_mutable = false},
					{.name = "j", .type = TYPE_INT, .is_mutable = false}});

	registerBuiltin(
		scope, "ord", TYPE_INT, 2,
		(argInfo[]){{.name = "s", .type = TYPE_STRING, .is_mutable = false},
					{.name = "i", .type = TYPE_INT, .is_mutable = false}});

	registerBuiltin(
		scope, "chr", TYPE_STRING, 1,
		(argInfo[]){{.name = "i", .type = TYPE_INT, .is_mutable = false}});

	registerBuiltin(
		scope, "strLength", TYPE_INT, 1,
		(argInfo[]){
			{.name = "value", .type = TYPE_STRING, .is_mutable = false}});

	registerBuiltin(
		scope, "toStr", TYPE_STRING, 1,
		(argInfo[]){{.name = "value", .type = TYPE_ANY, .is_mutable = false}});

	registerBuiltin(
		scope, "toInt", TYPE_INT, 1,
		(argInfo[]){
			{.name = "value", .type = TYPE_DOUBLE, .is_mutable = false}});
}

/**
 * @brief Goes through list of astNodes and analyses all of them
 * @param nodeList List of astNodes we are analyzing
 * @param scope Pointer to a scope of nodeList
 */
static void analyzeNodeList(astNodeList nodeList, Scope *scope) {
	for (size_t i = 0; i < nodeList.count; i++) {
		analyzeNode(nodeList.items[i], scope);
	}
}

/**
 * @brief Initializes sematic analysis, global scope and checks validity of root
 * node
 * @param root Pointer to root node of ast tree
 */
void semanticAnalyze(astNode *root) {
	if (root == NULL || root->type != AST_PROGRAM) {
		semanticError(INTERNAL_ERROR, "Root node is not a program", root, NULL);
	}
	rootNode = root;

	Scope globalScope = {.symbols = initTree(),
						 .parent = NULL,
						 .id = scopeIdCounter++,
						 .depth = 0};
	registerBuiltins(&globalScope);
	analyzeNode(root, &globalScope);

	if (mainNode == NULL) {
		semanticError(SEMANTIC_UNDEFINITION_ERROR, "Missing @main function",
					  root, &globalScope);
	}

	destroyAVL(globalScope.symbols);
}

/**
 * @brief Checks astnode type and and analyzes node accordingly
 * @param node Pointer to a node we are analyzing
 * @param scope Scope node is in
 */
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
		analyzeIf(node, scope);
		break;
	case AST_EXPR_STMT:
		analyzeNode(node->data.exprStmt.expr, scope);

		node->dataType = node->data.exprStmt.expr->dataType;
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
		analyzeBinaryOp(node, scope);
		break;
	case AST_UNARY_OP:
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
		semanticError(INTERNAL_ERROR, "Unknown AST node type", node, scope);
		break;
	}
}

/**
 * @brief Loops through scopes until root node or until we find symbol in scope
 * @param scope Current scope we are looking fir symbol in
 * @param name Identificator in symbol we are looking for
 * @return Pointer to AVLnode with maching name from first possible scope OR
 * NULL pointer
 */
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

/**
 * @brief Chcecks for complatibility of assigned values, existence of elements
 * and mutabily of target
 * @param node Pointer to ast node with both target and value
 * @param scope Scope of assignment
 */
static void analyzeAssign(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_ASSIGN) {
		semanticError(INTERNAL_ERROR, "Invalid assignment node", node, scope);
		return;
	}

	if (node->data.assign.target->type != AST_IDENTIFIER)
		semanticError(SYNTAX_ERROR, "Invalid assignment target", node, scope);

	analyzeNode(node->data.assign.target, scope);
	analyzeNode(node->data.assign.value, scope);

	if (node->data.assign.target->dataType !=
			node->data.assign.value->dataType &&
		!(node->data.assign.target->dataType == TYPE_DOUBLE &&
		  node->data.assign.value->dataType == TYPE_INT)) {
		semanticError(SEMANTIC_TYPE_ERROR, "Assignment type mismatch", node,
					  scope);
	}

	AVLNode *symbol = searchSymbolInScope(
		scope, node->data.assign.target->data.identifier.name);
	if (symbol == NULL) {
		semanticError(SEMANTIC_UNDEFINITION_ERROR,
					  "Assignment to undefined variable", node, scope);
		return;
	}
	if (symbol->params.is_mutable == false) {
		semanticError(SEMANTIC_UNSUPPORTED_MODIFICATON_ERROR,
					  "Assignment to immutable variable", node, scope);
	}
}

/**
 * @brief Creates new scope for block of code, analyzes node list and sets data
 * type of whole block
 * @param node Pointer to ast node with nodelist of all nodes from block
 * @param parentScope Pointer to scope that will become parentScope to new
 * blockScope
 */
static void analyzeBlock(astNode *node, Scope *parentScope) {
	if (node == NULL || node->type != AST_BLOCK) {
		semanticError(INTERNAL_ERROR, "Invalid block node", node, parentScope);
		return;
	}

	Scope blockScope = {.symbols = initTree(),
						.parent = parentScope,
						.id = scopeIdCounter++,
						.depth = parentScope->depth + 1};

	astNodeList stmts = node->data.block.stmts;
	analyzeNodeList(stmts, &blockScope);
	node->dataType = TYPE_UNIT;
	if (stmts.count > 0) {
		astNode *last = stmts.items[stmts.count - 1];
		if (last->type == AST_EXPR_STMT || last->type == AST_IF ||
			last->type == AST_RETURN) {
			node->dataType = last->dataType;
		}
	}
	destroyAVL(blockScope.symbols);
}

/**
 * @brief Checks for validity of data, redeclaration and then inserts new
 * variable to AVL
 * @param node Pointer to node with new value, to be set
 * @param scope Pointer to scope that new variable will be inserted to
 */
static void analyzeVarDecl(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_VAR_DECL) {
		semanticError(INTERNAL_ERROR, "Invalid variable declaration", node,
					  scope);
		return;
	}

	astNode *valueNode = node->data.varDecl.value;
	astDataType declType = node->data.varDecl.declType;

	if (valueNode == NULL) {
		semanticError(SEMANTIC_TYPE_ERROR, "Variable declaration without value",
					  node, scope);
		return;
	}
	analyzeNode(valueNode, scope);
	if (valueNode->dataType == TYPE_NONE) {
		semanticError(SEMANTIC_TYPE_ERROR,
					  "Variable declaration with TYPE_NONE value", node, scope);
		return;
	}

	if (declType != TYPE_NONE && valueNode->dataType != declType &&
		!(declType == TYPE_DOUBLE && valueNode->dataType == TYPE_INT)) {

		semanticError(SEMANTIC_TYPE_ERROR, "Variable declaration type mismatch",
					  node, scope);
		return;
	}
	if (declType == TYPE_NONE) {
		declType = valueNode->dataType;
	}

	AVLNode *existingSymbol =
		searchNode(scope->symbols, node->data.varDecl.name);

	if (existingSymbol != NULL) {
		semanticError(SEMANTIC_REDEFINITION_ERROR, "Variable redeclaration",
					  node, scope);
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
	node->scopeId = scope->id;

	return;
}

/**
 * @brief Checks for existance and validity of identifier and for mutualy nested
 * functions with no return type
 * @param node Pointer to node with identifier
 * @param scope Scope of identifier node
 */
static void analyzeIdentifier(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_IDENTIFIER) {
		semanticError(INTERNAL_ERROR, "Invalid identifier node", node, scope);
		return;
	}

	AVLNode *symbol = searchSymbolInScope(scope, node->data.identifier.name);
	if (symbol == NULL) {
		semanticError(SEMANTIC_UNDEFINITION_ERROR,
					  "Identifier used before declaration", node, scope);
		return;
	}

	if (symbol->params.node_type == FUNCTION) {
		astNode *definition = symbol->params.definition;
		if (definition == NULL ||
			definition->data.funcDef.paramLists.count > 0) {
			semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
						  "Function must be called with its parameter lists",
						  node, scope);
			return;
		}

		// Detect recursive calls with an unresolved return type
		if (symbol->params.resolveState == TYPE_RESOLVING &&
			symbol->params.type == TYPE_NONE) {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "Cannot infer return type of recursive functions; "
						  "declare a return type",
						  node, scope);
			return;
		}
		if (symbol->params.resolveState == TYPE_UNVISITED) {
			analyzeFunctionLazy(definition, scope, symbol->params.receiver);
		}
	}

	node->dataType = symbol->params.type;
	node->scopeId = symbol->params.scope;
}

/**
 * @brief Analyzes declared function or extension
 * @param node Pointer to node with declared extension or function
 * @param scope Pointer to scope of declared function or extension
 * @param receiver NULL if node is a function node or expresion if node is
 * extension
 */
static void analyzeFunction(astNode *node, Scope *scope, astNode *receiver) {
	// makes an extensionkey if receiver is set
	char *extensionKey =
		receiver != NULL
			? makeExtensionPrefix(receiver->data.param.type,
								  node->data.funcDef.name, node, scope)
			: NULL;
	AVLNode *searchedFunction = searchSymbolInScope(
		scope, extensionKey != NULL ? extensionKey : node->data.funcDef.name);
	free(extensionKey);
	if (searchedFunction == NULL) {
		semanticError(INTERNAL_ERROR, "Function definition is not registered",
					  node, scope);
		return;
	}
	if (searchedFunction->params.resolveState == TYPE_RESOLVED) {
		return;
	}

	// Detect recursive dependencies with an unresolved return type.
	if (searchedFunction->params.resolveState == TYPE_RESOLVING &&
		searchedFunction->params.type == TYPE_NONE) {
		semanticError(SEMANTIC_TYPE_ERROR,
					  "Cannot infer return type of recursive functions; "
					  "declare a return type",
					  node, scope);
		return;
	}
	searchedFunction->params.resolveState = TYPE_RESOLVING;

	Scope functionScope = {.symbols = initTree(),
						   .parent = scope,
						   .id = scopeIdCounter++,
						   .depth = scope->depth + 1,
						   .closingFunction = node};

	if (receiver != NULL) {
		node->scopeId = functionScope.id;
		if (receiver->type != AST_PARAM ||
			receiver->data.param.type == TYPE_NONE) {
			semanticError(SEMANTIC_TYPE_ERROR, "Invalid extension receiver",
						  receiver != NULL ? receiver : node, &functionScope);
		}
		// inserts extension receiver into scope AVL
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
	for (size_t i = 0; i < node->data.funcDef.paramLists.count; i++) {

		astNode *list = node->data.funcDef.paramLists.items[i];

		for (size_t j = 0; j < list->data.paramList.params.count; j++) {

			// inserts each param into scope AVL
			astNode *param = list->data.paramList.params.items[j];
			param->scopeId = functionScope.id;
			paramInfo info = {.name = param->data.param.name,
							  .type = param->data.param.type,
							  .node_type = VARIABLE,
							  .is_mutable = false,
							  .param_count = 0,
							  .param_list = NULL,
							  .scope = functionScope.id,
							  .depth = functionScope.depth};

			if (searchNode(functionScope.symbols, info.name) != NULL) {

				semanticError(SEMANTIC_REDEFINITION_ERROR,
							  "Duplicate function parameter", param,
							  &functionScope);
			}

			functionScope.symbols = insertNode(functionScope.symbols, info);
		}
	}

	analyzeNode(node->data.funcDef.body, &functionScope);

	astDataType declaredType = node->data.funcDef.retType;
	astDataType actualType = node->data.funcDef.body->dataType;

	// checks for main duplicity and validity
	if (node->data.funcDef.isMain) {

		if (declaredType != TYPE_UNIT) {

			semanticError(SEMANTIC_TYPE_ERROR, "Main is not type UNIT", node,
						  &functionScope);
		}
		if (mainNode != NULL) {
			semanticError(SEMANTIC_REDEFINITION_ERROR,
						  "Multiple main declarations", node, &functionScope);
		}
		mainNode = node;

		if (node->data.funcDef.paramLists.count != 1 ||
			node->data.funcDef.paramLists.items[0]
					->data.paramList.params.count != 0) {
			semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
						  "Main must not have parameters", node,
						  &functionScope);
		}
	}

	if (actualType == TYPE_NONE) {
		semanticError(SEMANTIC_TYPE_ERROR,
					  "Cannot determine function return type", node,
					  &functionScope);
		return;
	}

	bool finalStatementReturns = lastStatementReturns(node->data.funcDef.body);
	// checks if function declared and body datatypes are compatible
	if (declaredType != TYPE_NONE && declaredType != TYPE_UNIT &&
		!finalStatementReturns && declaredType != actualType &&
		!(declaredType == TYPE_DOUBLE && actualType == TYPE_INT)) {
		semanticError(SEMANTIC_TYPE_ERROR, "Function body type mismatch", node,
					  &functionScope);
		return;
	}

	astDataType finalType = TYPE_UNIT;
	if (declaredType != TYPE_UNIT) {
		finalType = (declaredType == TYPE_NONE) ? actualType : declaredType;
	}

	node->dataType = finalType;
	node->data.funcDef.retType = finalType;

	searchedFunction->params.type = finalType;

	// Free all of the used parameters
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
	searchedFunction->params.resolveState = TYPE_RESOLVED;
}

/**
 * @brief Help function for checking compability of argument and parameter
 * @param argType Datatype of argument
 * @param paramType Datatype of parameter
 * @return TRUE if compatible FALSE if they aren't
 */
static bool isArgCompatible(astDataType argType, astDataType paramType) {
	return argType == paramType || paramType == TYPE_ANY ||
		   (argType == TYPE_INT && paramType == TYPE_DOUBLE);
}

/**
 * @brief Checks for int to double conversion case and marks it
 * @param arg pointer to node with argument data
 * @param paramType Expected datatype of argument
 */
static void markIntToDouble(astNode *arg, astDataType paramType) {
	if (arg->dataType == TYPE_INT && paramType == TYPE_DOUBLE)
		arg->intToDouble = true;
}

/**
 * @brief Checks for validity of call
 * @param node call node with callee in data
 * @param scope scope of call
 */
static void analyzeCall(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_CALL) {
		semanticError(INTERNAL_ERROR, "Invalid function call node", node,
					  scope);
		return;
	}

	size_t totalArgs = 0;
	size_t totalLists = 0;

	astNode *callee = node;

	// checks for nested call nodes
	while (callee != NULL && callee->type == AST_CALL) {
		analyzeNodeList(callee->data.call.args, scope);
		totalArgs += callee->data.call.args.count;
		totalLists++;
		callee = callee->data.call.callee;
	}

	if (callee == NULL) {
		semanticError(INTERNAL_ERROR, "Missing call target", node, scope);
		return;
	}

	if (callee->type != AST_IDENTIFIER && callee->type != AST_MEMBER) {
		semanticError(INTERNAL_ERROR, "Unsupported callee type", callee, scope);
		return;
	}

	AVLNode *callFunc = NULL;
	if (callee->type == AST_IDENTIFIER) {
		callFunc = searchSymbolInScope(scope, callee->data.identifier.name);
	} else if (callee->type == AST_MEMBER) {

		callFunc = lookupExtension(callee, scope);
	}

	if (callFunc == NULL) {
		semanticError(SEMANTIC_UNDEFINITION_ERROR,
					  "Function used before declaration", callee, scope);
		return;
	}

	if (callFunc->params.node_type != FUNCTION) {
		semanticError(SEMANTIC_TYPE_ERROR, "Identifier is not a function",
					  callee, scope);
		return;
	}

	if (callFunc->params.type == TYPE_NONE &&
		callFunc->params.resolveState == TYPE_RESOLVING) {
		semanticError(SEMANTIC_TYPE_ERROR,
					  "Cannot infer return type of recursive functions; "
					  "declare a return type",
					  node, scope);
		return;
	}

	// checks for called unvisted functions and analysis them
	if (callFunc->params.definition != NULL &&
		callFunc->params.resolveState == TYPE_UNVISITED) {
		analyzeFunctionLazy(callFunc->params.definition, scope,
							callFunc->params.receiver);
	}

	astNode *definition = callFunc->params.definition;

	if (callee->type == AST_MEMBER && definition == NULL) {
		semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
					  "Member cannot be called with parentheses", node, scope);
		return;
	}

	astNode **calls = malloc(totalLists * sizeof(astNode *));
	if (calls == NULL) {
		semanticError(INTERNAL_ERROR, "Memory allocation failed", node, scope);
		return;
	}
	astNode *call = node;
	for (size_t i = totalLists; i > 0; i--) {
		calls[i - 1] = call;
		call = call->data.call.callee;
	}

	// checks argument count and datatypes
	if (definition != NULL) {
		astNodeList paramLists = definition->data.funcDef.paramLists;

		if (totalLists != paramLists.count) {
			free(calls);
			semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
						  "Function call parameter list count mismatch", node,
						  scope);
			return;
		}

		for (size_t i = 0; i < totalLists; i++) {
			astNodeList args = calls[i]->data.call.args;
			astNodeList params = paramLists.items[i]->data.paramList.params;

			if (args.count != params.count) {
				free(calls);
				semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
							  "Function call argument count mismatch", node,
							  scope);
				return;
			}

			for (size_t j = 0; j < args.count; j++) {
				if (!isArgCompatible(args.items[j]->dataType,
									 params.items[j]->data.param.type)) {
					free(calls);
					semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
								  "Function call argument type mismatch",
								  args.items[j], scope);
					return;
				}
				markIntToDouble(args.items[j],
								params.items[j]->data.param.type);
			}
		}
	} else {
		if (totalLists != 1 || totalArgs != callFunc->params.param_count) {
			free(calls);
			semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
						  "Function call argument count mismatch", node, scope);
			return;
		}

		for (size_t j = 0; j < totalArgs; j++) {
			astNode *arg = node->data.call.args.items[j];
			if (!isArgCompatible(arg->dataType,
								 callFunc->params.param_list[j].type)) {
				free(calls);
				semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
							  "Function call argument type mismatch", arg,
							  scope);
				return;
			}
			markIntToDouble(arg, callFunc->params.param_list[j].type);
		}
	}

	free(calls);
	node->dataType = callFunc->params.type;
	return;
}

/**
 * @brief Checks whether binary operation has correct datatypes
 * @param node Pointer to node with operation and children
 * @param scope Scope of binary operation
 */
static void analyzeBinaryOp(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_BINARY_OP) {
		semanticError(INTERNAL_ERROR, "Invalid binary operation", node, scope);
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
			semanticError(SEMANTIC_TYPE_ERROR, "Invalid types for addition",
						  node, scope);
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
			semanticError(SEMANTIC_TYPE_ERROR,
						  "Invalid types for arithmetic operation", node,
						  scope);
		}
		break;

	case op_equal:
	case op_notEqual:
		if (((leftType == TYPE_INT || leftType == TYPE_DOUBLE) &&
			 (rightType == TYPE_INT || rightType == TYPE_DOUBLE)) ||
			leftType == rightType) {
			node->dataType = TYPE_BOOL;
		} else {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "Invalid types for comparison operation", node,
						  scope);
		}
		break;
	case op_biggerThen:
	case op_lowerThen:
	case op_lowerEqual:
	case op_biggerEqual:
		if (((leftType == TYPE_INT || leftType == TYPE_DOUBLE) &&
			 (rightType == TYPE_INT || rightType == TYPE_DOUBLE)) ||
			(leftType == TYPE_STRING && rightType == TYPE_STRING)) {
			node->dataType = TYPE_BOOL;
		} else {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "Invalid types for comparison operation", node,
						  scope);
		}
		break;
	case op_and:
	case op_or:
		if (leftType == TYPE_BOOL && rightType == TYPE_BOOL) {
			node->dataType = TYPE_BOOL;
		} else {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "Invalid types for logical operation", node, scope);
		}
		break;
	default:
		break;
	}
}

/**
 * @brief Checks whether unary operation has correct datatypes
 * @param node Pointer to node with operation and child
 * @param scope Scope of unary operation
 */
static void analyzeUnaryOp(astNode *node, Scope *scope) {

	if (node == NULL || node->type != AST_UNARY_OP) {
		semanticError(INTERNAL_ERROR, "Invalid unary operation", node, scope);
		return;
	}

	analyzeNode(node->data.unary.operand, scope);

	astDataType operandType = node->data.unary.operand->dataType;

	switch (node->data.unary.op) {
	case op_not:
		if (operandType == TYPE_BOOL) {
			node->dataType = TYPE_BOOL;
		} else {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "Invalid type for logical NOT operation", node,
						  scope);
		}
		break;
	case op_sub:
		if (operandType == TYPE_INT || operandType == TYPE_DOUBLE) {
			node->dataType = operandType;
		} else {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "Invalid type for unary negation operation", node,
						  scope);
		}
		break;
	case op_add:
		if (operandType != TYPE_INT && operandType != TYPE_DOUBLE) {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "Invalid type for unary plus operation", node, scope);
		} else {
			node->dataType = operandType;
		}
		break;
	default:
		break;
	}
}

/**
 * @brief Registers function to globalScope so other functions can access it
 * @param node Pointer to node with function to be declared
 * @param scope Pointer to globalScope
 * @param receiver Pointer to node with receiver if function is extension else
 * NULL pointer
 */
static void registerFunction(astNode *node, Scope *scope, astNode *receiver) {
	if (node == NULL || node->type != AST_FUN_DEF) {
		semanticError(INTERNAL_ERROR, "Invalid function definition", node,
					  scope);
	}
	char *symbolName = node->data.funcDef.name;
	char *extensionKey = NULL;

	if (receiver != NULL) {
		extensionKey = makeExtensionPrefix(
			receiver->data.param.type, node->data.funcDef.name, node, scope);

		symbolName = extensionKey;
	}
	if (searchNode(scope->symbols, symbolName) != NULL) {
		free(extensionKey);
		semanticError(SEMANTIC_REDEFINITION_ERROR, "Function already defined",
					  node, scope);
	}

	size_t paramCount = 0;

	for (size_t i = 0; i < node->data.funcDef.paramLists.count; i++) {
		astNode *list = node->data.funcDef.paramLists.items[i];

		if (list == NULL || list->type != AST_PARAM_LIST) {
			semanticError(INTERNAL_ERROR, "Invalid parameter list",
						  list != NULL ? list : node, scope);
		}
		// counts param from multiple param lists
		paramCount += list->data.paramList.params.count;
	}

	argInfo *args = NULL;

	if (paramCount > 0) {
		args = calloc(paramCount, sizeof(argInfo));

		if (args == NULL) {
			free(extensionKey);
			semanticError(INTERNAL_ERROR, "Memory allocation failed", node,
						  scope);
		}
	}

	size_t index = 0;

	// initialyzes all function arguments
	for (size_t i = 0; i < node->data.funcDef.paramLists.count; i++) {
		astNode *list = node->data.funcDef.paramLists.items[i];

		for (size_t j = 0; j < list->data.paramList.params.count; j++) {
			astNode *paramNode = list->data.paramList.params.items[j];

			if (paramNode == NULL || paramNode->type != AST_PARAM) {
				free(args);
				free(extensionKey);

				semanticError(INTERNAL_ERROR, "Invalid parameter",
							  paramNode != NULL ? paramNode : node, scope);
			}
			if (paramNode->data.param.name == NULL) {
				free(args);
				free(extensionKey);

				semanticError(INTERNAL_ERROR, "Parameter name is NULL",
							  paramNode, scope);
			}
			if (paramNode->data.param.type == TYPE_NONE) {
				free(args);
				free(extensionKey);

				semanticError(INTERNAL_ERROR, "Parameter type is TYPE_NONE",
							  paramNode, scope);
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
	AVLNode *symbol = searchNode(scope->symbols, symbolName);
	symbol->params.definition = node;
	symbol->params.receiver = receiver;

	free(args);
	free(extensionKey);
}

/**
 * @brief Checks for validity of thenBody and elseBody type AND their return
 * types
 * @param node pointer to node with if-else statement
 * @param scope pointer to scope of ifNode
 */
static void analyzeIf(astNode *node, Scope *scope) {
	analyzeNode(node->data.ifStmt.cond, scope);
	if (node->data.ifStmt.cond->dataType != TYPE_BOOL) {
		semanticError(SEMANTIC_TYPE_ERROR, "If condition must be of type bool",
					  node->data.ifStmt.cond, scope);
	}
	analyzeNode(node->data.ifStmt.thenBody, scope);
	astDataType thenType = node->data.ifStmt.thenBody->dataType;

	if (node->data.ifStmt.elseBody == NULL) {
		node->dataType = TYPE_UNIT;
		return;
	}
	analyzeNode(node->data.ifStmt.elseBody, scope);
	astDataType elseType = node->data.ifStmt.elseBody->dataType;

	// checks if return occurs in body
	bool thenReturns = alwaysReturns(node->data.ifStmt.thenBody);
	bool elseReturns = alwaysReturns(node->data.ifStmt.elseBody);

	// assignment of ifNode return type
	if (thenReturns && elseReturns) {
		node->dataType = TYPE_UNIT;
	} else if (thenReturns) {
		node->dataType = elseType;
	} else if (elseReturns) {
		node->dataType = thenType;
	} else if (thenType == elseType) {
		node->dataType = thenType;
	} else if ((thenType == TYPE_INT && elseType == TYPE_DOUBLE) ||
			   (thenType == TYPE_DOUBLE && elseType == TYPE_INT)) {
		node->dataType = TYPE_DOUBLE;
	} else {
		semanticError(SEMANTIC_TYPE_ERROR,
					  "If branches have incompatible types", node, scope);
	}

	if (thenType == TYPE_NONE || elseType == TYPE_NONE) {
		semanticError(SEMANTIC_TYPE_ERROR,
					  "Cannot determine if expression type", node, scope);
	}
	return;
}

/**
 * @brief Checks for validity off forNode and new scope for it
 * @param node Pointer to a forNode
 * @param scope Pointer to a scope of forNode
 */
static void analyzeFor(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_FOR) {
		semanticError(INTERNAL_ERROR, "Invalid for loop node", node, scope);
		return;
	}
	astNode *fromNode = node->data.forLoop.from;
	astNode *toNode = node->data.forLoop.to;
	astNode *stepNode = node->data.forLoop.step;
	astNode *bodyNode = node->data.forLoop.body;

	if (fromNode == NULL || toNode == NULL || bodyNode == NULL ||
		node->data.forLoop.iterName == NULL) {
		semanticError(INTERNAL_ERROR, "Invalid for loop structure", node,
					  scope);
		return;
	}

	analyzeNode(fromNode, scope);
	analyzeNode(toNode, scope);

	if (fromNode->dataType != TYPE_INT || toNode->dataType != TYPE_INT) {
		semanticError(
			SEMANTIC_TYPE_ERROR,
			"For loop 'from' and 'to' expressions must be of type int",
			fromNode->dataType != TYPE_INT ? fromNode : toNode, scope);
	}
	if (stepNode != NULL) {
		analyzeNode(stepNode, scope);
		if (stepNode->dataType != TYPE_INT) {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "For loop 'step' expression must be of type int",
						  stepNode, scope);
		}
		if (stepNode->data.intValue == 0) {
			semanticError(SEMANTIC_ERROR,
						  "For loop 'step' expression cannot be zero", stepNode,
						  scope);
		}
	}

	Scope forScope = {.symbols = initTree(),
					  .parent = scope,
					  .id = scopeIdCounter++,
					  .depth = scope->depth + 1};

	node->scopeId = forScope.id;

	paramInfo iterInfo = {.name = node->data.forLoop.iterName,
						  .type = TYPE_INT,
						  .node_type = VARIABLE,
						  .is_mutable = false,
						  .param_count = 0,
						  .param_list = NULL,
						  .scope = forScope.id,
						  .depth = forScope.depth};

	forScope.symbols = insertNode(forScope.symbols, iterInfo);

	// checks validity for multiple guards
	for (size_t guardIndex = 0; guardIndex < node->data.forLoop.guards.count;
		 guardIndex++) {
		astNode *guardNode = node->data.forLoop.guards.items[guardIndex];
		if (guardNode == NULL) {
			semanticError(INTERNAL_ERROR, "Invalid for loop guard", node,
						  &forScope);
		}
		analyzeNode(guardNode, &forScope);
		if (guardNode->dataType != TYPE_BOOL) {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "For loop guard expressions must be of type bool",
						  guardNode, &forScope);
		}
	}
	analyzeNode(bodyNode, &forScope);
	destroyAVL(forScope.symbols);
}

/**
 * @brief Checks whileNode validity and assings new scope
 * @param node Pointer pointer to a node with condition and body
 * @param scope Pointer to a scope of whileNode
 */
static void analyzeWhile(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_WHILE) {
		semanticError(INTERNAL_ERROR, "Invalid while loop node", node, scope);
		return;
	}

	if (node->data.whileLoop.cond == NULL ||
		node->data.whileLoop.body == NULL) {
		semanticError(INTERNAL_ERROR, "Invalid while loop structure", node,
					  scope);
		return;
	}

	analyzeNode(node->data.whileLoop.cond, scope);
	if (node->data.whileLoop.cond->dataType != TYPE_BOOL) {
		semanticError(SEMANTIC_TYPE_ERROR,
					  "While condition must be of type bool",
					  node->data.whileLoop.cond, scope);
	}
	Scope whileScope = {.symbols = initTree(),
						.parent = scope,
						.id = scopeIdCounter++,
						.depth = scope->depth + 1};

	analyzeNode(node->data.whileLoop.body, &whileScope);
	destroyAVL(whileScope.symbols);
}

/**
 * @brief Checks for validity of receiver and calles registerFunction
 * @param node Pointer to node of extension declaration
 * @param scope Pointer to a globalScope
 */
static void registerExtension(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_EXTENSION) {
		semanticError(INTERNAL_ERROR, "Invalid extension node", node, scope);
		return;
	}

	astNode *receiver = node->data.extension.receiver;

	if (receiver == NULL || receiver->type != AST_PARAM ||
		receiver->data.param.type == TYPE_NONE) {
		semanticError(SEMANTIC_TYPE_ERROR, "Invalid extension receiver",
					  receiver != NULL ? receiver : node, scope);
	}

	for (size_t i = 0; i < node->data.extension.funDefs.count; i++) {
		astNode *method = node->data.extension.funDefs.items[i];

		registerFunction(method, scope, receiver);
	}
}

/**
 * @brief Checks for validity of extension and then further analyzing it using
 * analyzeFuncion
 * @param node Pointer to node with extension declaration
 * @param scope Pointer to a globalScope
 */
static void analyzeExtension(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_EXTENSION) {
		semanticError(INTERNAL_ERROR, "Invalid extension node", node, scope);
		return;
	}

	if (node->data.extension.receiver == NULL) {
		semanticError(INTERNAL_ERROR, "Invalid extension structure", node,
					  scope);
		return;
	}

	astNode *receiver = node->data.extension.receiver;

	for (size_t i = 0; i < node->data.extension.funDefs.count; i++) {
		astNode *method = node->data.extension.funDefs.items[i];

		analyzeFunction(method, scope, receiver);
	}
}

/**
 * @brief Creates an idenfier for extension
 * @param type Datatype of receiver
 * @param name String with extension name
 * @param node Pointer to extensionNode used only for error message
 * @param scope Pointer to scope of extension used only for error message
 * @return Dynamically allocated extension key that must be freed by the caller.
 */
static char *makeExtensionPrefix(astDataType type, const char *name,
								 const astNode *node, Scope *scope) {
	// checks for the length of new identifier
	int len = snprintf(NULL, 0, "#EXT:%d:%s", (int)type, name);

	if (len < 0) {
		semanticError(INTERNAL_ERROR, "Cannot create extension prefix string",
					  node, scope);
	}

	char *key = malloc((size_t)len + 1);

	if (key == NULL) {
		semanticError(INTERNAL_ERROR, "Memory allocation failed", node, scope);
	}

	snprintf(key, (size_t)len + 1, "#EXT:%d:%s", (int)type, name);

	return key;
}

/**
 * @brief Analyzes validity of an extension accessed without paranthesees
 * @param node Pointer to memberNode
 * @param scope Pointer to scope of member
 */
static void analyzeMember(astNode *node, Scope *scope) {
	AVLNode *method = lookupExtension(node, scope);

	if (method == NULL) {
		return;
	}

	astNode *definition = method->params.definition;

	if (definition != NULL && definition->data.funcDef.paramLists.count > 0) {

		semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
					  "Extension method requires parentheses", node, scope);
		return;
	}

	if (definition != NULL && method->params.resolveState == TYPE_RESOLVING &&
		method->params.type == TYPE_NONE)
		semanticError(
			SEMANTIC_TYPE_ERROR,
			"Cannot infer return type of recursive functions; declare a "
			"return type",
			node, scope);
	if (definition != NULL && method->params.resolveState == TYPE_UNVISITED)
		analyzeFunctionLazy(definition, scope, method->params.receiver);

	node->dataType = method->params.type;
	return;
}

/**
 * @brief Checks existance and validity of extension identifier
 * @param node Pointer to memberNode
 * @param scope Pointer to scope of member
 * @return Pointer to node of AVLtree where extension is declered
 */
static AVLNode *lookupExtension(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_MEMBER) {
		semanticError(INTERNAL_ERROR, "Invalid member node", node, scope);
		return NULL;
	}

	astNode *object = node->data.member.object;

	if (object == NULL || node->data.member.name == NULL) {
		semanticError(INTERNAL_ERROR, "Invalid member structure", node, scope);
		return NULL;
	}

	analyzeNode(object, scope);

	if (object->dataType == TYPE_NONE) {
		semanticError(SEMANTIC_TYPE_ERROR, "Unknown member object type", object,
					  scope);
		return NULL;
	}

	char *key = makeExtensionPrefix(object->dataType, node->data.member.name,
									node, scope);

	AVLNode *symbol = searchSymbolInScope(scope, key);

	free(key);

	if (symbol == NULL) {
		semanticError(SEMANTIC_UNDEFINITION_ERROR, "Undefined extension method",
					  node, scope);
		return NULL;
	}

	if (symbol->params.node_type != FUNCTION) {
		semanticError(SEMANTIC_TYPE_ERROR, "Member is not a function", node,
					  scope);
		return NULL;
	}

	return symbol;
}

/**
 * @brief Determines whether an AST node guarantees an explicit return.
 * @param node Pointer to the AST node being checked.
 * @return True if the node guarantees a return, otherwise false.
 */
static void analyzeReturn(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_RETURN) {
		semanticError(INTERNAL_ERROR, "Invalid return node", node, scope);
		return;
	}

	astNode *function = NULL;
	Scope *current = scope;
	// Finds a function above return
	while (current != NULL) {

		if (current->closingFunction != NULL) {
			function = current->closingFunction;
			break;
		}
		current = current->parent;
	}

	if (function == NULL) {
		semanticError(10, "Return outside function", node, scope);
		return;
	}

	astDataType expectedType = function->data.funcDef.retType;

	if (expectedType == TYPE_NONE) {
		semanticError(SEMANTIC_TYPE_ERROR,
					  "Function with return must declare return type", node,
					  scope);
		return;
	}

	astNode *value = node->data.ret.value;

	if (value == NULL) {
		if (expectedType != TYPE_UNIT) {
			semanticError(SEMANTIC_TYPE_ERROR, "Missing return value", node,
						  scope);
			return;
		}
		node->dataType = TYPE_UNIT;
		return;
	}

	analyzeNode(value, scope);

	astDataType actualType = value->dataType;

	if (actualType == TYPE_NONE) {
		semanticError(SEMANTIC_TYPE_ERROR, "Invalid return expression", value,
					  scope);
		return;
	}

	if (expectedType == TYPE_UNIT) {
		node->dataType = TYPE_UNIT;
		return;
	}

	if (actualType != expectedType &&
		!(expectedType == TYPE_DOUBLE && actualType == TYPE_INT)) {

		semanticError(SEMANTIC_TYPE_ERROR, "Return type mismatch", value,
					  scope);
		return;
	}

	node->dataType = expectedType;
}

/**
 * @brief Analyzes validity of interpNode an each part of interpolated string
 * @param node Pointer to node with interpolated string
 * @param scope Pointer to scope of analyzed block
 */
static void analyzeInterp(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_INTERP) {
		semanticError(INTERNAL_ERROR, "Invalid interpolation node", node,
					  scope);
		return;
	}

	for (size_t i = 0; i < node->data.interp.parts.count; i++) {
		astNode *part = node->data.interp.parts.items[i];

		if (part == NULL) {
			semanticError(INTERNAL_ERROR, "Invalid interpolation part", node,
						  scope);
			return;
		}

		analyzeNode(part, scope);

		if (part->dataType == TYPE_NONE) {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "Invalid interpolation expression", part, scope);
			return;
		}
	}

	node->dataType = TYPE_STRING;
}

/**
 * @brief Determinates whether blockNode has a return in its body
 * @param node Pointer to blockNode that will be checked for returns
 * @returns TRUE if return is located in body else FALSE
 */
static bool alwaysReturns(const astNode *node) {
	if (node == NULL)
		return false;

	switch (node->type) {
	case AST_RETURN:
		return true;

	case AST_BLOCK:
		for (size_t i = 0; i < node->data.block.stmts.count; i++) {
			if (alwaysReturns(node->data.block.stmts.items[i]))
				return true;
		}
		return false;

	case AST_IF:
		return node->data.ifStmt.elseBody != NULL &&
			   alwaysReturns(node->data.ifStmt.thenBody) &&
			   alwaysReturns(node->data.ifStmt.elseBody);

	case AST_EXPR_STMT:
		return alwaysReturns(node->data.exprStmt.expr);

	default:
		return false;
	}
}

/**
 * @brief Checks whether the last statement of a function body guarantees an
 * explicit return.
 * @param body Pointer to the function body AST node.
 * @return True if the last statement guarantees a return, otherwise false.
 */
static bool lastStatementReturns(const astNode *body) {
	if (body == NULL) {
		return false;
	}

	const astNode *last = body;

	if (body->type == AST_BLOCK) {
		astNodeList stmts = body->data.block.stmts;

		if (stmts.count == 0) {
			return false;
		}

		last = stmts.items[stmts.count - 1];
	}

	return alwaysReturns(last);
}

/**
 * @brief Destroys symbol tables along the scope chain, excluding the global
 * scope.
 * @param scope Pointer to the starting scope.
 * @return Pointer to the global scope, or NULL if scope is NULL.
 */
static Scope *destroyScopeChain(Scope *scope) {
	while (scope != NULL && scope->parent != NULL) {
		destroyAVL(scope->symbols);
		scope->symbols = NULL;
		scope = scope->parent;
	}
	return scope;
}

/**
 * @brief Destroys symbol tables of all active and suspended scopes.
 * @param scope Pointer to the current scope.
 */
static void destroyFullAVL(Scope *scope) {
	Scope *globalScope = destroyScopeChain(scope);

	// deletes other braches of AVL three
	for (SuspendedScope *s = suspendedScopes; s != NULL; s = s->prev) {
		Scope *root = destroyScopeChain(s->scope);
		if (globalScope == NULL)
			globalScope = root;
	}
	suspendedScopes = NULL;

	if (globalScope != NULL) {
		destroyAVL(globalScope->symbols);
		globalScope->symbols = NULL;
	}
}

/**
 * @brief Analyzes a previously unvisited function in the global scope while
 * preserving the calling scope for error cleanup.
 * @param definition Pointer to the called function definition.
 * @param scope Pointer to the calling scope.
 * @param receiver Extension receiver or NULL for a regular function.
 */
static void analyzeFunctionLazy(astNode *definition, Scope *scope,
								astNode *receiver) {
	Scope *definitionScope = scope;
	while (definitionScope->parent != NULL) {
		definitionScope = definitionScope->parent;
	}

	SuspendedScope suspended = {.scope = scope, .prev = suspendedScopes};
	suspendedScopes = &suspended;

	analyzeFunction(definition, definitionScope, receiver);

	suspendedScopes = suspended.prev;
}