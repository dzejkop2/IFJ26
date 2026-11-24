/**
 * @name ifj26
 * @authors Tomáš Zajac (xzajact00)
 * @file semantic.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "semantic.h"

static int scopeIdCounter = 0;
static astNode *mainNode = NULL;

static void semanticError(int code, const char *message, const astNode *node) {
	char diagnostic[512];
	if (node != NULL) {
		snprintf(diagnostic, sizeof(diagnostic), "%s (line %d, column %d)",
				 message, node->line, node->col);
	} else {
		snprintf(diagnostic, sizeof(diagnostic),
				 "%s (source location unavailable)", message);
	}
	error(code, diagnostic);
}

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
static void analyzeIf(astNode *node, Scope *scope);
static void analyzeFor(astNode *node, Scope *scope);
static void analyzeWhile(astNode *node, Scope *scope);
static void registerExtension(astNode *node, Scope *scope);
static void analyzeExtension(astNode *node, Scope *scope);
static char *makeExtensionPrefix(astDataType type, const char *name,
								 const astNode *node);
static void analyzeMember(astNode *node, Scope *scope);
static AVLNode *lookupExtension(astNode *node, Scope *scope);
static void analyzeReturn(astNode *node, Scope *scope);
static void analyzeInterp(astNode *node, Scope *scope);
static void registerBuiltins(Scope *scope);
static void registerBuiltin(Scope *scope, char *name, astDataType returnType,
							size_t paramCount, argInfo *params);
static bool alwaysReturns(const astNode *node);
static void registerBuiltinExtension(
    Scope *scope,
    astDataType receiverType,
    const char *name,
    astDataType returnType
); 
static bool lastStatementReturns(const astNode* body);

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

static void registerBuiltinExtension(
    Scope *scope,
    astDataType receiverType,
    const char *name,
    astDataType returnType
) {
    char *key = makeExtensionPrefix(receiverType, name, NULL);

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

    paramInfo info = {
        .name = key,
        .type = returnType,
        .node_type = FUNCTION,
        .is_mutable = false,
        .param_count = 0,
        .param_list = NULL,
        .scope = scope->id,
        .depth = scope->depth,
        .resolveState = TYPE_RESOLVED,
        .definition = NULL,
        .receiver = NULL
    };

    scope->symbols = insertNode(scope->symbols, info);

    free(key);
}


static void registerBuiltins(Scope *scope) {
	registerBuiltin(
		scope, "print", TYPE_UNIT, 1,
		(argInfo[]){{.name = "value", .type = TYPE_ANY, .is_mutable = false}});

	registerBuiltin(scope, "readLine", TYPE_STRING, 0, NULL);

	registerBuiltin(scope, "readInt", TYPE_INT, 0, NULL);

	registerBuiltin(scope, "readDouble", TYPE_DOUBLE, 0, NULL);

	registerBuiltin(scope, "readBoolean", TYPE_BOOL, 0, NULL);


registerBuiltinExtension(
    scope, TYPE_DOUBLE, "toInt", TYPE_INT
);

registerBuiltinExtension(
    scope, TYPE_DOUBLE, "floor", TYPE_DOUBLE
);

registerBuiltinExtension(
    scope, TYPE_STRING, "length", TYPE_INT
);

registerBuiltinExtension(
    scope, TYPE_INT, "toString", TYPE_STRING
);

registerBuiltinExtension(
    scope, TYPE_DOUBLE, "toString", TYPE_STRING
);

registerBuiltinExtension(
    scope, TYPE_BOOL, "toString", TYPE_STRING
);

registerBuiltinExtension(
    scope, TYPE_STRING, "toString", TYPE_STRING
);

registerBuiltinExtension(
    scope, TYPE_UNIT, "toString", TYPE_STRING
);

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
        {.name = "value", .type = TYPE_STRING, .is_mutable = false}
    }
);

registerBuiltin(
    scope, "toStr", TYPE_STRING, 1,
    (argInfo[]){
        {.name = "value", .type = TYPE_ANY, .is_mutable = false}
    }
);

registerBuiltin(
    scope, "toInt", TYPE_INT, 1,
    (argInfo[]){
        {.name = "value", .type = TYPE_DOUBLE, .is_mutable = false}
    }
);
}

static void analyzeNodeList(astNodeList nodeList, Scope *scope) {
	for (size_t i = 0; i < nodeList.count; i++) {
		analyzeNode(nodeList.items[i], scope);
	}
}

void semanticAnalyze(astNode *root) {
	if (root == NULL || root->type != AST_PROGRAM) {
		semanticError(INTERNAL_ERROR, "Root node is not a program", root);
	}

	Scope globalScope = {.symbols = initTree(),
						 .parent = NULL,
						 .id = scopeIdCounter++,
						 .depth = 0};
	registerBuiltins(&globalScope);
	analyzeNode(root, &globalScope);

	// a function named "main" without @main is not the entry point
	if (mainNode == NULL) {
		semanticError(SEMANTIC_UNDEFINITION_ERROR,
					  "Missing @main function", root);
	}

	destroyAVL(globalScope.symbols);
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
		semanticError(INTERNAL_ERROR, "Unknown AST node type", node);
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
		semanticError(INTERNAL_ERROR, "Invalid assignment node", node);
		return;
	}

	analyzeNode(node->data.assign.target, scope);
	analyzeNode(node->data.assign.value, scope);

	if (node->data.assign.target->dataType !=
			node->data.assign.value->dataType &&
		!(node->data.assign.target->dataType == TYPE_DOUBLE &&
		  node->data.assign.value->dataType == TYPE_INT)) {
		semanticError(SEMANTIC_TYPE_ERROR, "Assignment type mismatch", node);
	}

	AVLNode *symbol = searchSymbolInScope(
		scope, node->data.assign.target->data.identifier.name);
	if (symbol == NULL) {
		semanticError(SEMANTIC_UNDEFINITION_ERROR,
					  "Assignment to undefined variable", node);
		return;
	}
	if (symbol->params.is_mutable == false) {
		semanticError(SEMANTIC_UNSUPPORTED_MODIFICATON_ERROR,
					  "Assignment to immutable variable", node);
	}
}

static void analyzeBlock(astNode *node, Scope *parentScope) {
	if (node == NULL || node->type != AST_BLOCK) {
		semanticError(INTERNAL_ERROR, "Invalid block node", node);
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

static void analyzeVarDecl(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_VAR_DECL) {
		semanticError(INTERNAL_ERROR, "Invalid variable declaration", node);
		return;
	}

	astNode *valueNode = node->data.varDecl.value;
	astDataType declType = node->data.varDecl.declType;

	if (valueNode == NULL) {
		semanticError(SEMANTIC_TYPE_ERROR, "Variable declaration without value",
					  node);
		return;
	}
	analyzeNode(valueNode, scope);
	if (valueNode->dataType == TYPE_NONE) {
		semanticError(SEMANTIC_TYPE_ERROR,
					  "Variable declaration with TYPE_NONE value", node);
		return;
	}

	if (declType != TYPE_NONE && valueNode->dataType != declType &&
		!(declType == TYPE_DOUBLE && valueNode->dataType == TYPE_INT)) {

		semanticError(SEMANTIC_TYPE_ERROR, "Variable declaration type mismatch",
					  node);
		return;
	}
	if (declType == TYPE_NONE) {
		declType = valueNode->dataType;
	}

	AVLNode *existingSymbol =
		searchNode(scope->symbols, node->data.varDecl.name);

	if (existingSymbol != NULL) {
		semanticError(SEMANTIC_REDEFINITION_ERROR, "Variable redeclaration",
					  node);
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

static void analyzeIdentifier(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_IDENTIFIER) {
		semanticError(INTERNAL_ERROR, "Invalid identifier node", node);
		return;
	}

	AVLNode *symbol = searchSymbolInScope(scope, node->data.identifier.name);
	if (symbol == NULL) {
		semanticError(SEMANTIC_UNDEFINITION_ERROR,
					  "Identifier used before declaration", node);
		return;
	}

	if (symbol->params.node_type == FUNCTION) {
		astNode *definition = symbol->params.definition;
		if (definition == NULL ||
			definition->data.funcDef.paramLists.count > 0) {
			semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
						  "Function must be called with its parameter lists",
						  node);
			return;
		}

		if (symbol->params.resolveState == TYPE_RESOLVING &&
			symbol->params.type == TYPE_NONE) {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "Cannot infer return type of recursive functions; "
						  "declare a return type",
						  node);
			return;
		}
		if (symbol->params.resolveState == TYPE_UNVISITED) {
			Scope *definitionScope = scope;
			while (definitionScope->parent != NULL) {
				definitionScope = definitionScope->parent;
			}
			analyzeFunction(definition, definitionScope,
							symbol->params.receiver);
		}
	}

	node->dataType = symbol->params.type;
	node->scopeId = symbol->params.scope;
/* 	fprintf(stderr, "[SEMANTIC] %s -> scope %d\n",
        node->data.identifier.name,
        node->scopeId); */
}

static void analyzeFunction(astNode *node, Scope *scope, astNode *receiver) {
	char *extensionKey =
		receiver != NULL ? makeExtensionPrefix(receiver->data.param.type,
											   node->data.funcDef.name, node)
						 : NULL;
	AVLNode *searchedFunction = searchSymbolInScope(
		scope, extensionKey != NULL ? extensionKey : node->data.funcDef.name);
	free(extensionKey);
	if (searchedFunction == NULL) {
		semanticError(INTERNAL_ERROR, "Function definition is not registered",
					  node);
		return;
	}
	if (searchedFunction->params.resolveState == TYPE_RESOLVED) {
		return;
	}
	if (searchedFunction->params.resolveState == TYPE_RESOLVING &&
		searchedFunction->params.type == TYPE_NONE) {
		semanticError(SEMANTIC_TYPE_ERROR,
					  "Cannot infer return type of recursive functions; "
					  "declare a return type",
					  node);
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
						  receiver != NULL ? receiver : node);
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
	for (size_t i = 0; i < node->data.funcDef.paramLists.count; i++) {

		astNode *list = node->data.funcDef.paramLists.items[i];

		for (size_t j = 0; j < list->data.paramList.params.count; j++) {

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
							  "Duplicate function parameter", param);
			}

			functionScope.symbols = insertNode(functionScope.symbols, info);
		}
	}

	analyzeNode(node->data.funcDef.body, &functionScope);

	astDataType declaredType = node->data.funcDef.retType;
	astDataType actualType = node->data.funcDef.body->dataType;

	if(node->data.funcDef.isMain){

		if(declaredType != TYPE_UNIT) {
			semanticError(SEMANTIC_TYPE_ERROR,
				"Main is not type UNIT",node);
		}
		if (mainNode != NULL){
			semanticError(SEMANTIC_REDEFINITION_ERROR,
				"Multiple main declarations",node);
		}
		mainNode = node;

		if (node->data.funcDef.paramLists.count != 1 ||
			node->data.funcDef.paramLists.items[0]->data.paramList.params.count != 0) {
			semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
				"Main must not have parameters",node);
		}
	} 


	if (actualType == TYPE_NONE) {
		semanticError(SEMANTIC_TYPE_ERROR,
					  "Cannot determine function return type", node);
		return;
	}

	bool finalStatementReturns =
		lastStatementReturns(node->data.funcDef.body);

		if (declaredType != TYPE_NONE &&
			declaredType != TYPE_UNIT &&
			!finalStatementReturns &&
			declaredType != actualType &&
			!(declaredType == TYPE_DOUBLE && actualType == TYPE_INT)) {

			semanticError(
				SEMANTIC_TYPE_ERROR,
				"Function body type mismatch",
				node
			);
			return;
	}


    astDataType finalType = TYPE_UNIT;
    if (declaredType != TYPE_UNIT){

        finalType = (declaredType == TYPE_NONE) ? actualType : declaredType;
    }

	node->dataType = finalType;
	node->data.funcDef.retType = finalType;

	searchedFunction->params.type = finalType;

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

static bool isArgCompatible(astDataType argType, astDataType paramType) {
	return argType == paramType || paramType == TYPE_ANY ||
		   (argType == TYPE_INT && paramType == TYPE_DOUBLE);
}

static void markIntToDouble(astNode *arg, astDataType paramType) {
	if (arg->dataType == TYPE_INT && paramType == TYPE_DOUBLE)
		arg->intToDouble = true;
}

static void analyzeCall(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_CALL) {
        semanticError(INTERNAL_ERROR, "Invalid function call node", node);
        return;
    }

    size_t totalArgs = 0;
    size_t totalLists = 0;

    astNode *callee = node;

    while (callee != NULL && callee->type == AST_CALL) {
		analyzeNodeList(callee->data.call.args, scope);

        totalArgs += callee->data.call.args.count;

        totalLists++;

        callee = callee->data.call.callee;
    }

    if (callee == NULL) {
        semanticError(INTERNAL_ERROR, "Missing call target", node);
        return;
    }

    if (callee->type != AST_IDENTIFIER &&
        callee->type != AST_MEMBER) {
        semanticError(INTERNAL_ERROR, "Unsupported callee type", callee);
        return;
    }

    AVLNode *callFunc = NULL;
	if (callee->type == AST_IDENTIFIER) {
		callFunc = searchSymbolInScope(
			scope, callee->data.identifier.name);
	} else if (callee->type == AST_MEMBER) {

		callFunc = lookupExtension(callee, scope);
	}


	if (callFunc == NULL) {
		semanticError(SEMANTIC_UNDEFINITION_ERROR,
					  "Function used before declaration", callee);
		return;
	}

	if (callFunc->params.node_type != FUNCTION) {
		semanticError(SEMANTIC_TYPE_ERROR, "Identifier is not a function",
					  callee);
		return;
	}

	if (callFunc->params.type == TYPE_NONE &&
		callFunc->params.resolveState == TYPE_RESOLVING) {
		semanticError(SEMANTIC_TYPE_ERROR,
					  "Cannot infer return type of recursive functions; "
					  "declare a return type",
					  node);
		return;
	}
	if (callFunc->params.definition != NULL &&
		callFunc->params.resolveState == TYPE_UNVISITED) {

		Scope *definitionScope = scope;
		while (definitionScope->parent != NULL) {
			definitionScope = definitionScope->parent;
		}
		analyzeFunction(callFunc->params.definition, definitionScope,
						callFunc->params.receiver);
	}

	astNode *definition = callFunc->params.definition;

	if (callee->type == AST_MEMBER && definition == NULL) {
		semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
					  "Member cannot be called with parentheses", node);
		return;
	}

	astNode **calls = malloc(totalLists * sizeof(astNode *));
	if (calls == NULL) {
		semanticError(INTERNAL_ERROR, "Memory allocation failed", node);
		return;
	}
	astNode *call = node;
	for (size_t i = totalLists; i > 0; i--) {
		calls[i - 1] = call;
		call = call->data.call.callee;
	}

	if (definition != NULL) {
		astNodeList paramLists = definition->data.funcDef.paramLists;

		if (totalLists != paramLists.count) {
			free(calls);
			semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
						  "Function call parameter list count mismatch", node);
			return;
		}

		for (size_t i = 0; i < totalLists; i++) {
			astNodeList args = calls[i]->data.call.args;
			astNodeList params =
				paramLists.items[i]->data.paramList.params;

			if (args.count != params.count) {
				free(calls);
				semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
							  "Function call argument count mismatch", node);
				return;
			}

			for (size_t j = 0; j < args.count; j++) {
				if (!isArgCompatible(args.items[j]->dataType,
									 params.items[j]->data.param.type)) {
					free(calls);
					semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
								  "Function call argument type mismatch",
								  args.items[j]);
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
						  "Function call argument count mismatch", node);
			return;
		}

		for (size_t j = 0; j < totalArgs; j++) {
			astNode *arg = node->data.call.args.items[j];
			if (!isArgCompatible(arg->dataType,
								 callFunc->params.param_list[j].type)) {
				free(calls);
				semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
							  "Function call argument type mismatch", arg);
				return;
			}
			markIntToDouble(arg, callFunc->params.param_list[j].type);
		}
	}

	free(calls);
	node->dataType = callFunc->params.type;
	return;
}

static void analyzeBinaryOp(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_BINARY_OP) {
		semanticError(INTERNAL_ERROR, "Invalid binary operation", node);
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
						  node);
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
						  "Invalid types for arithmetic operation", node);
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
						  "Invalid types for comparison operation", node);
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
						  "Invalid types for comparison operation", node);
		}
		break;
	case op_and:
	case op_or:
		if (leftType == TYPE_BOOL && rightType == TYPE_BOOL) {
			node->dataType = TYPE_BOOL;
		} else {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "Invalid types for logical operation", node);
		}
		break;
	default:
		break;
	}
}

static void analyzeUnaryOp(astNode *node, Scope *scope) {

	if (node == NULL || node->type != AST_UNARY_OP) {
		semanticError(INTERNAL_ERROR, "Invalid unary operation", node);
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
						  "Invalid type for logical NOT operation", node);
		}
		break;
	case op_sub:
		if (operandType == TYPE_INT || operandType == TYPE_DOUBLE) {
			node->dataType = operandType;
		} else {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "Invalid type for unary negation operation", node);
		}
		break;
	case op_add:
		if (operandType != TYPE_INT && operandType != TYPE_DOUBLE) {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "Invalid type for unary plus operation", node);
		} else {
			node->dataType = operandType;
		}
		break;
	default:
		break;
	}
}

static void registerFunction(astNode *node, Scope *scope, astNode *receiver) {
	if (node == NULL || node->type != AST_FUN_DEF) {
		semanticError(INTERNAL_ERROR, "Invalid function definition", node);
	}
	char *symbolName = node->data.funcDef.name;
	char *extensionKey = NULL;

	if (receiver != NULL) {
		extensionKey = makeExtensionPrefix(receiver->data.param.type,
										   node->data.funcDef.name, node);

		symbolName = extensionKey;
	}
	if (searchNode(scope->symbols, symbolName) != NULL) {
		semanticError(SEMANTIC_REDEFINITION_ERROR, "Function already defined",
					  node);
	}

	size_t paramCount = 0;

	for (size_t i = 0; i < node->data.funcDef.paramLists.count; i++) {
		astNode *list = node->data.funcDef.paramLists.items[i];

		if (list == NULL || list->type != AST_PARAM_LIST) {
			semanticError(INTERNAL_ERROR, "Invalid parameter list",
						  list != NULL ? list : node);
		}

		paramCount += list->data.paramList.params.count;
	}

	argInfo *args = NULL;

	if (paramCount > 0) {
		args = calloc(paramCount, sizeof(argInfo));

		if (args == NULL) {
			semanticError(INTERNAL_ERROR, "Memory allocation failed", node);
		}
	}

	size_t index = 0;

	for (size_t i = 0; i < node->data.funcDef.paramLists.count; i++) {
		astNode *list = node->data.funcDef.paramLists.items[i];

		for (size_t j = 0; j < list->data.paramList.params.count; j++) {
			astNode *paramNode = list->data.paramList.params.items[j];

			if (paramNode == NULL || paramNode->type != AST_PARAM) {
				semanticError(INTERNAL_ERROR, "Invalid parameter",
							  paramNode != NULL ? paramNode : node);
			}
			if (paramNode->data.param.name == NULL) {
				semanticError(INTERNAL_ERROR, "Parameter name is NULL",
							  paramNode);
			}
			if (paramNode->data.param.type == TYPE_NONE) {
				semanticError(INTERNAL_ERROR, "Parameter type is TYPE_NONE",
							  paramNode);
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

static void analyzeIf(astNode *node, Scope *scope) {
	analyzeNode(node->data.ifStmt.cond, scope);
	if (node->data.ifStmt.cond->dataType != TYPE_BOOL) {
		semanticError(SEMANTIC_TYPE_ERROR, "If condition must be of type bool",
					  node->data.ifStmt.cond);
	}
	analyzeNode(node->data.ifStmt.thenBody, scope);
	astDataType thenType = node->data.ifStmt.thenBody->dataType;

	if (node->data.ifStmt.elseBody == NULL) {
		node->dataType = TYPE_UNIT;
		return;
	}
	analyzeNode(node->data.ifStmt.elseBody, scope);
	astDataType elseType = node->data.ifStmt.elseBody->dataType;

	bool thenReturns = alwaysReturns(node->data.ifStmt.thenBody);
	bool elseReturns = alwaysReturns(node->data.ifStmt.elseBody);

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
					  "If branches have incompatible types", node);
	}

	if (thenType == TYPE_NONE || elseType == TYPE_NONE) {
		semanticError(SEMANTIC_TYPE_ERROR,
					  "Cannot determine if expression type", node);
	}
	return;
}

static void analyzeFor(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_FOR) {
		semanticError(INTERNAL_ERROR, "Invalid for loop node", node);
		return;
	}
	astNode *fromNode = node->data.forLoop.from;
	astNode *toNode = node->data.forLoop.to;
	astNode *stepNode = node->data.forLoop.step;
	astNode *bodyNode = node->data.forLoop.body;

	if (fromNode == NULL || toNode == NULL || bodyNode == NULL ||
		node->data.forLoop.iterName == NULL) {
		semanticError(INTERNAL_ERROR, "Invalid for loop structure", node);
		return;
	}

	analyzeNode(fromNode, scope);
	analyzeNode(toNode, scope);

	if (fromNode->dataType != TYPE_INT || toNode->dataType != TYPE_INT) {
		semanticError(
			SEMANTIC_TYPE_ERROR,
			"For loop 'from' and 'to' expressions must be of type int",
			fromNode->dataType != TYPE_INT ? fromNode : toNode);
	}
	if (stepNode != NULL) {
		analyzeNode(stepNode, scope);
		if (stepNode->dataType != TYPE_INT) {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "For loop 'step' expression must be of type int",
						  stepNode);
		}
		if (stepNode->data.intValue == 0) {
			semanticError(SEMANTIC_ERROR,
						  "For loop 'step' expression cannot be zero",
						  stepNode);
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

	for (size_t guardIndex = 0; guardIndex < node->data.forLoop.guards.count;
		 guardIndex++) {
		astNode *guardNode = node->data.forLoop.guards.items[guardIndex];
		if (guardNode == NULL) {
			semanticError(INTERNAL_ERROR, "Invalid for loop guard", node);
		}
		analyzeNode(guardNode, &forScope);
		if (guardNode->dataType != TYPE_BOOL) {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "For loop guard expressions must be of type bool",
						  guardNode);
		}
	}
	analyzeNode(bodyNode, &forScope);
	destroyAVL(forScope.symbols);
}

static void analyzeWhile(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_WHILE) {
		semanticError(INTERNAL_ERROR, "Invalid while loop node", node);
		return;
	}

	if (node->data.whileLoop.cond == NULL ||
		node->data.whileLoop.body == NULL) {
		semanticError(INTERNAL_ERROR, "Invalid while loop structure", node);
		return;
	}

	analyzeNode(node->data.whileLoop.cond, scope);
	if (node->data.whileLoop.cond->dataType != TYPE_BOOL) {
		semanticError(SEMANTIC_TYPE_ERROR,
					  "While condition must be of type bool",
					  node->data.whileLoop.cond);
	}
	Scope whileScope = {.symbols = initTree(),
						.parent = scope,
						.id = scopeIdCounter++,
						.depth = scope->depth + 1};

	analyzeNode(node->data.whileLoop.body, &whileScope);
	destroyAVL(whileScope.symbols);
}

static void registerExtension(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_EXTENSION) {
		semanticError(INTERNAL_ERROR, "Invalid extension node", node);
		return;
	}

	astNode *receiver = node->data.extension.receiver;

	if (receiver == NULL || receiver->type != AST_PARAM ||
		receiver->data.param.type == TYPE_NONE) {
		semanticError(SEMANTIC_TYPE_ERROR, "Invalid extension receiver",
					  receiver != NULL ? receiver : node);
	}

	for (size_t i = 0; i < node->data.extension.funDefs.count; i++) {
		astNode *method = node->data.extension.funDefs.items[i];

		registerFunction(method, scope, receiver);
	}
}

static void analyzeExtension(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_EXTENSION) {
		semanticError(INTERNAL_ERROR, "Invalid extension node", node);
		return;
	}

	if (node->data.extension.receiver == NULL) {
		semanticError(INTERNAL_ERROR, "Invalid extension structure", node);
		return;
	}

	astNode *receiver = node->data.extension.receiver;

	for (size_t i = 0; i < node->data.extension.funDefs.count; i++) {
		astNode *method = node->data.extension.funDefs.items[i];

		analyzeFunction(method, scope, receiver);
	}
}

static char *makeExtensionPrefix(astDataType type, const char *name,
								 const astNode *node) {
	int len = snprintf(NULL, 0, "#EXT:%d:%s", (int)type, name);

	if (len < 0) {
		semanticError(INTERNAL_ERROR, "Cannot create extension prefix string",
					  node);
	}

	char *key = malloc((size_t)len + 1);

	if (key == NULL) {
		semanticError(INTERNAL_ERROR, "Memory allocation failed", node);
	}

	snprintf(key, (size_t)len + 1, "#EXT:%d:%s", (int)type, name);

	return key;
}

static void analyzeMember(astNode *node, Scope *scope) {
	AVLNode *method = lookupExtension(node, scope);

	if (method == NULL) {
		return;
	}

	astNode *definition = method->params.definition;

	if (definition != NULL && definition->data.funcDef.paramLists.count > 0) {

		semanticError(SEMANTIC_ARGUMENT_COUNT_ERROR,
					  "Extension method requires parentheses", node);
		return;
	}

	node->dataType = method->params.type;
	return;
}

static AVLNode *lookupExtension(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_MEMBER) {
		semanticError(INTERNAL_ERROR, "Invalid member node", node);
		return NULL;
	}

	astNode *object = node->data.member.object;

	if (object == NULL || node->data.member.name == NULL) {
		semanticError(INTERNAL_ERROR, "Invalid member structure", node);
		return NULL;
	}

	analyzeNode(object, scope);

	if (object->dataType == TYPE_NONE) {
		semanticError(SEMANTIC_TYPE_ERROR, "Unknown member object type",
					  object);
		return NULL;
	}

	char *key =
		makeExtensionPrefix(object->dataType, node->data.member.name, node);

	AVLNode *symbol = searchSymbolInScope(scope, key);

	free(key);

	if (symbol == NULL) {
		semanticError(SEMANTIC_UNDEFINITION_ERROR, "Undefined extension method",
					  node);
		return NULL;
	}

	if (symbol->params.node_type != FUNCTION) {
		semanticError(SEMANTIC_TYPE_ERROR, "Member is not a function", node);
		return NULL;
	}

	return symbol;
}

static void analyzeReturn(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_RETURN) {
		semanticError(INTERNAL_ERROR, "Invalid return node", node);
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
		semanticError(10, "Return outside function", node);
		return;
	}

	astDataType expectedType = function->data.funcDef.retType;

	if (expectedType == TYPE_NONE) {
		semanticError(SEMANTIC_TYPE_ERROR,
					  "Function with return must declare return type", node);
		return;
	}

	astNode *value = node->data.ret.value;

	if (value == NULL) {
		if (expectedType != TYPE_UNIT) {
			semanticError(SEMANTIC_TYPE_ERROR, "Missing return value", node);
			return;
		}

		node->dataType = TYPE_UNIT;
		return;
	}

	analyzeNode(value, scope);

	astDataType actualType = value->dataType;

	if (actualType == TYPE_NONE) {
		semanticError(SEMANTIC_TYPE_ERROR, "Invalid return expression", value);
		return;
	}

	if (expectedType == TYPE_UNIT) {
		node->dataType = TYPE_UNIT;
		return;
	}

	if (actualType != expectedType &&
		!(expectedType == TYPE_DOUBLE && actualType == TYPE_INT)) {

		semanticError(SEMANTIC_TYPE_ERROR, "Return type mismatch", value);
		return;
	}

	node->dataType = expectedType;
}

static void analyzeInterp(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_INTERP) {
		semanticError(INTERNAL_ERROR, "Invalid interpolation node", node);
		return;
	}

	for (size_t i = 0; i < node->data.interp.parts.count; i++) {
		astNode *part = node->data.interp.parts.items[i];

		if (part == NULL) {
			semanticError(INTERNAL_ERROR, "Invalid interpolation part", node);
			return;
		}

		analyzeNode(part, scope);

		if (part->dataType == TYPE_NONE) {
			semanticError(SEMANTIC_TYPE_ERROR,
						  "Invalid interpolation expression", part);
			return;
		}
	}

	node->dataType = TYPE_STRING;
}

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

static bool lastStatementReturns(const astNode* body) {
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
