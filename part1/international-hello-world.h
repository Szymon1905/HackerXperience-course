
#ifndef INTERNATIONAL_HELLO_WORLD_H
#define INTERNATIONAL_HELLO_WORLD_H

//LANGUAGE SELECTION
#define ENGLISH
// #define FRENCH
// #define SPANISH
// #define LATIN


#ifdef ENGLISH
#define GREETING "Hello, World!"
#elif defined(FRENCH)
#define GREETING "Bonjour le monde !"
#elif defined(SPANISH)
    #define GREETING "¡Hola, Mundo!"
#elif defined(LATIN)
    #define GREETING "Salve, Mundi!"
#else
    #define GREETING "Hello, World!"
#endif
void print_international_hello(void);
#endif