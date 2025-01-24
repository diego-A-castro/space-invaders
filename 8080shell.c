#include "shell.h"

struct conditionCode{
    uint8_t z:1;
    uint8_t c:1;
    uint8_t p:1;
    uint8_t ac:1;
    uint8_t s:1;
    uint8_t pad:3;
};

struct state8080{
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint16_t sp;
    uint16_t pc;
    uint8_t *memeory;
    uint8_t int_enable;
    struct conditionCode cc;
};