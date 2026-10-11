/**
 * @name ifj26
 * @authors Marek Rzeszoto (xrzeszm00)
 * @file testDynstr.h
 */

/*
RunCommand: 
clang -std=c11 -Wall -Wextra -g test/testDynstr.c dynamicString.c error.c -o test/testDynstr                                                                             ─╯
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./test/testDynstr
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../dynamicString.h"

/*
AI generated vlagrind test file
Promnt: Give me test for my new reallocation logic functions (dynStr_ctor, dynStr_load, dynStr_freeBuff, dynStr_output, dynStr_dtor) using mem managment tool valgrind, this is expecting output:
1: "abcdefghijklmnopqrstuvwxyzabcdefghijklmn" (len 40, capacity 64)
2: "xy" (len 2, capacity 64)
3: "" (len 0)
*/
int main(void) {
    dynamicString str;
    dynStr_ctor(&str);

    for (int i = 0; i < 40; i++) {
        dynStr_load(&str, 'a' + (i % 26));
    }

    char *first = dynStr_output(&str);
    printf("1: \"%s\" (len %zu, capacity %zu)\n", first, strlen(first), str.capacity);

    dynStr_freeBuff(&str);
    dynStr_load(&str, 'x');
    dynStr_load(&str, 'y');

    char *second = dynStr_output(&str);
    printf("2: \"%s\" (len %zu, capacity %zu)\n", second, strlen(second), str.capacity);

    dynStr_freeBuff(&str);
    char *empty = dynStr_output(&str);
    printf("3: \"%s\" (len %zu)\n", empty, strlen(empty));

    free(first);
    free(second);
    free(empty);
    dynStr_dtor(&str);
    return 0;
}