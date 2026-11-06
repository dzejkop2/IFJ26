#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parser.h"
#include "test/tokenDump.h"
#include "test/astPrint.h"

void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [source_file] [options]\n"
        "Options:\n"
        "  -t, --tokens     Dump tokens from lexer\n"
        "  -a, --ast        Print AST (default if no explicit flags)\n",
        prog);
}

int main(int argc, char *argv[]) {
    const int useStdin = (argc < 2);
    const char *srcPath = useStdin ? NULL : argv[1];

    bool flagTokens = false;
    bool flagAST = false;
    bool flagParse = false;

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "-t") == 0) flagTokens = true;
        else if (strcmp(argv[i], "-a") == 0) flagAST = true;
        else if (strcmp(argv[i], "-p") == 0) flagParse = true;
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (flagTokens) {
        dumpTokens();
    }

    if(flagParse) {
       astNode *root = parse();
       ast_print(root, 0);
    }

    if (flagAST) {
        
    }

    printf("%s, THIS FILE WILL BE USED IN THE FUTURE\n",srcPath);

    return 0;
}