/**
 * @name ifj26
 * @authors Marek Rzeszoto (xrzeszm00)
 * @file peekErrorTest.c
 */
/*
AI generated test file, for memory leaks after syntax error
Prompt: Simulate the parser peeks a token and on the first identifier "detects"
a syntax error and calls error() -> exit() exactly like a real parser would so make
me a test for this case

RunCommand (from the project root):
gcc -std=c11 -Wall -Wextra -g -O0 \
    test/peekErrorTest.c scanner.c dynamicString.c error.c \
    -o test/peekErrorTest
valgrind --leak-check=full --show-leak-kinds=all --num-callers=30 \
    ./test/peekErrorTest < test/testEasy.ifj26
*/

#include <stdio.h>
#include <stdlib.h>
#include "../scanner.h"
#include "../error.h"

int main(void) {
    scannerInit();

    for (;;) {
        token t = peekToken();

        if (t.specType == ps_eof) {
            break;
        }

        // the "parser" sees an identifier where it did not expect one
        if (t.specType == id_idtName) {
            fprintf(stderr, "simulated syntax error at %d:%d after peeking '%s'\n",
                    t.line, t.col, t.value.word);
            error(LEXICAL_ERROR, "simulated syntax error after peekToken");
        }

        t = getToken();
        if (t.specType == id_idtName || t.specType == lit_string || t.specType == intp_part) {
            free(t.value.word);
        }
    }

    scannerFree();
    return 0;
}