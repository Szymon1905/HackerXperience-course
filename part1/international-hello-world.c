

#include <stdio.h>
#include "international-hello-world.h"

void print_international_hello(void) {
    // The pre-processor replaces GREETING with the literal string
    // defined in the header before the compiler even sees it.
    printf("%s\n", GREETING);
}

int main(void) {
    print_international_hello();
    return 0;
}

