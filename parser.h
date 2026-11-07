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

typedef struct {
    token current;
    astNode **unattached;
    size_t unattachedCount;
    size_t unattachedCapacity;
    Stack exprStack;    // precedence analysis stack, shared by all (recursive) parseExpr() calls
    bool active;
} parserState;

astNode *parse(void);

void parserAdvance(void);
bool parserCheck(specTokenType specType);
bool parserMatch(specTokenType specType);
//token parserExpect(specTokenType specType, const char *errorMsg);

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
astNode *parseIf(void);
astNode *parseFor(void);
void parseForHeader(astNode *forNode);
void parseGenerator(astNode *forNode);
void parseGuards(astNodeList *guards);
void parseGuardLines(astNodeList *guards);

astNode *parseExpr(void);
bool isExprStart(specTokenType specType);

#endif
