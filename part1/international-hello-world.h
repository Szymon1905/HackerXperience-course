#ifndef INTERNATIONAL_HELLO_WORLD_H
#define INTERNATIONAL_HELLO_WORLD_H


#if !defined(FRENCH) && !defined(SPANISH) && !defined(CHINESE) && !defined(DANISH)
    #ifndef ENGLISH
        #define ENGLISH
    #endif
#endif

void print_hello_string(void);

#endif