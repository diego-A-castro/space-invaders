#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>

typedef struct ConditionCode{
    uint8_t z:1;
    uint8_t c:1;
    uint8_t p:1;
    uint8_t ac:1;
    uint8_t s:1;
    uint8_t pad:3;
} ConditionCode;

typedef struct State8080{
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint16_t sp;
    uint16_t pc;
    uint8_t *memory;
    uint8_t int_enable;
    struct ConditionCode cc;
} State8080;

int Emulate8080(State8080 *state, bool printTrace);
int Disassemble8080(unsigned char *buffer, int pc);
void GenInterrupt(State8080 *state, int interrput_num);
void ReadFileIntoMemory(State8080 *state, char *filename, uint32_t offset);
State8080 *Init8080(void);

#endif