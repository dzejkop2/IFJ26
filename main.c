/**
 * @name ifj26
 * @authors Jakub Geleta (xgeletj00)
 * @file main.c
 */

#include "parser.h"
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
			"  -p, --parse      Parse (default if no explicit flags)\n"
			"  -s, --semantic   Analysis semantic\n"
			"  -g, --generate   Code generation\n",

			prog);
}

int main(int argc, char *argv[]) {
	bool flagSemantic = false;
	bool flagGenerate = false;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-s") == 0)
			flagSemantic = true;
		else if (strcmp(argv[i], "-g") == 0)
			flagGenerate = true;
		else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (argc == 1)
		flagGenerate = true;

	if (flagSemantic) {

		astNode *root = parse();
		semanticAnalyze(root);
		astFree(root);
	}

	if (flagGenerate) {
		astNode *root = parse();
		semanticAnalyze(root);
		generateCode(root);
		astFree(root);
	}

	return 0;
}
