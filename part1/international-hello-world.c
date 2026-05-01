#include <stdio.h>
#include "international-hello-world.h"

void print_hello_string(void) {
#ifdef FRENCH
    printf("Bonjour le monde!\n");
#elif defined(SPANISH)
    printf("Hola, mundo!\n");
#elif defined(CHINESE)
    printf("Nihao, shijie!\n");
#elif defined(DANISH)
    printf("Hej Verden!\n");
#else
    printf("Hello world!\n");
#endif
}

int main(void) {
    print_hello_string();
    return 0;
}