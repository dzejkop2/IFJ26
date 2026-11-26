/**
 * @name ifj26
 * @authors Jakub Geleta (xgeletj00)
 * @file parser.h
 */

#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "scanner.h"
#include "ast.h"
#include "stack.h"

/**
 * @brief Parser struct that is created in the beginning of the syntax
 * 		  analysis cycle
 */
typedef struct {
	token current;
	astNode **unattached;
	size_t unattachedCount;
	size_t unattachedCapacity;
	Stack exprStack;
	bool active;
} parserState;

/**
 * @brief Precedence table indexes, worded for better usage
 */
typedef enum {
	PREC_ADD,
	PREC_SUB,
	PREC_MUL,
	PREC_DIV,
	PREC_LT,
	PREC_LE,
	PREC_GT,
	PREC_GE,
	PREC_EQ,
	PREC_NE,
	PREC_UPLUS,
	PREC_UMINUS,
	PREC_LBR,
	PREC_RBR,
	PREC_ID,
	PREC_NOT,
	PREC_AND,
	PREC_OR,
	PREC_COMMA,
	PREC_DOT,
	PREC_END,
	PREC_COUNT,
} precIndex;

astNode *parse(void);

void parserAdvance(void);
bool parserCheck(specTokenType specType);
bool parserMatch(specTokenType specType);

astNode *parseProgram(void);
void parseGlobalDefs(astNodeList *defs);
astNode *parseGlobalDef(void);
astNode *parseExtension(void);
void parseExtBody(astNode *extension);
astNode *parseFuncDef(bool isMain);
void parseParamLists(astNodeList *paramLists);
void parseParams(astNodeList *params);
astNode *parseParam(void);
astDataType parseTypeOpt(void);
astDataType parseType(void);

astNode *parseBody(void);
astNode *parseBlock(void);
astNode *parseStmt(void);
astNode *parseSimple(void);
astNode *parseInline(void);
astNode *parseVarDecl(bool isMutable);
astNode *parseReturn(void);
astNode *parseWhile(void);
astNode *parseValue(void);
astNode *parseIfHead(void);
astNode *parseIf(bool isValue);
astNode *parseFor(void);
void parseForHeader(astNode *forNode);
void parseGenerator(astNode *forNode);
void parseGuards(astNodeList *guards);
void parseGuardLines(astNodeList *guards);
astNode *parseInterp(void);
astNode *parseIfExpr(void);
astNode *parseExprOrIf(void);

astNode *parseExpr(void);

#endif
