/**
 * @name ifj26
 * @authors Tomáš Zajac (xzajact00)
 * @file codegen.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "builtin.h"
#include "error.h"

static void generateFunction(astNode *node, astNode *receiver);
static void generateExtension(astNode *node);
static void generateNode(astNode *node);
static void generateDeclarations(astNode *node);
static const char *getBuiltinExtensionLabel(astDataType receiverType,
											const char *name);

static size_t labelCounter;
static astDataType returnType;

static astNode *programRoot = NULL;

static int isZeroListFunction(astNode *node) {
	if (programRoot == NULL || node == NULL || node->type != AST_IDENTIFIER)
		return 0;

	astNodeList defs = programRoot->data.program.defs;

	for (size_t i = 0; i < defs.count; i++) {
		astNode *def = defs.items[i];

		if (def == NULL || def->type != AST_FUN_DEF)
			continue;

		if (strcmp(def->data.funcDef.name, node->data.identifier.name) == 0 &&
			def->scopeId == node->scopeId &&
			def->data.funcDef.paramLists.count == 0) {
			return 1;
		}
	}

	return 0;
}

static void generateVariableName(const char *name, int scopeId) {
	printf("LF@%s$%d", name, scopeId);
}

static const char *builtinNames[] = {
	"print",	 "readLine", "readInt", "readDouble", "toStr",		"toInt",
	"strLength", "subStr",	 "ord",		"chr",		  "readBoolean"};

#include <string.h>

static const char *getBuiltinExtensionLabel(astDataType receiverType,
											const char *name) {
	if (strcmp(name, "toString") == 0) {
		return "$$toStr";
	}

	if (receiverType == TYPE_DOUBLE) {
		if (strcmp(name, "toInt") == 0)
			return "$$toInt";

		if (strcmp(name, "floor") == 0)
			return "$$floor";
	}

	if (receiverType == TYPE_STRING && strcmp(name, "length") == 0) {
		return "$$strLength";
	}

	return NULL;
}

static void generateDeclarationList(astNodeList list) {
	for (size_t i = 0; i < list.count; ++i)
		generateDeclarations(list.items[i]);
}

static void generateDeclarations(astNode *node) {
	if (node == NULL)
		return;
	switch (node->type) {
	case AST_VAR_DECL:
		printf("DEFVAR LF@%s$%d\n", node->data.varDecl.name, node->scopeId);
		generateDeclarations(node->data.varDecl.value);
		break;
	case AST_ASSIGN:
		generateDeclarations(node->data.assign.value);
		break;
	case AST_RETURN:
		generateDeclarations(node->data.ret.value);
		break;
	case AST_BLOCK:
		generateDeclarationList(node->data.block.stmts);
		break;
	case AST_IF:
		generateDeclarations(node->data.ifStmt.cond);
		generateDeclarations(node->data.ifStmt.thenBody);
		generateDeclarations(node->data.ifStmt.elseBody);
		break;
	case AST_WHILE:
		generateDeclarations(node->data.whileLoop.cond);
		generateDeclarations(node->data.whileLoop.body);
		break;
	case AST_FOR:
		printf("DEFVAR LF@%s$%d\n", node->data.forLoop.iterName, node->scopeId);
		printf("DEFVAR LF@%%forTo$%d\n", node->scopeId);
		printf("DEFVAR LF@%%forStep$%d\n", node->scopeId);
		generateDeclarations(node->data.forLoop.from);
		generateDeclarations(node->data.forLoop.to);
		generateDeclarations(node->data.forLoop.step);
		generateDeclarationList(node->data.forLoop.guards);
		generateDeclarations(node->data.forLoop.body);
		break;
	case AST_EXPR_STMT:
		generateDeclarations(node->data.exprStmt.expr);
		break;
	case AST_BINARY_OP:
		generateDeclarations(node->data.binary.left);
		generateDeclarations(node->data.binary.right);
		break;
	case AST_UNARY_OP:
		generateDeclarations(node->data.unary.operand);
		break;
	case AST_CALL:
		generateDeclarations(node->data.call.callee);
		generateDeclarationList(node->data.call.args);
		break;
	case AST_MEMBER:
		generateDeclarations(node->data.member.object);
		break;
	case AST_INTERP:
		generateDeclarationList(node->data.interp.parts);
		break;
	default:
		break;
	}
}

static void generateExtension(astNode *node) {
	if (node == NULL || node->type != AST_EXTENSION) {
		return;
	}

	astNode *receiver = node->data.extension.receiver;
	astNodeList functions = node->data.extension.funDefs;

	for (size_t i = 0; i < functions.count; i++) {
		generateFunction(functions.items[i], receiver);
	}
}

static void generateFunction(astNode *node, astNode *receiver) {
	if (node == NULL || node->type != AST_FUN_DEF) {
		return;
	}

	if (receiver != NULL) {
		printf("LABEL fun$ext$%d$%s\n", (int)receiver->data.param.type,
			   node->data.funcDef.name);
	} else {
		printf("LABEL fun$%s\n", node->data.funcDef.name);
	}
	printf("CREATEFRAME\n");
	printf("PUSHFRAME\n\n");

	astNodeList paramLists = node->data.funcDef.paramLists;

	for (size_t i = paramLists.count; i > 0; i--) {

		astNode *list = paramLists.items[i - 1];
		astNodeList params = list->data.paramList.params;

		for (size_t j = params.count; j > 0; j--) {

			astNode *param = params.items[j - 1];

			printf("DEFVAR LF@%s$%d\n", param->data.param.name, param->scopeId);

			printf("POPS LF@%s$%d\n", param->data.param.name, param->scopeId);
		}
	}
	if (receiver != NULL) {
		printf("DEFVAR LF@%s$%d\n", receiver->data.param.name, node->scopeId);

		printf("POPS LF@%s$%d\n", receiver->data.param.name, node->scopeId);
	}

	printf("DEFVAR LF@%%discard\nDEFVAR LF@%%left\nDEFVAR LF@%%right\n");
	generateDeclarations(node->data.funcDef.body);
	returnType = node->data.funcDef.retType;
	generateNode(node->data.funcDef.body);
	if (node->data.funcDef.retType == TYPE_UNIT) {
		printf("POPS LF@%%discard\nPUSHS nil@nil\n");
	} else if (node->data.funcDef.retType == TYPE_DOUBLE &&
			   node->data.funcDef.body->dataType == TYPE_INT) {
		printf("INT2FLOATS\n");
	}

	printf("\nPOPFRAME\n");
	printf("RETURN\n\n");
}

static void generateCallArguments(astNode *node) {
	if (node == NULL || node->type != AST_CALL)
		return;

	if (node->data.call.callee != NULL &&
		node->data.call.callee->type == AST_CALL) {
		generateCallArguments(node->data.call.callee);
	}

	for (size_t i = 0; i < node->data.call.args.count; i++) {
		astNode *arg = node->data.call.args.items[i];
		generateNode(arg);
		if (arg->intToDouble)
			printf("INT2FLOATS\n");
	}
}

static void generateNode(astNode *node) {
	if (node == NULL) {
		printf("PUSHS nil@nil\n");
		return;
	}
	switch (node->type) {
	case AST_BLOCK: {
		astNodeList stmts = node->data.block.stmts;
		for (size_t i = 0; i < stmts.count; ++i) {
			generateNode(stmts.items[i]);
			if (i + 1 < stmts.count)
				printf("POPS LF@%%discard\n");
		}
		if (stmts.count == 0)
			printf("PUSHS nil@nil\n");
		break;
	}
	case AST_LIT_INT:
		printf("PUSHS int@%lld\n", node->data.intValue);
		break;
	case AST_LIT_DOUBLE:
		printf("PUSHS float@%a\n", node->data.doubleValue);
		break;
	case AST_LIT_BOOL:
		printf("PUSHS bool@%s\n", node->data.boolValue ? "true" : "false");
		break;
	case AST_LIT_UNIT:
		printf("PUSHS nil@nil\n");
		break;
	case AST_LIT_STRING:
		printf("PUSHS string@");
		for (const unsigned char *c =
				 (const unsigned char *)node->data.stringValue;
			 *c; ++c) {
			if (*c <= 32 || *c == '#' || *c == 92)
				printf("%c%03u", 92, (unsigned)*c);
			else
				putchar(*c);
		}
		putchar('\n');
		break;

	case AST_IDENTIFIER:
		if (isZeroListFunction(node)) {
			printf("CALL fun$%s\n", node->data.identifier.name);
		} else {
			/* fprintf(stderr, "[CODEGEN] %s -> scope %d\n",
			node->data.identifier.name,
			node->scopeId); */
			printf("PUSHS ");
			generateVariableName(node->data.identifier.name, node->scopeId);
			putchar('\n');
		}

		break;
	case AST_VAR_DECL:
		generateNode(node->data.varDecl.value);
		if (node->data.varDecl.declType == TYPE_DOUBLE &&
			node->data.varDecl.value &&
			node->data.varDecl.value->dataType == TYPE_INT)
			printf("INT2FLOATS\n");
		printf("POPS LF@%s$%d\nPUSHS nil@nil\n", node->data.varDecl.name,
			   node->scopeId);
		break;
	case AST_ASSIGN: {
		astNode *target = node->data.assign.target;
		generateNode(node->data.assign.value);
		if (target->dataType == TYPE_DOUBLE &&
			node->data.assign.value->dataType == TYPE_INT)
			printf("INT2FLOATS\n");
		printf("POPS LF@%s$%d\nPUSHS nil@nil\n", target->data.identifier.name,
			   target->scopeId);
		break;
	}
	case AST_EXPR_STMT:
		generateNode(node->data.exprStmt.expr);
		break;
	case AST_RETURN: {
		generateNode(node->data.ret.value);

		if (returnType == TYPE_UNIT) {
			printf("POPS LF@%%discard\n");
			printf("PUSHS nil@nil\n");
		} else if (returnType == TYPE_DOUBLE && node->data.ret.value != NULL &&
				   node->data.ret.value->dataType == TYPE_INT) {
			printf("INT2FLOATS\n");
		}

		printf("POPFRAME\nRETURN\n");
		break;
	}
	case AST_IF: {
		size_t label = labelCounter++;
		generateNode(node->data.ifStmt.cond);
		printf("PUSHS bool@false\nJUMPIFEQS $$else$%zu\n", label);
		generateNode(node->data.ifStmt.thenBody);
		if (node->dataType == TYPE_DOUBLE &&
			node->data.ifStmt.thenBody->dataType == TYPE_INT)
			printf("INT2FLOATS\n");
		printf("JUMP $$endif$%zu\nLABEL $$else$%zu\n", label, label);
		generateNode(node->data.ifStmt.elseBody);
		if (node->dataType == TYPE_DOUBLE && node->data.ifStmt.elseBody &&
			node->data.ifStmt.elseBody->dataType == TYPE_INT)
			printf("INT2FLOATS\n");
		printf("LABEL $$endif$%zu\n", label);
		if (node->dataType == TYPE_UNIT)
			printf("POPS LF@%%discard\nPUSHS nil@nil\n");
		break;
	}
	case AST_WHILE: {
		size_t label = labelCounter++;
		printf("LABEL $$while$%zu\n", label);
		generateNode(node->data.whileLoop.cond);
		printf("PUSHS bool@false\nJUMPIFEQS $$endwhile$%zu\n", label);
		generateNode(node->data.whileLoop.body);
		printf("POPS LF@%%discard\nJUMP $$while$%zu\nLABEL "
			   "$$endwhile$%zu\nPUSHS nil@nil\n",
			   label, label);
		break;
	}

	case AST_FOR: {
		size_t label = labelCounter++;

		int scopeId = node->scopeId;
		const char *iterName = node->data.forLoop.iterName;

		generateNode(node->data.forLoop.from);

		printf("POPS LF@%s$%d\n", iterName, scopeId);

		generateNode(node->data.forLoop.to);

		printf("POPS LF@%%forTo$%d\n", scopeId);

		if (node->data.forLoop.step != NULL) {
			generateNode(node->data.forLoop.step);
		} else {
			printf("PUSHS int@1\n");
		}

		printf("POPS LF@%%forStep$%d\n", scopeId);

		if (node->data.forLoop.step != NULL) {
			printf("JUMPIFNEQ $$forStepOk$%zu LF@%%forStep$%d int@0\n", label,
				   scopeId);
			printf("EXIT int@40\n");
			printf("LABEL $$forStepOk$%zu\n", label);
		}

		printf("LABEL $$for$%zu\n", label);
		printf("PUSHS LF@%%forStep$%d\n", scopeId);
		printf("PUSHS int@0\n");
		printf("GTS\n");
		printf("PUSHS bool@true\n");
		printf("JUMPIFEQS $$forPositive$%zu\n", label);

		printf("PUSHS LF@%s$%d\n", iterName, scopeId);
		printf("PUSHS LF@%%forTo$%d\n", scopeId);

		if (node->data.forLoop.isInclusive) {
			printf("LTS\n");
		} else {
			printf("GTS\nNOTS\n");
		}

		printf("PUSHS bool@true\n");
		printf("JUMPIFEQS $$endfor$%zu\n", label);
		printf("JUMP $$forBody$%zu\n", label);

		printf("LABEL $$forPositive$%zu\n", label);
		printf("PUSHS LF@%s$%d\n", iterName, scopeId);
		printf("PUSHS LF@%%forTo$%d\n", scopeId);

		if (node->data.forLoop.isInclusive) {
			printf("GTS\n");
		} else {
			printf("LTS\nNOTS\n");
		}

		printf("PUSHS bool@true\n");
		printf("JUMPIFEQS $$endfor$%zu\n", label);

		printf("LABEL $$forBody$%zu\n", label);

		for (size_t i = 0; i < node->data.forLoop.guards.count; i++) {
			generateNode(node->data.forLoop.guards.items[i]);

			printf("PUSHS bool@false\n");
			printf("JUMPIFEQS $$forstep$%zu\n", label);
		}

		generateNode(node->data.forLoop.body);

		printf("POPS LF@%%discard\n");

		printf("LABEL $$forstep$%zu\n", label);

		printf("PUSHS LF@%s$%d\n", iterName, scopeId);
		printf("PUSHS LF@%%forStep$%d\n", scopeId);

		printf("ADDS\n");

		printf("POPS LF@%s$%d\n", iterName, scopeId);

		printf("JUMP $$for$%zu\n", label);

		printf("LABEL $$endfor$%zu\n", label);

		printf("PUSHS nil@nil\n");

		break;
	}
	case AST_UNARY_OP: {

		if (node->data.unary.op == op_sub)
			printf(node->dataType == TYPE_DOUBLE ? "PUSHS float@0x0p+0\n"
												 : "PUSHS int@0\n");
		generateNode(node->data.unary.operand);
		if (node->data.unary.op == op_sub)
			printf("SUBS\n");
		else if (node->data.unary.op == op_not)
			printf("NOTS\n");
		break;
	}

	case AST_BINARY_OP: {
		astNode *left = node->data.binary.left,
				*right = node->data.binary.right;
		specTokenType op = node->data.binary.op;
		generateNode(left);
		if (op == op_and || op == op_or) {
			size_t label = labelCounter++;
			const char *value = op == op_and ? "false" : "true";
			printf("PUSHS bool@%s\nJUMPIFEQS $$short$%zu\n", value, label);
			generateNode(right);
			printf("JUMP $$logic$%zu\nLABEL $$short$%zu\nPUSHS bool@%s\nLABEL "
				   "$$logic$%zu\n",
				   label, label, value, label);
			break;
		}
		if (left->dataType == TYPE_INT && right->dataType == TYPE_DOUBLE)
			printf("INT2FLOATS\n");
		generateNode(right);
		if (right->dataType == TYPE_INT && left->dataType == TYPE_DOUBLE)
			printf("INT2FLOATS\n");
		switch (op) {
		case op_add:
			if (node->dataType == TYPE_STRING)
				printf("POPS LF@%%right\nPOPS LF@%%left\nCONCAT LF@%%left "
					   "LF@%%left LF@%%right\nPUSHS LF@%%left\n");
			else
				printf("ADDS\n");
			break;
		case op_sub:
			printf("SUBS\n");
			break;
		case op_mul:
			printf("MULS\n");
			break;
		case op_div:
			printf(node->dataType == TYPE_INT ? "IDIVS\n" : "DIVS\n");
			break;
		case op_equal:
			printf("EQS\n");
			break;
		case op_notEqual:
			printf("EQS\nNOTS\n");
			break;
		case op_lowerThen:
			printf("LTS\n");
			break;
		case op_biggerThen:
			printf("GTS\n");
			break;
		case op_lowerEqual:
			printf("GTS\nNOTS\n");
			break;
		case op_biggerEqual:
			printf("LTS\nNOTS\n");
			break;
		default:
			break;
		}
		break;
	}

	case AST_CALL: {
		astNode *callee = node;

		while (callee != NULL && callee->type == AST_CALL) {
			callee = callee->data.call.callee;
		}

		if (callee == NULL) {
			error(INTERNAL_ERROR, "Missing call target");
		}

		if (callee->type == AST_MEMBER) {
			astNode *object = callee->data.member.object;
			const char *name = callee->data.member.name;

			generateNode(object);

			generateCallArguments(node);

			const char *label =
				getBuiltinExtensionLabel(object->dataType, name);

			if (label != NULL) {
				printf("CALL %s\n", label);
			} else {
				printf("CALL fun$ext$%d$%s\n", (int)object->dataType, name);
			}
		} else if (callee->type == AST_IDENTIFIER) {
			generateCallArguments(node);

			const char *name = callee->data.identifier.name;
			const char *prefix = "fun$";

			for (size_t i = 0; i < sizeof(builtinNames) / sizeof(*builtinNames);
				 i++) {

				if (strcmp(name, builtinNames[i]) == 0) {
					prefix = "$$";
					break;
				}
			}

			printf("CALL %s%s\n", prefix, name);
		} else {
			error(INTERNAL_ERROR, "Unsupported callee type");
		}

		break;
	}

	case AST_MEMBER: {
		astNode *object = node->data.member.object;

		const char *label =
			getBuiltinExtensionLabel(object->dataType, node->data.member.name);

		generateNode(object);

		if (label != NULL) {
			printf("CALL %s\n", label);
		} else {
			printf("CALL fun$ext$%d$%s\n", (int)object->dataType,
				   node->data.member.name);
		}
		break;
	}
	case AST_INTERP: {

		astNodeList parts = node->data.interp.parts;

		printf("PUSHS string@\n");

		for (size_t i = 0; i < parts.count; i++) {

			astNode *part = parts.items[i];

			generateNode(part);

			if (part->dataType != TYPE_STRING) {
				printf("CALL $$toStr\n");
			}

			printf("POPS LF@%%right\n");
			printf("POPS LF@%%left\n");

			printf("CONCAT LF@%%left LF@%%left LF@%%right\n");

			printf("PUSHS LF@%%left\n");
		}

		break;
	}

	default:
		break;
	}
}

void generateCode(astNode *root) {

	if (root == NULL || root->type != AST_PROGRAM) {
		return;
	}

	programRoot = root;

	printf(".IFJcode26\n");

	printf("JUMP $$entry\n");
	printf("# builtin functions\n\n");

	builtinGenPrint();
	builtinGenReadLine();
	builtinGenReadInt();
	builtinGenReadDouble();
	builtinGenToStr();
	builtinGenToInt();
	builtinGenStrLength();
	builtinGenSubStr();
	builtinGenOrd();
	builtinGenChr();
	builtinGenReadBoolean();
	builtinGenFloor();

	char *mainName = NULL;

	for (size_t i = 0; i < root->data.program.defs.count; i++) {

		astNode *def = root->data.program.defs.items[i];

		if (def->type == AST_FUN_DEF) {
			generateFunction(def, NULL);
			if (def->data.funcDef.isMain) {
				mainName = def->data.funcDef.name;
			}

		} else if (def->type == AST_EXTENSION) {
			generateExtension(def);
		}
	}
	if (mainName == NULL) {
		error(INTERNAL_ERROR, "Main function not found");
	}

	printf("LABEL $$entry\n");
	printf("CALL fun$%s\n", mainName);
	printf("EXIT int@0\n");
}