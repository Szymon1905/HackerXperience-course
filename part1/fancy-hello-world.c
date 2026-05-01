#include <stdio.h>

// ANSI Escape Codes for Colors
#define RED   "\x1B[31m"
#define GREEN   "\x1B[32m"
#define YELLOW   "\x1B[33m"
#define RESET "\x1B[0m"

/**
 * @brief Prints a stylized greeting with a border and colors.
 */
int main(void) {
    printf("%s-------SZYMON-------%s\n", YELLOW, RESET);
    printf("%s|   %s%s   %s|%s\n", GREEN, RED, "Hello World!", GREEN, RESET);
    printf("%s-------SZYMON-------%s\n", YELLOW, RESET);
    return 0;
}