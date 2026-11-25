/**
 * @name ifj26
 * @authors Jakub Geleta (xgeletj00)
 * @file parser.c
 *
 * AI usage:
 * "Create function stubs according to the language grammar"
 * "Fill the precedence table from the .md file"
 * "Update the precedence table so it is up to date with the .md file"
 * "Add debug prints for parsing"
 */

#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "scanner.h"
#include "error.h"
#include <string.h>

static parserState parser;

// ------------------------------------
/*
 * DEBUG PRINTS
 *
 *
 * Set PARSER_DEBUG to 0 (or compile with -DPARSER_DEBUG=0) to turn them off.
 * They go to stderr, so they never mix with the generated code on stdout.
 */
#ifndef PARSER_DEBUG
#define PARSER_DEBUG 0
#endif

#if PARSER_DEBUG
#include "test/tokenDump.h"

static void parserDebugToken(const char *label, token tok) {
	fprintf(stderr, "%s %s", label, specName(tok.specType));

	if ((tok.specType == id_idtName || tok.specType == lit_string ||
		 tok.specType == intp_part) &&
		tok.value.word != NULL) {
		fprintf(stderr, " '%s'", tok.value.word);
	} else if (tok.specType == lit_int) {
		fprintf(stderr, " %lld", tok.value.num);
	} else if (tok.specType == lit_double) {
		fprintf(stderr, " %g", tok.value.decNum);
	}

	fprintf(stderr, " (%d:%d)\n", tok.line, tok.col);
}

// prints the function that is being entered and the token it starts on
#define PARSER_TRACE()                                                         \
	do {                                                                       \
		fprintf(stderr, "[parser] >> %-16s", __func__);                        \
		parserDebugToken(" at", parser.current);                               \
	} while (0)
// prints every token the parser reads
#define PARSER_DEBUG_TOKEN(tok)                                                \
	do {                                                                       \
		fprintf(stderr, "[parser]    ");                                       \
		parserDebugToken("token", (tok));                                      \
	} while (0)
#else
#define PARSER_TRACE() ((void)0)
#define PARSER_DEBUG_TOKEN(tok) ((void)0)
#endif
// ------------------------------------

const char precedenceTable[PREC_COUNT][PREC_COUNT] = {
	{'>', '>', '<', '<', '>', '>', '>', '>', '>', '>', '<',
	 '<', '<', '>', '<', '<', '>', '>', '>', '<', '>'},
	{'>', '>', '<', '<', '>', '>', '>', '>', '>', '>', '<',
	 '<', '<', '>', '<', '<', '>', '>', '>', '<', '>'},
	{'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<',
	 '<', '<', '>', '<', '<', '>', '>', '>', '<', '>'},
	{'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<',
	 '<', '<', '>', '<', '<', '>', '>', '>', '<', '>'},
	{'<', '<', '<', '<', ' ', ' ', ' ', ' ', '>', '>', '<',
	 '<', '<', '>', '<', '<', '>', '>', '>', '<', '>'},
	{'<', '<', '<', '<', ' ', ' ', ' ', ' ', '>', '>', '<',
	 '<', '<', '>', '<', '<', '>', '>', '>', '<', '>'},
	{'<', '<', '<', '<', ' ', ' ', ' ', ' ', '>', '>', '<',
	 '<', '<', '>', '<', '<', '>', '>', '>', '<', '>'},
	{'<', '<', '<', '<', ' ', ' ', ' ', ' ', '>', '>', '<',
	 '<', '<', '>', '<', '<', '>', '>', '>', '<', '>'},
	{'<', '<', '<', '<', '<', '<', '<', '<', ' ', ' ', '<',
	 '<', '<', '>', '<', '<', '>', '>', '>', '<', '>'},
	{'<', '<', '<', '<', '<', '<', '<', '<', ' ', ' ', '<',
	 '<', '<', '>', '<', '<', '>', '>', '>', '<', '>'},
	{'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<',
	 '<', '<', '>', '<', '<', '>', '>', '>', '<', '>'},
	{'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<',
	 '<', '<', '>', '<', '<', '>', '>', '>', '<', '>'},
	{'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<',
	 '<', '<', '=', '<', '<', '<', '<', '<', '<', ' '},
	{'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', ' ',
	 ' ', '>', '>', ' ', ' ', '>', '>', '>', '>', '>'},
	{'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', ' ',
	 ' ', '=', '>', ' ', ' ', '>', '>', '>', '>', '>'},
	{'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<',
	 '<', '<', '>', '<', '<', '>', '>', '>', '<', '>'},
	{'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<',
	 '<', '<', '>', '<', '<', '>', '>', '>', '<', '>'},
	{'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<',
	 '<', '<', '>', '<', '<', '<', '>', '>', '<', '>'},
	{'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<',
	 '<', '<', '>', '<', '<', '<', '<', '>', '<', '>'},
	{'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<',
	 '<', '<', '>', '<', '<', '>', '>', '>', '>', '>'},
	{'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<',
	 '<', '<', ' ', '<', '<', '<', '<', '<', '<', ' '},
};

/**
 * @brief Checks any unattached nodes and frees them
 */
void parserCleanup(void) {
	if (!parser.active)
		return;
	parser.active = false;
	tokenFree(&parser.current);

	while (parser.unattachedCount > 0) {
		parser.unattachedCount--;
		astFree(parser.unattached[parser.unattachedCount]);
	}
	free(parser.unattached);
	parser.unattached = NULL;
	parser.unattachedCapacity = 0;

	while (!isEmpty(&parser.exprStack))
		pop(&parser.exprStack);
}

/**
 * @brief Helper function, tracks nodes that werent attached yet
 * 		  so when code crashes, they dont stay lost
 * @param node Pointer to a node that should be tracked
 * @return Pointer to the tracked node
 */
astNode *parserTrackUnattached(astNode *node) {
	if (node == NULL)
		return NULL;

	if (parser.unattachedCount == parser.unattachedCapacity) {
		size_t newCapacity =
			parser.unattachedCapacity == 0 ? 16 : parser.unattachedCapacity * 2;
		astNode **newUnattached =
			realloc(parser.unattached, newCapacity * sizeof(astNode *));
		if (newUnattached == NULL) {
			astFree(node);
			error(INTERNAL_ERROR,
				  "Parser unattached node stack realloc memory failed");
		}
		parser.unattached = newUnattached;
		parser.unattachedCapacity = newCapacity;
	}

	parser.unattached[parser.unattachedCount++] = node;
	return node;
}

/**
 * @brief Helper function, unattaches the node, when it is in the AST
 * @param node Pointer to a node that should be unattached
 * @return Pointer to the unattached node
 */
astNode *parserMarkAttached(astNode *node) {
	if (node == NULL)
		return NULL;

	for (size_t i = parser.unattachedCount; i > 0; i--) {
		if (parser.unattached[i - 1] == node) {
			memmove(&parser.unattached[i - 1], &parser.unattached[i],
					(parser.unattachedCount - i) * sizeof(astNode *));
			parser.unattachedCount--;
			return node;
		}
	}
	return node;
}

/**
 * @brief Helper function, appends the node and makes it attached
 * @param list Node list to append to
 * @param node Node to append
 */
void parserListAppend(astNodeList *list, astNode *node) {
	astListAppend(list, node);
	parserMarkAttached(node);
}

astNode *parse(void) {
	static bool cleanupRegistered = false;
	if (!cleanupRegistered) {
		if (atexit(parserCleanup) != 0)
			error(INTERNAL_ERROR, "Failed to register parser cleanup");
		cleanupRegistered = true;
	}

	memset(&parser, 0, sizeof(parser));
	parser.active = true;

	scannerInit();

	astNode *program = parseProgram();

	if (program == NULL) {
		error(SYNTAX_ERROR, "Invalid program structure");
	}

	parserMarkAttached(program);
	parserCleanup();
	scannerFree();
	return program;
}

void parserAdvance(void) {
	parser.current.value.word = NULL;
	parser.current = getToken();
	PARSER_DEBUG_TOKEN(parser.current);
}

bool parserCheck(specTokenType specType) {
	return parser.current.specType == specType;
}

bool parserMatch(specTokenType specType) {
	if (!parserCheck(specType))
		return false;
	parserAdvance();
	return true;
}

/**
 * @brief Helper function, skips new lines in code
 */
void skipNL(void) {
	while (parserCheck(ps_newLine)) {
		parserAdvance();
	}
}

astNode *parseProgram(void) {
	PARSER_TRACE();
	parserAdvance();
	skipNL();

	if (!parserCheck(kw_import))
		error(SYNTAX_ERROR, "Expected 'import' at the start of the program");
	int line = parser.current.line;
	int col = parser.current.col;
	parserAdvance();

	if (!parserCheck(id_idtName) ||
		strcmp(parser.current.value.word, "ifj26") != 0)
		error(SYNTAX_ERROR, "Expected 'ifj26' after 'import'");

	astNode *program = parserTrackUnattached(astNewProgram(NULL, line, col));
	program->data.program.importName = parser.current.value.word;
	parserAdvance();

	if (!parserMatch(sn_dot))
		error(SYNTAX_ERROR, "Expected '.' after 'import ifj26'");
	if (!parserMatch(op_mul))
		error(SYNTAX_ERROR, "Expected '*' after 'import ifj26.'");
	if (!parserCheck(ps_newLine))
		error(SYNTAX_ERROR, "Expected newline after 'import ifj26.*'");

	parseGlobalDefs(&program->data.program.defs);

	if (!parserCheck(ps_eof))
		error(SYNTAX_ERROR, "Expected end of file");

	return program;
}

void parseGlobalDefs(astNodeList *defs) {
	PARSER_TRACE();
	bool mainFound = false;

	skipNL();
	while (!parserCheck(ps_eof)) {
		astNode *def = parseGlobalDef();
		if (def == NULL)
			error(SYNTAX_ERROR, "Invalid global definition");

		parserListAppend(defs, def);

		if (def->type == AST_FUN_DEF && def->data.funcDef.isMain) {
			if (mainFound) {
				error(SEMANTIC_REDEFINITION_ERROR,
					  "Multiple main functions found");
			}
			mainFound = true;
		}

		skipNL();
	}
}

astNode *parseGlobalDef(void) {
	PARSER_TRACE();
	if (parserCheck(kw_extension)) {
		return parseExtension();
	} else if (parserCheck(kw_def)) {
		return parseFuncDef(false);
	} else if (parserCheck(kw_main)) {
		parserAdvance();
		skipNL();
		return parseFuncDef(true);
	}

	return NULL;
}

astNode *parseExtension(void) {
	PARSER_TRACE();
	int line = parser.current.line;
	int col = parser.current.col;
	parserAdvance();

	if (!parserMatch(br_start))
		error(SYNTAX_ERROR, "Expected '(' after 'extension'");
	if (!parserCheck(id_idtName))
		error(SYNTAX_ERROR, "Expected a identifier in extension");

	astNode *extension =
		parserTrackUnattached(astNewExtension(NULL, line, col));
	extension->data.extension.receiver = parserMarkAttached(parseParam());

	if (!parserMatch(br_end))
		error(SYNTAX_ERROR, "Expected ')' after extension");

	parseExtBody(extension);

	return extension;
}

void parseExtBody(astNode *extension) {
	PARSER_TRACE();

	astNodeList *funcDefs = &extension->data.extension.funDefs;

	if (parserMatch(ps_newLine)) {
		if (!parserMatch(ps_indent))
			error(SYNTAX_ERROR, "Expected indented extension body");

		do {
			if (!parserCheck(kw_def))
				error(SYNTAX_ERROR, "Expected 'def' in extension body");

			parserListAppend(funcDefs, parseFuncDef(false));
			skipNL();
		} while (!parserCheck(ps_dedent) && !parserCheck(ps_eof));

		if (!parserMatch(ps_dedent))
			error(SYNTAX_ERROR, "Expected end of extension body");
		return;
	}

	if (!parserCheck(kw_def))
		error(SYNTAX_ERROR, "Expected 'def' after extension");
	parserListAppend(funcDefs, parseFuncDef(false));
}

astNode *parseFuncDef(bool isMain) {
	PARSER_TRACE();
	if (!parserMatch(kw_def))
		error(SYNTAX_ERROR, "Expected 'def'");
	if (!parserCheck(id_idtName))
		error(SYNTAX_ERROR, "Expected function name after 'def'");

	astNode *funcDef = parserTrackUnattached(
		astNewFuncDef(parser.current.value.word, isMain, parser.current.line,
					  parser.current.col));
	parserAdvance();

	parseParamLists(&funcDef->data.funcDef.paramLists);

	funcDef->data.funcDef.retType = parseTypeOpt();

	if (!parserMatch(op_assign))
		error(SYNTAX_ERROR, "Expected '=' in function definition");

	if (parserMatch(ps_newLine)) {
		if (!parserMatch(ps_indent))
			error(SYNTAX_ERROR, "Expected indented function body");

		funcDef->data.funcDef.body = parserMarkAttached(parseBlock());
		if (funcDef->data.funcDef.body == NULL)
			error(SYNTAX_ERROR, "Expected function body");

		if (!parserMatch(ps_dedent))
			error(SYNTAX_ERROR, "Expected end of function body");
	} else {
		funcDef->data.funcDef.body = parserMarkAttached(parseValue());
		if (funcDef->data.funcDef.body == NULL)
			error(SYNTAX_ERROR, "Expected function body");
	}

	return funcDef;
}

void parseParamLists(astNodeList *paramLists) {
	PARSER_TRACE();
	while (parserCheck(br_start)) {
		astNode *paramList = parserTrackUnattached(
			astNewParamList(parser.current.line, parser.current.col));
		parserListAppend(paramLists, paramList);
		parserAdvance();

		parseParams(&paramList->data.paramList.params);

		if (!parserMatch(br_end))
			error(SYNTAX_ERROR, "Expected ')' to end parameter list");
	}
}

void parseParams(astNodeList *params) {
	PARSER_TRACE();
	if (!parserCheck(id_idtName))
		return;

	parserListAppend(params, parseParam());

	while (parserCheck(sn_comma)) {
		// the scanner swallows newlines inside parentheses
		int commaLine = parser.current.line;
		parserAdvance();

		if (!parserCheck(id_idtName)) {
			if (parser.current.line == commaLine)
				error(SYNTAX_ERROR,
					  "Trailing comma only allowed when a new line follows");
			break;
		}

		parserListAppend(params, parseParam());
	}
}

astNode *parseParam(void) {
	PARSER_TRACE();
	if (!parserCheck(id_idtName))
		return NULL;

	astNode *param = parserTrackUnattached(
		astNewParam(NULL, TYPE_NONE, parser.current.line, parser.current.col));
	param->data.param.name = parser.current.value.word;
	parserAdvance();

	if (!parserMatch(sn_colon))
		error(SYNTAX_ERROR, "Expected ':' after parameter name");

	param->data.param.type = parseType();
	if (param->data.param.type == TYPE_NONE)
		error(SYNTAX_ERROR, "Expected type of parameter");

	return param;
}

astDataType parseTypeOpt(void) {
	PARSER_TRACE();
	if (!parserMatch(sn_colon))
		return TYPE_NONE;

	astDataType type = parseType();
	if (type == TYPE_NONE)
		error(SYNTAX_ERROR, "Expected type after ':'");
	return type;
}

astDataType parseType(void) {
	PARSER_TRACE();
	astDataType type = astTypeFromToken(parser.current.specType);
	if (type != TYPE_NONE)
		parserAdvance();
	return type;
}

astNode *parseBody(void) {
	PARSER_TRACE();

	if (parserMatch(ps_newLine)) {
		if (!parserMatch(ps_indent))
			error(SYNTAX_ERROR, "Expected indented block");

		astNode *block = parseBlock();
		if (!parserMatch(ps_dedent))
			error(SYNTAX_ERROR, "Expected dedent after block");
		return block;
	}

	return parseStmt();
}

astNode *parseBlock(void) {
	PARSER_TRACE();

	astNode *block = parserTrackUnattached(
		astNewBlock(parser.current.line, parser.current.col));

	skipNL();
	while (!parserCheck(ps_dedent) && !parserCheck(ps_eof)) {
		parserListAppend(&block->data.block.stmts, parseStmt());
		skipNL();
	}

	return block;
}

astNode *parseStmt(void) {
	PARSER_TRACE();

	astNode *stmt = NULL;

	if (parserCheck(kw_var)) {
		stmt = parseVarDecl(true);
	} else if (parserCheck(kw_val)) {
		stmt = parseVarDecl(false);
	} else if (parserCheck(kw_return)) {
		stmt = parseReturn();
	} else if (parserCheck(kw_while)) {
		stmt = parseWhile();
	} else if (parserCheck(kw_if)) {
		stmt = parseIf(false);
	} else if (parserCheck(kw_for)) {
		stmt = parseFor();
	} else {
		stmt = parseSimple();
	}

	if (stmt == NULL)
		error(SYNTAX_ERROR, "Invalid statement");

	return stmt;
}

astNode *parseSimple(void) {
	PARSER_TRACE();

	astNode *expr = parseExpr();
	if (expr == NULL)
		error(SYNTAX_ERROR, "Expected expression in statement");

	if (parserCheck(op_assign)) {
		astNode *assign = parserTrackUnattached(
			astNewAssign(expr, NULL, parser.current.line, parser.current.col));
		parserMarkAttached(expr);
		parserAdvance();

		assign->data.assign.value = parserMarkAttached(parseValue());
		if (assign->data.assign.value == NULL)
			error(SYNTAX_ERROR, "Expected value in assignment");

		return assign;
	}

	astNode *exprStmt =
		parserTrackUnattached(astNewExprStmt(expr, expr->line, expr->col));
	parserMarkAttached(expr);

	if (!parserCheck(ps_newLine) && !parserCheck(sn_semiColon))
		error(SYNTAX_ERROR, "Expected end of line after statement");
	parserAdvance();
	return exprStmt;
}

astNode *parseInline(void) {
	PARSER_TRACE();

	if (parserCheck(kw_return)) {
		astNode *ret = parserTrackUnattached(
			astNewReturn(NULL, parser.current.line, parser.current.col));
		parserAdvance();

		ret->data.ret.value = parserMarkAttached(parseExpr());
		return ret;
	}

	astNode *expr = parseExpr();
	if (expr == NULL)
		return NULL;

	if (parserCheck(op_assign)) {
		astNode *assign = parserTrackUnattached(
			astNewAssign(expr, NULL, parser.current.line, parser.current.col));
		parserMarkAttached(expr);
		parserAdvance();

		assign->data.assign.value = parserMarkAttached(parseExpr());
		if (assign->data.assign.value == NULL)
			error(SYNTAX_ERROR, "Expected value in assignment");

		return assign;
	}

	astNode *exprStmt =
		parserTrackUnattached(astNewExprStmt(expr, expr->line, expr->col));
	parserMarkAttached(expr);
	return exprStmt;
}

astNode *parseVarDecl(bool isMutable) {
	PARSER_TRACE();

	parserAdvance();
	if (!parserCheck(id_idtName))
		error(SYNTAX_ERROR, "Expected variable name after 'var' or 'val'");
	astNode *var = parserTrackUnattached(
		astNewVarDecl(parser.current.value.word, TYPE_NONE, NULL, isMutable,
					  parser.current.line, parser.current.col));
	parserAdvance();

	var->data.varDecl.declType = parseTypeOpt();

	if (!parserMatch(op_assign))
		error(SYNTAX_ERROR, "Expected '=' in variable declaration");

	var->data.varDecl.value = parserMarkAttached(parseValue());
	if (var->data.varDecl.value == NULL)
		error(SYNTAX_ERROR, "Expected value in variable declaration");

	return var;
}

astNode *parseReturn(void) {
	PARSER_TRACE();

	astNode *ret = parserTrackUnattached(
		astNewReturn(NULL, parser.current.line, parser.current.col));
	parserAdvance();

	if (parserCheck(ps_newLine) || parserCheck(sn_semiColon)) {
		ret->data.ret.value = NULL;
		parserAdvance();
		return ret;
	}

	ret->data.ret.value = parserMarkAttached(parseValue());
	return ret;
}

astNode *parseWhile(void) {
	PARSER_TRACE();
	astNode *whileNode = parserTrackUnattached(
		astNewWhile(NULL, NULL, parser.current.line, parser.current.col));
	parserAdvance();

	whileNode->data.whileLoop.cond = parserMarkAttached(parseExpr());
	if (whileNode->data.whileLoop.cond == NULL)
		error(SYNTAX_ERROR, "Expected condition in while loop");

	if (parserMatch(kw_do) != true)
		error(SYNTAX_ERROR, "Expected 'do' after while loop condition");

	whileNode->data.whileLoop.body = parserMarkAttached(parseBody());
	if (whileNode->data.whileLoop.body == NULL)
		error(SYNTAX_ERROR, "Expected body in while loop");

	return whileNode;
}

astNode *parseValue(void) {
	PARSER_TRACE();

	if (parserCheck(kw_if))
		return parseIf(true);

	astNode *value = parseExpr();

	if (value != NULL && !parserMatch(ps_newLine) && !parserMatch(sn_semiColon))
		error(SYNTAX_ERROR, "Expected end of line after value");

	return value;
}

astNode *parseIfHead(void) {
	PARSER_TRACE();

	astNode *ifNode = parserTrackUnattached(
		astNewIf(NULL, NULL, NULL, parser.current.line, parser.current.col));
	parserAdvance();

	ifNode->data.ifStmt.cond = parserMarkAttached(parseExpr());
	if (ifNode->data.ifStmt.cond == NULL)
		error(SYNTAX_ERROR, "Expected condition in if");

	if (!parserMatch(kw_then))
		error(SYNTAX_ERROR, "Expected 'then' after if condition");

	return ifNode;
}

astNode *parseIf(bool isValue) {
	PARSER_TRACE();

	astNode *ifNode = parseIfHead();

	if (parserCheck(ps_newLine)) {
		ifNode->data.ifStmt.thenBody = parserMarkAttached(parseBody());

		if (!parserMatch(kw_else)) {
			if (isValue)
				error(SYNTAX_ERROR,
					  "Expected else statement when if is used as a value");
			return ifNode;
		}
	} else {
		ifNode->data.ifStmt.thenBody =
			parserMarkAttached(isValue ? parseExprOrIf() : parseInline());
		if (ifNode->data.ifStmt.thenBody == NULL)
			error(SYNTAX_ERROR, isValue ? "Expected value after 'then'"
										: "Expected statement after 'then'");

		if (parserMatch(ps_newLine) || parserMatch(sn_semiColon)) {
			if (isValue)
				error(SYNTAX_ERROR,
					  "Expected else statement when if is used as a value");
			return ifNode;
		}

		if (!parserMatch(kw_else))
			error(SYNTAX_ERROR,
				  "Expected 'else' or end of line after then branch");
	}

	if (isValue && !parserCheck(ps_newLine))
		ifNode->data.ifStmt.elseBody = parserMarkAttached(parseValue());
	else
		ifNode->data.ifStmt.elseBody = parserMarkAttached(parseBody());

	if (ifNode->data.ifStmt.elseBody == NULL)
		error(SYNTAX_ERROR, "Expected statement after 'else'");

	return ifNode;
}

/**
 * @brief Helper function, checks if a special word is being parsed, used in For
 * loop
 * @param word Word to check
 * @return True if found, false otherwise
 */
bool parserCheckWord(const char *word) {
	return parserCheck(id_idtName) &&
		   strcmp(parser.current.value.word, word) == 0;
}

astNode *parseFor(void) {
	PARSER_TRACE();

	astNode *forNode = parserTrackUnattached(
		astNewFor(NULL, parser.current.line, parser.current.col));
	parserAdvance();

	parseForHeader(forNode);

	if (!parserMatch(kw_do))
		error(SYNTAX_ERROR, "Expected 'do' after for header");

	forNode->data.forLoop.body = parserMarkAttached(parseBody());
	if (forNode->data.forLoop.body == NULL)
		error(SYNTAX_ERROR, "Expected body in for loop");

	return forNode;
}

void parseForHeader(astNode *forNode) {
	PARSER_TRACE();

	if (parserMatch(ps_newLine)) {
		if (!parserMatch(ps_indent))
			error(SYNTAX_ERROR, "Expected indented for header");

		parseGenerator(forNode);
		if (!parserMatch(ps_newLine))
			error(SYNTAX_ERROR, "Expected end of line after for generator");

		parseGuardLines(&forNode->data.forLoop.guards);

		if (!parserMatch(ps_dedent))
			error(SYNTAX_ERROR, "Expected end of for header");
		return;
	}

	parseGenerator(forNode);
	parseGuards(&forNode->data.forLoop.guards);
}

void parseGenerator(astNode *forNode) {
	PARSER_TRACE();

	if (!parserCheck(id_idtName))
		error(SYNTAX_ERROR, "Expected loop variable after 'for'");
	forNode->data.forLoop.iterName = parser.current.value.word;
	parserAdvance();

	if (!parserMatch(ct_arrowLeft))
		error(SYNTAX_ERROR, "Expected '<-' after loop variable");

	forNode->data.forLoop.from = parserMarkAttached(parseExpr());
	if (forNode->data.forLoop.from == NULL)
		error(SYNTAX_ERROR, "Expected start of range in for");

	if (parserCheckWord("to")) {
		forNode->data.forLoop.isInclusive = true;
	} else if (parserCheckWord("until")) {
		forNode->data.forLoop.isInclusive = false;
	} else {
		error(SYNTAX_ERROR, "Expected 'to' or 'until' in for range");
	}
	tokenFree(&parser.current);
	parserAdvance();

	forNode->data.forLoop.to = parserMarkAttached(parseExpr());
	if (forNode->data.forLoop.to == NULL)
		error(SYNTAX_ERROR, "Expected end of range in for");

	if (parserCheckWord("by")) {
		tokenFree(&parser.current);
		parserAdvance();

		forNode->data.forLoop.step = parserMarkAttached(parseExpr());
		if (forNode->data.forLoop.step == NULL)
			error(SYNTAX_ERROR, "Expected step after 'by'");
	}
}

void parseGuards(astNodeList *guards) {
	PARSER_TRACE();

	while (parserMatch(kw_if)) {
		astNode *guard = parseExpr();
		if (guard == NULL)
			error(SYNTAX_ERROR, "Expected condition after 'if' in for");

		parserListAppend(guards, guard);
	}
}

void parseGuardLines(astNodeList *guards) {
	PARSER_TRACE();

	while (parserMatch(kw_if)) {
		astNode *guard = parseExpr();
		if (guard == NULL)
			error(SYNTAX_ERROR, "Expected condition after 'if' in for");

		parserListAppend(guards, guard);

		if (!parserMatch(ps_newLine))
			error(SYNTAX_ERROR, "Expected end of line after for guard");
	}
}

// Code used for the precedence

/**
 * @brief Helper function, checks if the symbol is a terminal with the given
 * precedence
 * @param symbol The expression symbol
 * @param prec Precedence the terminal should have
 * @return True if it is that terminal, false otherwise
 */
bool exprIsTerm(const exprSymbol *symbol, int prec) {
	return symbol->kind == EXPR_TERM && symbol->prec == prec;
}

/**
 * @brief Helper function, returns the precedence of the topmost terminal
 * 		  on the expression stack (skips a nonterminal on top)
 * @return Precedence of the top terminal
 */
int exprTopTerminalPrec(void) {
	if (top(&parser.exprStack)->kind == EXPR_TERM)
		return top(&parser.exprStack)->prec;

	exprSymbol e = pop(&parser.exprStack);
	int prec = top(&parser.exprStack)->prec;
	push(&parser.exprStack, e);
	return prec;
}

/**
 * @brief Helper function, checks if the top of the stack is an operand
 * 		  (nonterminal, identifier / literal or ')')
 * @return True if the next token comes after an operand, false otherwise
 */
bool exprAfterOperand(void) {
	const exprSymbol *topSymbol = top(&parser.exprStack);
	return topSymbol->kind == EXPR_NONTERM || exprIsTerm(topSymbol, PREC_ID) ||
		   exprIsTerm(topSymbol, PREC_RBR);
}

/**
 * @brief Helper function, classifies the current token (returns a precedence)
 * @param afterOperand If the token comes after an operand,
 * 		  used to tell apart unary and binary operators
 * @param parenDepth How many brackets are open, used for brackets and commas
 * @return Returns a precedence, PREC_END if the token ends the expression
 */
int exprClassify(bool afterOperand, int parenDepth) {
	switch (parser.current.specType) {
	case op_add:
		return afterOperand ? PREC_ADD : PREC_UPLUS;
	case op_sub:
		return afterOperand ? PREC_SUB : PREC_UMINUS;
	case op_not:
		return afterOperand ? PREC_END : PREC_NOT;

	case op_mul:
		return afterOperand ? PREC_MUL : PREC_END;
	case op_div:
		return afterOperand ? PREC_DIV : PREC_END;
	case op_lowerThen:
		return afterOperand ? PREC_LT : PREC_END;
	case op_lowerEqual:
		return afterOperand ? PREC_LE : PREC_END;
	case op_biggerThen:
		return afterOperand ? PREC_GT : PREC_END;
	case op_biggerEqual:
		return afterOperand ? PREC_GE : PREC_END;
	case op_equal:
		return afterOperand ? PREC_EQ : PREC_END;
	case op_notEqual:
		return afterOperand ? PREC_NE : PREC_END;
	case op_and:
		return afterOperand ? PREC_AND : PREC_END;
	case op_or:
		return afterOperand ? PREC_OR : PREC_END;

	case br_start:
		return PREC_LBR;
	case br_end:
		return parenDepth > 0 ? PREC_RBR : PREC_END;
	case sn_comma:
		return parenDepth > 0 ? PREC_COMMA : PREC_END;
	case sn_dot:
		return afterOperand ? PREC_DOT : PREC_END;

	case id_idtName:
	case lit_int:
	case lit_double:
	case lit_string:
	case lit_true:
	case lit_false:
		return afterOperand ? PREC_END : PREC_ID;

	case intp_begin:
		return afterOperand ? PREC_END : PREC_ID;

	case kw_if:
		return afterOperand ? PREC_END : PREC_ID;

	default:
		return PREC_END;
	}
}

/**
 * @brief Helper function, makes AST node for the current token if it is an
 * operand (identifier or literal)
 * @return Pointer to the created node, NULL if the token is not an operand
 */
astNode *exprMakeOperand(void) {
	token tok = parser.current;

	switch (tok.specType) {
	case id_idtName:
		return astNewIdentifier(tok.value.word, tok.line, tok.col);
	case lit_int:
		return astNewIntLit(tok.value.num, tok.line, tok.col);
	case lit_double:
		return astNewDoubleLit(tok.value.decNum, tok.line, tok.col);
	case lit_string:
		return astNewStringLit(tok.value.word, tok.line, tok.col);
	case lit_true:
		return astNewBoolLit(true, tok.line, tok.col);
	case lit_false:
		return astNewBoolLit(false, tok.line, tok.col);
	default:
		return NULL;
	}
}

/**
 * @brief Helper function, pushes the current token on the expression stack
 * @param prec Precedence of the current token
 * @param withHandle If a handle should be pushed before the token ('<' in the
 * table)
 *
 * Operands, interpolated strings and if expressions get their AST node here
 */
void exprShift(int prec, bool withHandle) {
	if (withHandle) {
		exprSymbol handle;
		handle.kind = EXPR_HANDLE;
		handle.node = NULL;
		handle.isArgList = false;

		if (top(&parser.exprStack)->kind == EXPR_NONTERM) {
			exprSymbol e = pop(&parser.exprStack);
			push(&parser.exprStack, handle);
			push(&parser.exprStack, e);
		} else {
			push(&parser.exprStack, handle);
		}
	}

	exprSymbol symbol;
	symbol.kind = EXPR_TERM;
	symbol.prec = prec;
	symbol.tok = parser.current;
	symbol.node = NULL;
	symbol.isArgList = false;

	if (prec == PREC_ID && parser.current.specType == kw_if) {
		symbol.node = parseIfExpr();
		push(&parser.exprStack, symbol);
		return;
	}

	if (prec == PREC_ID) {
		if (parser.current.specType == intp_begin)
			symbol.node = parseInterp();
		else
			symbol.node = parserTrackUnattached(exprMakeOperand());
	}

	push(&parser.exprStack, symbol);
	parserAdvance();
}

/**
 * @brief Helper function, checks if the precedence is a binary operator
 * @param prec Precedence of the operator
 * @return True if binary operation, false otherwise
 */
bool exprIsBinaryOp(int prec) {
	switch (prec) {
	case PREC_ADD:
	case PREC_SUB:
	case PREC_MUL:
	case PREC_DIV:
	case PREC_LT:
	case PREC_LE:
	case PREC_GT:
	case PREC_GE:
	case PREC_EQ:
	case PREC_NE:
	case PREC_AND:
	case PREC_OR:
		return true;
	default:
		return false;
	}
}

/**
 * @brief Helper function, checks that the symbol is not an argument list
 * @param symbol The expression symbol
 *
 * Argument lists "a, b" can only be inside of a function call
 */
void exprCheckNotArgList(const exprSymbol *symbol) {
	if (symbol->kind == EXPR_NONTERM && symbol->isArgList)
		error(SYNTAX_ERROR, "Unexpected ',' in expression");
}

/**
 * @brief Helper function, makes a function call node
 * @param callee Node of the called function
 * @param args Symbol with the arguments, NULL if there are none
 * @param line Line of the call
 * @param col Column of the call
 * @return Pointer to the call node
 */
astNode *exprMakeCall(astNode *callee, const exprSymbol *args, int line,
					  int col) {
	astNode *call;

	if (args != NULL && args->isArgList) {
		call = args->node;
		call->data.call.callee = callee;
	} else {
		call = parserTrackUnattached(astNewCall(callee, line, col));
		if (args != NULL)
			parserListAppend(&call->data.call.args, args->node);
	}

	parserMarkAttached(callee);
	return call;
}

/**
 * @brief Helper function, makes a member access node -> object.name
 * @param object Node before the '.'
 * @param nameNode Identifier node after the '.', gets freed
 * @param line Line of the '.'
 * @param col Column of the '.'
 * @return Pointer to the member access node
 */
astNode *exprMakeMember(astNode *object, astNode *nameNode, int line, int col) {
	if (nameNode->type != AST_IDENTIFIER)
		error(SYNTAX_ERROR, "Expected identifier after '.'");

	astNode *member = parserTrackUnattached(
		astNewMember(object, nameNode->data.identifier.name, line, col));
	parserMarkAttached(object);

	nameNode->data.identifier.name = NULL;
	parserMarkAttached(nameNode);
	astFree(nameNode);

	return member;
}

/**
 * @brief Helper function, reduces the symbols up to the handle on the
 * 		  expression stack into one nonterminal ('>' in the table)
 *
 * Handles operands, brackets, unary and binary operators,
 * argument lists, function calls and member access
 */
void exprReduce(void) {
	exprSymbol fromTop[6];
	size_t count = 0;

	while (top(&parser.exprStack)->kind != EXPR_HANDLE) {
		if (exprIsTerm(top(&parser.exprStack), PREC_END) || count == 6)
			error(SYNTAX_ERROR, "Invalid expression");
		fromTop[count++] = pop(&parser.exprStack);
	}
	pop(&parser.exprStack);

	exprSymbol *h[6];
	for (size_t i = 0; i < count; i++)
		h[i] = &fromTop[count - 1 - i];

	astNode *result = NULL;
	bool isArgList = false;

	if (count == 1 && exprIsTerm(h[0], PREC_ID)) {
		// single operand
		result = h[0]->node;
	} else if (count == 2 && exprIsTerm(h[0], PREC_LBR) &&
			   exprIsTerm(h[1], PREC_RBR)) {
		// Unit -> ()
		result =
			parserTrackUnattached(astNewUnitLit(h[0]->tok.line, h[0]->tok.col));
	} else if (count == 3 && exprIsTerm(h[0], PREC_LBR) &&
			   h[1]->kind == EXPR_NONTERM && exprIsTerm(h[2], PREC_RBR)) {
		// expression in paranthesis -> (expr)
		exprCheckNotArgList(h[1]);
		result = h[1]->node;
	} else if (count == 2 && h[0]->kind == EXPR_TERM &&
			   h[1]->kind == EXPR_NONTERM &&
			   (h[0]->prec == PREC_UPLUS || h[0]->prec == PREC_UMINUS ||
				h[0]->prec == PREC_NOT)) {
		// unary operator
		exprCheckNotArgList(h[1]);
		result = parserTrackUnattached(astNewUnary(
			h[0]->tok.specType, h[1]->node, h[0]->tok.line, h[0]->tok.col));
		parserMarkAttached(h[1]->node);
	} else if (count == 3 && h[0]->kind == EXPR_NONTERM &&
			   exprIsTerm(h[1], PREC_COMMA) && h[2]->kind == EXPR_NONTERM) {
		// argument list
		exprCheckNotArgList(h[2]);
		if (h[0]->isArgList) {
			result = h[0]->node;
		} else {
			result = parserTrackUnattached(
				astNewCall(NULL, h[0]->tok.line, h[0]->tok.col));
			parserListAppend(&result->data.call.args, h[0]->node);
		}
		parserListAppend(&result->data.call.args, h[2]->node);
		isArgList = true;
	} else if (count == 2 && h[0]->kind == EXPR_NONTERM &&
			   exprIsTerm(h[1], PREC_COMMA) && parserCheck(br_end)) {
		// trailing comma check for arguments
		if (parser.current.line == h[1]->tok.line)
			error(SYNTAX_ERROR,
				  "Trailing comma only allowed when a new line follows");
		if (h[0]->isArgList) {
			result = h[0]->node;
		} else {
			result = parserTrackUnattached(
				astNewCall(NULL, h[0]->tok.line, h[0]->tok.col));
			parserListAppend(&result->data.call.args, h[0]->node);
		}
		isArgList = true;
	} else if (count == 3 && h[0]->kind == EXPR_NONTERM &&
			   h[1]->kind == EXPR_TERM && h[2]->kind == EXPR_NONTERM &&
			   exprIsBinaryOp(h[1]->prec)) {
		// binary operation
		exprCheckNotArgList(h[0]);
		exprCheckNotArgList(h[2]);
		result = parserTrackUnattached(
			astNewBinary(h[1]->tok.specType, h[0]->node, h[2]->node,
						 h[1]->tok.line, h[1]->tok.col));
		parserMarkAttached(h[0]->node);
		parserMarkAttached(h[2]->node);
	} else if ((count == 3 || (count == 4 && h[2]->kind == EXPR_NONTERM)) &&
			   (exprIsTerm(h[0], PREC_ID) || h[0]->kind == EXPR_NONTERM) &&
			   exprIsTerm(h[1], PREC_LBR) &&
			   exprIsTerm(h[count - 1], PREC_RBR)) {
		// function calls
		exprCheckNotArgList(h[0]);
		if (exprIsTerm(h[0], PREC_ID) && h[0]->node->type != AST_IDENTIFIER)
			error(SYNTAX_ERROR, "Only a function can be called");

		result = exprMakeCall(h[0]->node, count == 4 ? h[2] : NULL,
							  h[1]->tok.line, h[1]->tok.col);
	} else if (count == 3 && h[0]->kind == EXPR_NONTERM &&
			   exprIsTerm(h[1], PREC_DOT) && h[2]->kind == EXPR_NONTERM) {
		// member access -> foo . foo
		exprCheckNotArgList(h[0]);
		exprCheckNotArgList(h[2]);
		astNode *right = h[2]->node;

		// member access after argLists
		astNode *innerCall = right;
		while (innerCall->type == AST_CALL &&
			   innerCall->data.call.callee != NULL &&
			   innerCall->data.call.callee->type == AST_CALL)
			innerCall = innerCall->data.call.callee;

		if (innerCall->type == AST_CALL &&
			innerCall->data.call.callee != NULL &&
			innerCall->data.call.callee->type == AST_IDENTIFIER) {
			astNode *callee = innerCall->data.call.callee;
			innerCall->data.call.callee = NULL;
			parserTrackUnattached(callee);

			astNode *member = exprMakeMember(h[0]->node, callee, h[1]->tok.line,
											 h[1]->tok.col);
			innerCall->data.call.callee = parserMarkAttached(member);
			result = right;
		} else {
			result = exprMakeMember(h[0]->node, right, h[1]->tok.line,
									h[1]->tok.col);
		}
	} else {
		error(SYNTAX_ERROR, "Invalid expression");
	}

	exprSymbol e;
	e.kind = EXPR_NONTERM;
	e.prec = PREC_END;
	e.node = result;
	e.isArgList = isArgList;
	e.tok.line = result->line;
	e.tok.col = result->col;
	push(&parser.exprStack, e);
}

astNode *parseExpr(void) {
	PARSER_TRACE();

	exprSymbol end;
	end.kind = EXPR_TERM;
	end.prec = PREC_END;
	end.tok = parser.current;
	end.node = NULL;
	end.isArgList = false;

	push(&parser.exprStack, end);
	int parenDepth = 0;

	while (true) {
		int incoming = exprClassify(exprAfterOperand(), parenDepth);

		int topPrec = exprTopTerminalPrec();
		if (topPrec == PREC_END && incoming == PREC_END)
			break;

		char relation = precedenceTable[topPrec][incoming];
		switch (relation) {
		case '<':
		case '=':
			if (incoming == PREC_LBR)
				parenDepth++;
			if (incoming == PREC_RBR)
				parenDepth--;
			exprShift(incoming, relation == '<');
			break;
		case '>':
			exprReduce();
			break;
		default:
			if (topPrec == PREC_LBR && incoming == PREC_END)
				error(SYNTAX_ERROR, "Expected ')' in expression");
			error(SYNTAX_ERROR, "Invalid expression");
		}
	}

	astNode *result = NULL;
	if (top(&parser.exprStack)->kind == EXPR_NONTERM) {
		exprCheckNotArgList(top(&parser.exprStack));
		result = pop(&parser.exprStack).node;
	}
	pop(&parser.exprStack);

	return result;
}

astNode *parseExprOrIf(void) {
	if (parserCheck(kw_if)) {
		return parseIfExpr();
	} else {
		return parseExpr();
	}
}

astNode *parseIfExpr(void) {
	PARSER_TRACE();

	astNode *ifNode = parseIfHead();

	ifNode->data.ifStmt.thenBody = parserMarkAttached(parseExprOrIf());
	if (ifNode->data.ifStmt.thenBody == NULL)
		error(SYNTAX_ERROR, "Expected value after 'then'");

	if (!parserMatch(kw_else))
		error(SYNTAX_ERROR, "if used as a value needs an else branch");

	ifNode->data.ifStmt.elseBody = parserMarkAttached(parseExprOrIf());
	if (ifNode->data.ifStmt.elseBody == NULL)
		error(SYNTAX_ERROR, "Expected value after 'else'");

	return ifNode;
}

astNode *parseInterp(void) {
	PARSER_TRACE();

	astNode *interpStr = parserTrackUnattached(
		astNewInterp(parser.current.line, parser.current.col));
	parserAdvance();

	while (!parserCheck(intp_end)) {
		astNode *intpPart = NULL;

		if (parserCheck(intp_part)) {
			intpPart = parserTrackUnattached(
				astNewStringLit(parser.current.value.word, parser.current.line,
								parser.current.col));
			parserAdvance();
		} else if (parserCheck(id_idtName)) {
			intpPart = parserTrackUnattached(
				astNewIdentifier(parser.current.value.word, parser.current.line,
								 parser.current.col));
			parserAdvance();
		} else if (parserCheck(intp_exprBegin)) {
			parserAdvance();

			intpPart = parseExprOrIf();

			if (intpPart == NULL)
				error(SYNTAX_ERROR, "Expected expression after '${'");
			if (!parserMatch(intp_exprEnd))
				error(SYNTAX_ERROR, "Expected '}' in interpolated string");

		} else
			error(SYNTAX_ERROR, "Wrong format of a interpolated string");

		parserListAppend(&interpStr->data.interp.parts, intpPart);
	}

	return interpStr;
}
