#include "parser.h"
#include "test/astPrint.h"
#include "test/tokenDump.h"
#include "semantic.h"
#include "codegen.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void usage(const char *prog) {
	fprintf(stderr,
			"Usage: %s [options]\n"
			"Options:\n"
			"  -t, --tokens     Dump tokens from lexer\n"
			"  -a, --ast        Print AST\n"
			"  -p, --parse      Parse (default if no explicit flags)\n"
			"  -s, --semantic   Analysis semantic\n"
			"  -g, --generate   Code generation\n",

			prog);
}

int main(int argc, char *argv[]) {
	bool flagTokens = false;
	bool flagAST = false;
	bool flagParse = false;
	bool flagSemantic = false;
	bool flagGenerate = false;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-t") == 0)
			flagTokens = true;
		else if (strcmp(argv[i], "-a") == 0)
			flagAST = true;
		else if (strcmp(argv[i], "-p") == 0)
			flagParse = true;
		else if (strcmp(argv[i], "-s") == 0)
			flagSemantic = true;
		else if (strcmp(argv[i], "-g") == 0)
			flagGenerate = true;
		else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (flagTokens) {
		dumpTokens();
	}

	if (flagParse) {
		astNode *root = parse();
		ast_print(root, 0);
		ast_free(root);
	}

	if (flagAST) {
	}

	if (flagSemantic) {

		astNode *root = parse();
		semanticAnalyze(root);
		ast_free(root);
	}

	if (flagGenerate) {
		astNode *root = parse();
		semanticAnalyze(root);
		generateCode(root);
		ast_free(root);
	}

	return 0;
}
