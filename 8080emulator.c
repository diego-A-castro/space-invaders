#include "shell.h"

unsigned char cycles8080[] = {
	4, 10, 7, 5, 5, 5, 7, 4, 4, 10, 7, 5, 5, 5, 7, 4, //0x00..0x0f
	4, 10, 7, 5, 5, 5, 7, 4, 4, 10, 7, 5, 5, 5, 7, 4, //0x10..0x1f
	4, 10, 16, 5, 5, 5, 7, 4, 4, 10, 16, 5, 5, 5, 7, 4,
	4, 10, 13, 5, 10, 10, 10, 4, 4, 10, 13, 5, 5, 5, 7, 4,

	5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5, //0x40..0x4f
	5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
	5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
	7, 7, 7, 7, 7, 7, 7, 7, 5, 5, 5, 5, 5, 5, 7, 5,

	4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4, //0x80..8x4f
	4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
	4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
	4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,

	11, 10, 10, 10, 17, 11, 7, 11, 11, 10, 10, 10, 10, 17, 7, 11, //0xc0..0xcf
	11, 10, 10, 10, 17, 11, 7, 11, 11, 10, 10, 10, 10, 17, 7, 11,
	11, 10, 10, 18, 17, 11, 7, 11, 11, 5, 10, 5, 17, 17, 7, 11,
	11, 10, 10, 4, 17, 11, 7, 11, 11, 5, 10, 4, 17, 17, 7, 11,
};

int parity(int x, int size){
	int parity = 0;

	int i;
	for (i = 0; i < size; i++)
	{
		parity += x & 1;
		x = x >> 1;
	}
	return (parity % 2 == 0);
}

void falgsWithoutCarry(State8080 *state, uint16_t answer){
    state->cc.z = ((answer & 0x00) == 0);
    state->cc.s = ((answer & 0x80) != 0);
    state->cc.p = parity(answer, 8);
    state->cc.ac = (answer > 0x09);
}

void allFlags(State8080 *state, uint16_t answer){
    state->cc.c = (answer > 0xff); //checks if result is greater than 0xff
	state->cc.z = ((answer & 0xff) == 0);
	state->cc.s = (0x80 == (answer & 0x80)); //res & 1000 0000
	state->cc.p = parity(answer, 8);
	state->cc.ac = (answer > 0x09);
}

void ANA(State8080 *state, uint8_t reg){
    uint16_t result = (uint16_t)state->a & (uint16_t)reg;
    allFlags(state, result);
    state->cc.c = 0;
    state->a = result;
}

void CALL(State8080 *state, unsigned char *opcode){
	uint16_t ret = state->pc + 2;
	state->memory[state->sp - 1] = (ret >> 8) & 0xff;
	state->memory[state->sp - 2] = (ret & 0xff);
	state->sp = state->sp - 2;
	state->pc = (opcode[2] << 8) | opcode[1];
}

void CMP(State8080 *state, uint8_t reg){
    uint16_t result = state->a - reg;
    state->cc.z = (result == 0);
    state->cc.s = ((result & 0x80) != 0);
    state->cc.p = parity(result, 8);
    state->cc.c = (state->a < result);
}

void LXI(uint8_t *firstPair, uint8_t *secondPair, unsigned char *opcode){
    *firstPair = opcode[2];
    *secondPair = opcode[1];
}

void ORA(State8080 *state, uint8_t reg){
    uint16_t result = (uint16_t)state->a | (uint16_t)reg;
    allFlags(state, result);
    state->cc.c = 0;
    state->a = result;
}

void POP(State8080 *state, uint8_t *reg1, uint8_t *reg2){
    *reg1 = state->memory[state->sp];
    *reg2 = state->memory[state->sp + 1];

    state->sp += 2;
}

void POP_PSW(State8080 *state){
    uint8_t PSW = state->memory[state->sp];

	// carry flag (CY) <- ((SP))_0
	state->cc.c = ((PSW & 0x1) != 0);

	// parity flag (P) <- ((SP))_2
	state->cc.p = ((PSW & 0x4) != 0);

	// auxiliary flag (AC) <- ((SP))_4
	state->cc.ac = ((PSW & 0x10) != 0);

	// zero flag (Z) <- ((SP))_6
	state->cc.z = ((PSW & 0x40) != 0);

	// sign flag (S) <- ((SP))_7
	state->cc.s = ((PSW & 0x80) != 0);

	state->a = state->memory[state->sp + 1];
	state->sp += 2;
}

void PUSH(State8080 *state, uint8_t reg1, uint8_t reg2){
    state->memory[state->sp - 2] = reg2;
    state->memory[state->sp - 1] = reg1;

    state->sp -= 2;
}

void PUSH_PSW(State8080 *state){
    state->memory[state->sp - 2] = (state->cc.c & 0x01) | 	//0th position
								   (0x02) |				   	//1st
								   (state->cc.c << 2) |   	//2nd
								   (state->cc.ac << 4) |   	//4th
								   (state->cc.z << 6) |		//6th
								   (state->cc.s << 7) |		//7th
								   (0x00);				   	//0 in other positions

	state->memory[state->sp - 1] = state->a;
	state->sp -= 2;
}

void XRA(State8080 *state, uint8_t reg){
    uint16_t result = (uint16_t)state->a ^ (uint16_t)reg;
    falgsWithoutCarry(state, result);
    state->cc.c = 0;
    state->a = result;
}

void RET(State8080 *state){
    state->pc = (state->memory[state->sp + 1]) << 8 | (state->memory[state->sp]);
    state->sp += 2;
}

void unimplementedInstruction(State8080* state){
    state->pc = state->pc - 1;
    printf("Error: unimplemented instruction");
    exit(1);
}

int Emulate8080(State8080 *state, bool printTrace){
    unsigned char *opcode = &state->memory[state->pc];
    Disassemble8080(state->memory, state->pc);
    state->pc += 1;
    uint16_t address = (state->h << 8) | (state->l);

    switch(*opcode){
        case 0x00:{
            break;
        }
        case 0x01:{
            LXI(&state->b, &state->c, opcode);
            break;
            state->pc += 2;
        }
        case 0x02:{
            uint16_t bc = (state->b << 8) | (state->c);
            state->memory[bc] = state->a;
            break;
        }
        case 0x03:{
            state->c += 1;

            //check for overflow
            if(state->c == 0){
                state->b += 1;
            }
            break;
        }
        case 0x04:{
            uint16_t answer = state->b + 1;
            falgsWithoutCarry(state, answer);
            state->b = answer & 0xff;
            break;
        }
        case 0x05:{
            uint16_t answer = state->b - 1;
            falgsWithoutCarry(state, answer);
            state->b = (answer & 0xff);
            break;
        }
        case 0x06:{
            state->b = opcode[1];
            state->pc += 1;
            break;
        }
        case 0x07:{
            uint8_t temp = state->a;
            state->a = (temp << 1) | (temp >> 7);
            state->cc.c = ((temp >> 7) > 0);
            break;
        }
        case 0x08:{
            break;
        }
        case 0x09:{
            uint32_t bc = (state->b << 8) | (state->c);
            uint32_t hl = (state->h << 8) | (state->l);
            uint32_t result = hl + bc;

            state->cc.c = ((result & 0xffff0000) > 0);

            state->h = (result & 0x00ff) >> 8;
            state->l = (result & 0xff);
            break;
        }
        case 0x0a:{
            uint16_t bc = (state->b << 8) | (state->c);
            state->a = state->memory[bc];
            break;
        }
        case 0x0b:{
            state->c -= 1;

            if(state->c == 0xff){
                state->b--;
            }
            break;
        }
        case 0x0c:{
            uint16_t answer = state->b + 1;
            allFlags(state, answer);
            break;
        }
        case 0x0d:{
            uint16_t answer = state->c - 1;
            allFlags(state, answer);
            break;
        }
        case 0x0e:{
            state->c = opcode[1];
            state->pc += 1;
            break;
        }
        case 0x0f:{
            uint8_t temp = state->a;
            state->a = (temp >> 1) | (temp << 7);
            state->cc.c = ((temp >> 7) > 0);
            break;
        }
        case 0x10:{
            break;
        }
        case 0x11:{
            LXI(&state->d, &state->e, opcode);
            state->pc += 2;
            break;
        }
        case 0x12:{
            uint16_t de = (state->d) | (state->e);
            state->memory[de] = state->a;
            break;
        }
        case 0x13:{
            state->e += 1;

            if(state->e == 0){
                state->d += 1;
            }
            break;
        }
        case 0x14:{
            uint16_t answer = state->b + 1;
            falgsWithoutCarry(state, answer);
            state->d = answer & 0xff;
            break;
        }
        case 0x15:{
            uint16_t answer = state->d - 1;
            falgsWithoutCarry(state, answer);
            state->d = answer;
            break;
        }
        case 0x16:{
            state->d = opcode[1];
            state->pc += 1;
            break;
        }
        case 0x17:{
            uint8_t temp = state->a;
            state->a = (temp << 1) | state->cc.c;
            state->cc.c = (temp >> 7);
            break;
        }
        case 0x18:{
            break;
        }
        case 0x19:{
            uint32_t de = (state->d << 8) | (state->e);
            uint32_t hl = (state->h << 8) | (state->l);
            uint32_t result = hl + de;

            state->cc.c = ((result & 0xffff0000) > 0);

            state->h = (result & 0x00ff) >> 8;
            state->l = (result & 0xff);
            break;
        }
        case 0x1a:{
            uint16_t de = (state->d << 8) | (state->e);
            state->a = state->memory[de];
            break;
        }
        case 0x1b:{
            state->e--;

            if(state->e == 0xff){
                state->d--;
            }
            break;
        }
        case 0x1c:{
            uint16_t answer = state->b + 1;
            falgsWithoutCarry(state, answer);
            state->e = answer & 0xff;
            break;
        }
        case 0x1d:{
            uint16_t answer = state->d - 1;
            falgsWithoutCarry(state, answer);
            state->d = answer;
            break;
        }
        case 0x1e:{
            state->e = opcode[1];
            state->pc += 1;
            break;
        }
        case 0x1f:{
            uint8_t temp = state->a;
            state->a = (state->a >> 1) | ((temp >> 7) << 7);
            state->cc.c = ((temp << 7) >> 7);
            break;
        }
        case 0x20:{ 
            unimplementedInstruction(state); 
            break;}
        case 0x21:{
            LXI(&state->h, &state->l, opcode);
            state->pc += 2;
            break;
        }
        case 0x22:{
            uint16_t SHLDaddress = (opcode[2] << 8) | opcode[1];
            state->memory[SHLDaddress] = state->l;
            state->memory[SHLDaddress + 1] = state->h;
            state->pc += 2;
            break;
        }
        case 0x23:{
            state->l += 1;

            if(state->l == 0){
                state->h += 1;
            }
            break;
        }
        case 0x24:{
            uint16_t answer = state->b + 1;
            falgsWithoutCarry(state, answer);
            state->h = answer & 0xff;
            break;
        }
        case 0x25:{
            uint16_t answer = state->h - 1;
            falgsWithoutCarry(state, answer);
            state->h = answer;
            break;
        }
        case 0x26:{
            state->h = opcode[1];
            state->pc += 1;
            break;
        }
        case 0x27:{
            unsigned char ac = state->a;
            unsigned char lower_nibble = (ac & 0x0f);
            uint16_t high_nibble = (ac & 0xf0);
            //checks if lower_nibble is greater than 9
            if(lower_nibble > 9){
                lower_nibble += 6;
                state->cc.ac = (lower_nibble > 15);
            }
            //adds 1 to high_nibble if the ac=1
            high_nibble += (state->cc.ac == 1);
            if(high_nibble > 0x90){
                high_nibble += 0x60;
            }
            state->cc.c = (high_nibble > 0xff);
            state->a = (lower_nibble & 0x0f);
            state->a = (high_nibble & 0xff);
            break;
        }
        case 0x28:{
            break;
        }
        case 0x29:{
            uint32_t hl = (state->h << 8) | (state->l);
            //(hl << 1);

            state->cc.c = ((hl & 0xffff0000) > 0);

            state->h = (hl & 0xff00) >> 8;
            state->l = (hl & 0xff);
            break;
            //check this might need to be ORed with 0x00000000
        }
        case 0x2a:{
            uint16_t LHLDaddress = (opcode[2] << 8) | opcode[1];
            state->l = state->memory[LHLDaddress];
            state->h = state->memory[LHLDaddress + 1];
            state->pc += 2;
            break;
        }
        case 0x2b:{
            state->l--;

            if(state->l == 0xff){
                state->h--;
            }
            state->pc++;
            break;
        }
        case 0x2c:{
            uint16_t answer = state->b + 1;
            falgsWithoutCarry(state, answer);
            state->l = answer & 0xff;
            break;
        }
        case 0x2d:{
            uint16_t answer = state->l - 1;
            falgsWithoutCarry(state, answer);
            state->l = answer;
            break;
        }
        case 0x2e:{
            state->l = opcode[1];
            state->pc += 1;
            break;
        }
        case 0x2f:{
            state->a = ~(state->a);
            break;
        }
        case 0x30:{
            unimplementedInstruction(state); 
            break;
        }
        case 0x31:{
            state->sp = (opcode[2] << 8) | opcode[1];
            state->pc += 2;
            break;
        }
        case 0x32:{
            uint16_t STAaddress = (opcode[2] << 8) | opcode[1];
            state->memory[STAaddress] = state->a;
            state->pc += 2;
            break;
        }
        case 0x33:{
            state->sp += 1;
        }
        case 0x34:{
            uint16_t answer = state->b + 1;
            falgsWithoutCarry(state, answer);
            state->memory[address] = answer & 0xff;
            break;
        }
        case 0x35:{
            uint16_t answer = state->memory[address] - 1;
            falgsWithoutCarry(state, answer);
            state->memory[address] = answer;
            break;
        }
        case 0x36:{
            state->memory[address] = opcode[1];
            state->pc += 1;
            break;
        }
        case 0x37:{
            state->cc.c = 1;
            break;
        }
        case 0x38:{
            break;
        }
        case 0x39:{
            uint32_t hl = (state->h << 8) | (state->l);
            uint32_t result = hl + state->sp;

            state->cc.c = ((result & 0xffff0000) > 0);

            state->h = ((result & 0x00ff) >> 8);
            state->l = (result & 0xff);
            break;
        }
        case 0x3a:{
            uint16_t LDAaddress = (opcode[2] << 8) | opcode[1];
            state->a = state->memory[LDAaddress];
            state->pc += 2;
            break;
        }
        case 0x3b:{
            state->sp--;
            break;
        }
        case 0x3c:{
            uint16_t answer = state->b + 1;
            falgsWithoutCarry(state, answer);
            state->a = answer & 0xff;
            break;
        }
        case 0x3d:{
            uint16_t answer = state->a - 1;
            falgsWithoutCarry(state, answer);
            state->a = answer;
            break;
        }
        case 0x3e:{
            state->a = opcode[1];
            state->pc += 1;
            break;
        }
        case 0x3f:{
            if(state->cc.c == 0){
                state->cc.c = 1;
            }
            else{
                state->cc.c = 0;
            }
            break;
        }
        case 0x40:{
            state->b = state->b;
            break;
        }
        case 0x41:{
            state->b = state->c;
            break;
        }
        case 0x42:{
            state->b = state->d;
            break;
        }
        case 0x43:{
            state->b = state->e;
            break;
        }
        case 0x44:{
            state->b = state->h;
            break;
        }
        case 0x45:{
            state->b = state->l;
            break;
        }
        case 0x46:{
            state->b = state->memory[address];
            break;
        }
        case 0x47:{
            state->b = state->a;
            break;
        }
        case 0x48:{
            state->c = state->b;
            break;
        }
        case 0x49:{
            state->c = state->c;
            break;
        }
        case 0x4a:{
            state->c = state->d;
            break;
        }
        case 0x4b:{
            state->c = state->e;
            break;
        }
        case 0x4c:{
            state->c = state->h;
            break;
        }
        case 0x4d:{
            state->c = state->l;
            break;
        }
        case 0x4e:{
            state->c = state->memory[address];
            break;
        }
        case 0x4f:{
            state->c = state->a;
            break;
        }
        case 0x50:{
            state->d = state->b;
            break;
        }
        case 0x51:{
            state->d = state->c;
            break;
        }
        case 0x52:{
            state->d = state->d;
            break;
        }
        case 0x53:{
            state->d = state->e;
            break;
        }
        case 0x54:{
            state->d = state->h;
            break;
        }
        case 0x55:{
            state->d = state->l;
            break;
        }
        case 0x56:{
            state->d = state->memory[address];
            break;
        }
        case 0x57:{
            state->d = state->a;
            break;
        }
        case 0x58:{
            state->e = state->b;
            break;
        }
        case 0x59:{
            state->e = state->c;
            break;
        }
        case 0x5a:{
            state->e = state->d;
            break;
        }
        case 0x5b:{
            state->e = state->e;
        }
        case 0x5c:{
            state->e = state->h;
            break;
        }
        case 0x5d:{
            state->e = state->l;
            break;
        }
        case 0x5e:{
            state->e = state->memory[address];
            break;
        }
        case 0x5f:{
            state->e = state->a;
            break;
        }
        case 0x60:{
            state->h = state->b;
            break;
        }
        case 0x61:{
            state->h = state->c;
            break;
        }
        case 0x62:{
            state->h = state->d;
            break;
        }
        case 0x63:{
            state->h = state->e;
            break;
        }
        case 0x64:{
            state->h = state->h;
            break;
        }
        case 0x65:{
            state->h = state->l;
            break;
        }
        case 0x66:{
            state->h = state->memory[address];
            break;
        }
        case 0x67:{
            state->h = state->a;
            break;
        }
        case 0x68:{
            state->l = state->b;
            break;
        }
        case 0x69:{
            state->l = state->c;
            break;
        }
        case 0x6a:{
            state->l = state->d;
            break;
        }
        case 0x6b:{
            state->l = state->e;
            break;
        }
        case 0x6c:{
            state->l = state->h;
            break;
        }
        case 0x6d:{
            state->l = state->l;
            break;
        }
        case 0x6e:{
            state->l = state->memory[address];
            break;
        }
        case 0x6f:{
            state->l = state->a;
            break;
        }
        case 0x70:{
            state->memory[address] = state->b;
            break;
        }
        case 0x71:{
            state->memory[address] = state->c;
            break;
        }
        case 0x72:{
            state->memory[address] = state->d;
            break;
        }
        case 0x73:{
            state->memory[address] = state->e;
            break;
        }
        case 0x74:{
            state->memory[address] = state->h;
            break;
        }
        case 0x75:{
            state->memory[address] = state->l;
            break;
        }
        case 0x76:{
            exit(0);
            break;
        }
        case 0x77:{
            state->memory[address] = state->a;
            break;
        }
        case 0x78:{
            state->a = state->b;
            break;
        }
        case 0x79:{
            state->a = state->c;
            break;
        }
        case 0x7a:{
            state->a = state->d;
            break;
        }
        case 0x7b:{
            state->a = state->e;
            break;
        }
        case 0x7c:{
            state->a = state->h;
            break;
        }
        case 0x7d:{
            state->a = state->l;
            break;
        }
        case 0x7e:{
            state->a = state->memory[address];
            break;
        }
        case 0x7f:{
            state->a = state->a;
            break;
        }
        case 0x80:{
            //start of ADD instructions
            uint16_t answer = (uint16_t)state->a + (uint16_t)state->b;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            break;
        }
        case 0x81:{
            uint16_t answer = (uint16_t)state->a + (uint16_t)state->c;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            break;
        }
        case 0x82:{
            uint16_t answer = (uint16_t)state->a + (uint16_t)state->d;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            break;
        }
        case 0x83:{
            uint16_t answer = (uint16_t)state->a + (uint16_t)state->e;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            break;
        }
        case 0x84:{
            uint16_t answer = (uint16_t)state->a + (uint16_t)state->h;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            break;
        }
        case 0x85:{
            uint16_t answer = (uint16_t)state->a + (uint16_t)state->l;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            break;
        }
        case 0x86:{
            uint16_t answer = (uint16_t)state->a + (uint16_t)state->memory[address];
            allFlags(state, answer);
            state->a = (answer & 0xff);
            break;
        }
        case 0x87:{
            uint16_t answer = (uint16_t)state->a * 2;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            break;
            //end of ADD intructions
        }
        case 0x88:{
            //start of ADC instructions
            uint16_t answer = (uint16_t)state->a + (uint16_t)state->b + (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            break;
        }
        case 0x89:{
            uint16_t answer = (uint16_t)state->a + (uint16_t)state->c + (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            break;
        }
        case 0x8a:{
            uint16_t answer = (uint16_t)state->a + (uint16_t)state->d + (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            break;
        }
        case 0x8b:{
            uint16_t answer = (uint16_t)state->a + (uint16_t)state->e + (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            break;
        }
        case 0x8c:{
            uint16_t answer = (uint16_t)state->a +(uint16_t) state->h + (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            break;
        }
        case 0x8d:{
            uint16_t answer = (uint16_t)state->a + (uint16_t)state->l + (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            break;
        }
        case 0x8e:{
            uint16_t answer = (uint16_t)state->a + (uint16_t)state->memory[address] + (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            break;
        }
        case 0x8f:{
            uint16_t answer = ((uint16_t)state->a * 2) + (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            break;
            //end of ADC insructions
        }
        case 0x90:{
            //start of SUB instructions
            uint16_t answer = (uint16_t)state->a - (uint16_t)state->b;
            allFlags(state, answer);
            state->a = answer & 0xff;
            break;
        }
        case 0x91:{
            uint16_t answer = (uint16_t)state->a - (uint16_t)state->c;
            allFlags(state, answer);
            state->a = answer & 0xff;
            break;
        }
        case 0x92:{
            uint16_t answer = (uint16_t)state->a - (uint16_t)state->d;
            allFlags(state, answer);
            state->a = answer & 0xff;
            break;
        }
        case 0x93:{
            uint16_t answer = (uint16_t)state->a - (uint16_t)state->e;
            allFlags(state, answer);
            state->a = answer & 0xff;
            break;
        }
        case 0x94:{
            uint16_t answer = (uint16_t)state->a - (uint16_t)state->h;
            allFlags(state, answer);
            state->a = answer & 0xff;
            break;
        }
        case 0x95:{
            uint16_t answer = (uint16_t)state->a - (uint16_t)state->l;
            allFlags(state, answer);
            state->a = answer & 0xff;
            break;
        }
        case 0x96:{
            uint16_t answer = (uint16_t)state->a - (uint16_t)state->memory[address];
            allFlags(state, answer);
            state->a = answer & 0xff;
            break;
        }
        case 0x97:{
            uint16_t answer = (uint16_t)state->a - (uint16_t)state->a;
            allFlags(state, answer);
            state->a = answer & 0xff;
            break;
            //end of SUB instructions
        }
        case 0x98:{
            //start of SBB instructions
            uint16_t answer = (uint16_t)state->a - (uint16_t)state->b - (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = answer & 0xff;
            break;
        }
        case 0x99:{
            uint16_t answer = (uint16_t)state->a - (uint16_t)state->c - (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = answer & 0xff;
            break;
        }
        case 0x9a:{
            uint16_t answer = (uint16_t)state->a - (uint16_t)state->d - (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = answer & 0xff;
            break;
        }
        case 0x9b:{
            uint16_t answer = (uint16_t)state->a - (uint16_t)state->e - (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = answer & 0xff;
            break;
        }
        case 0x9c:{
            uint16_t answer = (uint16_t)state->a - (uint16_t)state->h - (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = answer & 0xff;
            break;
        }
        case 0x9d:{
            uint16_t answer = (uint16_t)state->a - (uint16_t)state->l - (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = answer & 0xff;
            break;
        }
        case 0x9e:{
            uint16_t answer = (uint16_t)state->a - (uint16_t)state->memory[address] - (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = answer & 0xff;
            break;
        }
        case 0x9f:{
            uint16_t answer = (uint16_t)state->a - (uint16_t)state->a - (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = answer & 0xff;
            break;
            //end of SBB instructions
        }
        case 0xa0:{
            ANA(state, state->b);
            break;
        }
        case 0xa1:{
            ANA(state, state->c);
            break;
        }
        case 0xa2:{
            ANA(state, state->d);
            break;
        }
        case 0xa3:{
            ANA(state, state->e);
            break;
        }
        case 0xa4:{
            ANA(state, state->h);
            break;
        }
        case 0xa5:{
            ANA(state, state->l);
            break;
        }
        case 0xa6:{
            ANA(state, state->memory[address]);
            break;
        }
        case 0xa7:{
            ANA(state, state->a);
            break;
        }
        case 0xa8:{
            XRA(state, state->b);
            break;
        }
        case 0xa9:{
            XRA(state, state->c);
            break;
        }
        case 0xaa:{
            XRA(state, state->d);
            break;
        }
        case 0xab:{
            XRA(state, state->e);
            break;
        }
        case 0xac:{
            XRA(state, state->h);
            break;
        }
        case 0xad:{
            XRA(state, state->l);
            break;
        }
        case 0xae:{
            XRA(state, state->memory[address]);
            break;
        }
        case 0xaf:{
            XRA(state, state->a);
            break;
        }
        case 0xb0:{
            ORA(state, state->b);
            break;
        }
        case 0xb1:{
            ORA(state, state->c);
            break;
        }
        case 0xb2:{
            ORA(state, state->d);
            break;
        }
        case 0xb3:{
            ORA(state, state->e);
            break;
        }
        case 0xb4:{
            ORA(state, state->h);
            break;
        }
        case 0xb5:{
            ORA(state, state->l);
            break;
        }
        case 0xb6:{
            ORA(state, state->memory[address]);
            break;
        }
        case 0xb7:{
            ORA(state, state->a);
            break;
        }
        case 0xb8:{
            CMP(state, state->b);
            break;
        }
        case 0xb9:{
            CMP(state, state->c);
            break;
        }
        case 0xba:{
            CMP(state, state->d);
            break;
        }
        case 0xbb:{
            CMP(state, state->e);
            break;
        }
        case 0xbc:{
            CMP(state, state->h);
            break;
        }
        case 0xbd:{
            CMP(state, state->l);
            break;
        }
        case 0xbe:{
            CMP(state, state->memory[address]);
            break;
        }
        case 0xbf:{
            CMP(state, state->a);
            break;
        }
        case 0xc0:{
            if(state->cc.z == 0){
                RET(state);
            }
            break;
        }
        case 0xc1:{
            POP(state, &state->b, &state->c);
            break;
        }
        case 0xc2:{
            if(state->cc.z == 0){
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            else{
                state->pc += 2;
            }
            break;
        }
        case 0xc3:{
            state->pc = (opcode[2] << 8) | opcode[1];
            break;
        }
        case 0xc4:{
            if(state->cc.z == 1){
                uint16_t ret = state->pc + 2;
                state->memory[state->sp - 1] = (ret >> 8) & 0xff;
                state->memory[state->sp - 2] = (ret & 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            else{
                state->pc += 2;
            }
            break;
        }
        case 0xc5:{
            PUSH(state, state->b, state->c);
            break;
        }
        case 0xc6:{
            uint16_t answer = (uint16_t)state->a + (uint16_t)opcode[1];
            allFlags(state, answer);
            state->a = (answer & 0xff);
            state->pc += 1;
            break;
        }
        case 0xc7:{
            unsigned char tmp[3];
            tmp[1] = 0;
            tmp[2] = 0;
            CALL(state, tmp);
            break;
        }
        case 0xc8:{
            if(state->cc.z == 1){
                RET(state);
            }
            break;
        }
        case 0xc9:{
            RET(state);
            break;
        }
        case 0xca:{
            if(state->cc.z == 1){
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            else{
                state->pc += 2;
            }
            break;
        }
        case 0xcb:{
            break;
        }
        case 0xcc:{
            if(state->cc.z == 0){
                uint16_t ret = state->pc + 2;
                state->memory[state->sp - 1] = (ret >> 8) & 0xff;
                state->memory[state->sp - 2] = (ret & 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            else{
                state->pc += 2;
            }
            break;
        }
        case 0xcd:{
            uint16_t ret = state->pc + 2;
            state->memory[state->sp - 1] = (ret >> 8) & 0xff;
            state->memory[state->sp - 2] = (ret & 0xff);
            state->sp = state->sp - 2;
            state->pc = (opcode[2] << 8) | opcode[1];
            break;
        }
        case 0xce:{
            uint16_t answer = (uint16_t)state->a + (uint16_t)opcode[1] + (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            state->pc += 1;
            break;
        }
        case 0xcf:{
            unsigned char tmp[3];
            tmp[1] = 8;
            tmp[2] = 0;
            CALL(state, tmp);
            break;
        }
        case 0xd0:{
            if(state->cc.c == 0){
                RET(state);
            }
            break;
        }
        case 0xd1:{
            POP(state, &state->d, &state->e);
            break;
        }
        case 0xd2:{
            if(state->cc.c == 0){
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            else{
                state->pc += 2;
            }
            break;
        }
        case 0xd3:{
            break;
        }
        case 0xd4:{
            if(state->cc.c == 1){
                uint16_t ret = state->pc + 2;
                state->memory[state->sp - 1] = (ret >> 8) & 0xff;
                state->memory[state->sp - 2] = (ret & 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            else{
                state->pc += 2;
            }
            break;
        }
        case 0xd5:{
            PUSH(state, state->d, state->e);
            break;
        }
        case 0xd6:{
            uint16_t answer = (uint16_t)state->a - (uint16_t)opcode[1];
            allFlags(state, answer);
            state->a = (answer & 0xff);
            state->pc += 1;
            break;
        }
        case 0xd7:{
            unsigned char tmp[3];
            tmp[1] = 16;
            tmp[2] = 0;
            CALL(state, tmp);
            break;
        }
        case 0xd8:{
            if(state->cc.c == 1){
                RET(state);
            }
            break;
        }
        case 0xd9:{
            break;
        }
        case 0xda:{
            if(state->cc.c == 1){
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            else{
                state->pc += 2;
            }
            break;
        }
        case 0xdb:{
            break;
        }
        case 0xdc:{
            if(state->cc.c == 1){
                uint16_t ret = state->pc + 2;
                state->memory[state->sp - 1] = (ret >> 8) & 0xff;
                state->memory[state->sp - 2] = (ret & 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            else{
                state->pc += 2;
            }
            break;
        }
        case 0xdd:{
            break;
        }
        case 0xde:{
            uint16_t answer = (uint16_t)state->a - (uint16_t)opcode[1] - (uint16_t)state->cc.c;
            allFlags(state, answer);
            state->a = (answer & 0xff);
            state->pc += 1;
            break;
        }
        case 0xdf:{
            unsigned char tmp[3];
            tmp[1] = 24;
            tmp[2] = 0;
            CALL(state, tmp);
            break;
        }
        case 0xe0:{
            if(state->cc.p == 0){
                RET(state);
            }
            break;
        }
        case 0xe1:{
            POP(state, &state->h, &state->l);
            break;
        }
        case 0xe2:{
            if(state->cc.p == 0){
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            else{
                state->pc += 2;
            }
            break;
        }
        case 0xe3:{
            state->l = state->memory[state->sp];
            state->h = state->memory[state->sp + 1];
            break;
        }
        case 0xe4:{
            if(state->cc.p == 0){
                uint16_t ret = state->pc + 2;
                state->memory[state->sp - 1] = (ret >> 8) & 0xff;
                state->memory[state->sp - 2] = (ret & 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            else{
                state->pc += 2;
            }
            break;
        }
        case 0xe5:{
            PUSH(state, state->h, state->l);
            break;
        }
        case 0xe6:{
            ANA(state, opcode[1]);
            state->pc += 1;
            break;
        }
        case 0xe7:{
            unsigned char tmp[3];
            tmp[1] = 32;
            tmp[2] = 0;
            CALL(state, tmp);
            break;
        }
        case 0xe8:{
            if(state->cc.p == 1){
                RET(state);
            }
            break;
        }
        case 0xe9:{
            state->pc = address;
            break;
        }
        case 0xea:{
            if(state->cc.p == 1){
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            else{
                state->pc += 2;
            }
            break;
        }
        case 0xeb:{
            uint8_t tmpH = state->h;
            uint8_t tmpL = state->l;

            state->h = state->d;
            state->l = state->e;

            state->d = tmpH;
            state->e = tmpL;
            break;
        }
        case 0xec:{
            if(state->cc.p == 1){
                uint16_t ret = state->pc + 2;
                state->memory[state->sp - 1] = (ret >> 8) & 0xff;
                state->memory[state->sp - 2] = (ret & 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            else{
                state->pc += 2;
            }
            break;
        }
        case 0xed:{
            break;
        }
        case 0xee:{
            XRA(state, opcode[1]);
            state->pc += 1;
            break;
        }
        case 0xef:{
            unsigned char tmp[3];
            tmp[1] = 40;
            tmp[2] = 0;
            CALL(state, tmp);
            break;
        }
        case 0xf0:{
            if(state->cc.s == 0){
                RET(state);
            }
            break;
        }
        case 0xf1:{
            POP_PSW(state);
            break;
        }
        case 0xf2:{
            if(state->cc.s == 0){
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            else{
                state->pc += 2;
            }
            break;
        }
        case 0xf3:{
            state->int_enable = 0;
            break;
        }
        case 0xf4:{
            if(state->cc.s == 0){
                uint16_t ret = state->pc + 2;
                state->memory[state->sp - 1] = (ret >> 8) & 0xff;
                state->memory[state->sp - 2] = (ret & 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            else{
                state->pc += 2;
            }
            break;
        }
        case 0xf5:{
            PUSH_PSW(state);
            break;
        }
        case 0xf6:{
            ORA(state, opcode[1]);
            state->pc += 1;
            break;
        }
        case 0xf7:{
            unsigned char tmp[3];
            tmp[1] = 48;
            tmp[2] = 0;
            CALL(state, tmp);
            break;
        }
        case 0xf8:{
            if(state->cc.s == 1){
                RET(state);
            }
            break;
        }
        case 0xf9:{
            state->sp = address;
            break;
        }
        case 0xfa:{
            if(state->cc.s == 1){
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            else{
                state->pc += 2;
            }
            break;
        }
        case 0xfb:{
            state->int_enable = 1;
            break;
        }
        case 0xfc:{
            if(state->cc.s == 1){
                uint16_t ret = state->pc + 2;
                state->memory[state->sp - 1] = (ret >> 8) & 0xff;
                state->memory[state->sp - 2] = (ret & 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            else{
                state->pc += 2;
            }
            break;
        }
        case 0xfd:{
            break;
        }
        case 0xfe:{
            CMP(state, opcode[1]);
            state->pc += 1;
            break;
        }
        case 0xff:{
            unsigned char tmp[3];
            tmp[1] = 56;
            tmp[2] = 0;
            CALL(state, tmp);
            break;
        }
        default:{
            unimplementedInstruction(state);
            break;
        }
    }
}

int Disassemble8080(unsigned char *buffer, int pc){
    unsigned char *opcode = &buffer[pc];
	int opbytes = 1;
	printf("0x%04x  ", pc);

	switch (*opcode)
	{
		case 0x00:
		case 0x08:
		case 0x10:
		case 0x18:
		case 0x28:
		case 0x38:
		case 0xcb:
		case 0xd9:
		case 0xdd:
		case 0xed:
		case 0xfd:
			printf("NOP");
			break;
		case 0x01:
			printf("LXI\tB, #$0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0x02:
			printf("STAX\tB");
			break;
		case 0x03:
			printf("INX\tB");
			break;
		case 0x04:
			printf("INR\tB");
			break;
		case 0x05:
			printf("DCR\tB");
			break;
		case 0x06:
			printf("MVI\tB, #$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0x07:
			printf("RLC");
			break;
		case 0x09:
			printf("DAD\tB");
			break;
		case 0x0a:
			printf("LDAX\tB");
			break;
		case 0x0b:
			printf("DCX\tB");
			break;
		case 0x0c:
			printf("INR\tC");
			break;
		case 0x0d:
			printf("DCR\tC");
			break;
		case 0x0e:
			printf("MVI\tC, #$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0x0f:
			printf("RRC");
			break;
		case 0x11:
			printf("LXI\tD, #$0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0x12:
			printf("STAX\tD");
			break;
		case 0x13:
			printf("INX\tD");
			break;
		case 0x14:
			printf("INR\tD");
			break;
		case 0x15:
			printf("DCR\tD");
			break;
		case 0x16:
			printf("DCR\tD");
			break;
		case 0x17:
			printf("RAL");
			break;
		case 0x19:
			printf("DAD\tD");
			break;
		case 0x1a:
			printf("LDAX\tD");
			break;
		case 0x1b:
			printf("DCX\tD");
			break;
		case 0x1c:
			printf("INR\tE");
			break;
		case 0x1d:
			printf("DCR\tE");
			break;
		case 0x1e:
			printf("MVI\tE, #$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0x1f:
			printf("RAR");
			break;
		case 0x20:
			printf("RIM");
			break;
		case 0x21:
			printf("LXI\tH, #$0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0x22:
			printf("SHLD\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0x23:
			printf("INX\tH");
			break;
		case 0x24:
			printf("INR\tH");
			break;
		case 0x25:
			printf("DCR\tH");
			break;
		case 0x26:
			printf("MVI\tH, #$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0x27:
			printf("DAA");
			break;
		case 0x29:
			printf("DAD\tH");
			break;
		case 0x2a:
			printf("LHLD\t 0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0x2b:
			printf("DCX\tH");
			break;
		case 0x2c:
			printf("INR\tL");
			break;
		case 0x2d:
			printf("DCR\tL");
			break;
		case 0x2e:
			printf("MVI\tL, #$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0x2f:
			printf("CMA");
			break;
		case 0x30:
			printf("SIM");
			break;
		case 0x31:
			printf("LXI\tSP, #$0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0x32:
			printf("STA\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0x33:
			printf("INX\tSP");
			break;
		case 0x34:
			printf("INR\tM");
			break;
		case 0x35:
			printf("DCR\tM");
			break;
		case 0x36:
			printf("MVI\tM, #$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0x37:
			printf("STC");
			break;
		case 0x39:
			printf("DAD\tSP");
			break;
		case 0x3a:
			printf("LDA\t0x%02x%02X", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0x3b:
			printf("DCX\tSP");
			break;
		case 0x3c:
			printf("INR\tA");
			break;
		case 0x3d:
			printf("DCR\tA");
			break;
		case 0x3e:
			printf("MVI\tA, #$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0x3f:
			printf("CMC");
			break;
		case 0x40:
			printf("MOV\tB, B");
			break;
		case 0x41:
			printf("MOV\tB, C");
			break;
		case 0x42:
			printf("MOV\tB, D");
			break;
		case 0x43:
			printf("MOV\tB, E");
			break;
		case 0x44:
			printf("MOV\tB, H");
			break;
		case 0x45:
			printf("MOV\tB, L");
			break;
		case 0x46:
			printf("MOV\tB, M");
			break;
		case 0x47:
			printf("MOV\tB, A");
			break;
		case 0x48:
			printf("MOV\tC, B");
			break;
		case 0x49:
			printf("MOV\tC, C");
			break;
		case 0x4a:
			printf("MOV\tC, D");
			break;
		case 0x4b:
			printf("MOV\tC, E");
			break;
		case 0x4c:
			printf("MOV\tC, H");
			break;
		case 0x4d:
			printf("MOV\tC, L");
			break;
		case 0x4e:
			printf("MOV\tC, M");
			break;
		case 0x4f:
			printf("MOV\tC, A");
			break;
		case 0x50:
			printf("MOV\tD, B");
			break;
		case 0x51:
			printf("MOV\tD, C");
			break;
		case 0x52:
			printf("MOV\tD, D");
			break;
		case 0x53:
			printf("MOV\tD, E");
			break;
		case 0x54:
			printf("MOV\tD, H");
			break;
		case 0x55:
			printf("MOV\tD, L");
			break;
		case 0x56:
			printf("MOV\tD, M");
			break;
		case 0x57:
			printf("MOV\tD, A");
			break;
		case 0x58:
			printf("MOV\tE, B");
			break;
		case 0x59:
			printf("MOV\tE, C");
			break;
		case 0x5a:
			printf("MOV\tE, D");
			break;
		case 0x5b:
			printf("MOV\tE, E");
			break;
		case 0x5c:
			printf("MOV\tE, H");
			break;
		case 0x5d:
			printf("MOV\tE, L");
			break;
		case 0x5e:
			printf("MOV\tE, M");
			break;
		case 0x5f:
			printf("MOV\tE, A");
			break;
		case 0x60:
			printf("MOV\tH, B");
			break;
		case 0x61:
			printf("MOV\tH, C");
			break;
		case 0x62:
			printf("MOV\tH, D");
			break;
		case 0x63:
			printf("MOV\tH, E");
			break;
		case 0x64:
			printf("MOV\tH, H");
			break;
		case 0x65:
			printf("MOV\tH, L");
			break;
		case 0x66:
			printf("MOV\tH, M");
			break;
		case 0x67:
			printf("MOV\tH, A");
			break;
		case 0x68:
			printf("MOV\tL, B");
			break;
		case 0x69:
			printf("MOV\tL, C");
			break;
		case 0x6a:
			printf("MOV\tL, D");
			break;
		case 0x6b:
			printf("MOV\tL, E");
			break;
		case 0x6c:
			printf("MOV\tL, H");
			break;
		case 0x6d:
			printf("MOV\tL, L");
			break;
		case 0x6e:
			printf("MOV\tL, M");
			break;
		case 0x6f:
			printf("MOV\tL, A");
			break;
		case 0x70:
			printf("MOV\tM, B");
			break;
		case 0x71:
			printf("MOV\tM, C");
			break;
		case 0x72:
			printf("MOV\tM, D");
			break;
		case 0x73:
			printf("MOV\tM, E");
			break;
		case 0x74:
			printf("MOV\tM, H");
			break;
		case 0x75:
			printf("MOV\tM, L");
			break;
		case 0x76:
			printf("HLT");
			break;
		case 0x77:
			printf("MOV\tM, A");
			break;
		case 0x78:
			printf("MOV\tA, B");
			break;
		case 0x79:
			printf("MOV\tA, C");
			break;
		case 0x7a:
			printf("MOV\tA, D");
			break;
		case 0x7b:
			printf("MOV\tA, E");
			break;
		case 0x7c:
			printf("MOV\tA, H");
			break;
		case 0x7d:
			printf("MOV\tA, L");
			break;
		case 0x7e:
			printf("MOV\tA, M");
			break;
		case 0x7f:
			printf("MOV\tA, A");
			break;
		case 0x80:
			printf("ADD\tB");
			break;
		case 0x81:
			printf("ADD\tC");
			break;
		case 0x82:
			printf("ADD\tD");
			break;
		case 0x83:
			printf("ADD\tE");
			break;
		case 0x84:
			printf("ADD\tH");
			break;
		case 0x85:
			printf("ADD\tL");
			break;
		case 0x86:
			printf("ADD\tM");
			break;
		case 0x87:
			printf("ADD\tA");
			break;
		case 0x88:
			printf("ADC\tB");
			break;
		case 0x89:
			printf("ADC\tC");
			break;
		case 0x8a:
			printf("ADC\tE");
			break;
		case 0x8b:
			printf("ADC\tE");
			break;
		case 0x8c:
			printf("ADC\tH");
			break;
		case 0x8d:
			printf("ADC\tL");
			break;
		case 0x8e:
			printf("ADC\tM");
			break;
		case 0x8f:
			printf("ADC\tA");
			break;
		case 0x90:
			printf("SUB\tB");
			break;
		case 0x91:
			printf("SUB\tC");
			break;
		case 0x92:
			printf("SUB\tD");
			break;
		case 0x93:
			printf("SUB\tE");
			break;
		case 0x94:
			printf("SUB\tH");
			break;
		case 0x95:
			printf("SUB\tL");
			break;
		case 0x96:
			printf("SUB\tM");
			break;
		case 0x97:
			printf("SUB\tA");
			break;
		case 0x98:
			printf("SBB\tB");
			break;
		case 0x99:
			printf("SBB\tC");
			break;
		case 0x9a:
			printf("SBB\tD");
			break;
		case 0x9b:
			printf("SBB\tE");
			break;
		case 0x9c:
			printf("SBB\tH");
			break;
		case 0x9d:
			printf("SBB\tL");
			break;
		case 0x9e:
			printf("SBB\tM");
			break;
		case 0x9f:
			printf("SBB\tA");
			break;
		case 0xa0:
			printf("ANA\tB");
			break;
		case 0xa1:
			printf("ANA\tC");
			break;
		case 0xa2:
			printf("ANA\tD");
			break;
		case 0xa3:
			printf("ANA\tE");
			break;
		case 0xa4:
			printf("ANA\tH");
			break;
		case 0xa5:
			printf("ANA\tL");
			break;
		case 0xa6:
			printf("ANA\tM");
			break;
		case 0xa7:
			printf("ANA\tA");
			break;
		case 0xa8:
			printf("XRA\tB");
			break;
		case 0xa9:
			printf("XRA\tC");
			break;
		case 0xaa:
			printf("XRA\tD");
			break;
		case 0xab:
			printf("XRA\tE");
			break;
		case 0xac:
			printf("XRA\tH");
			break;
		case 0xad:
			printf("XRA\tL");
			break;
		case 0xae:
			printf("XRA\tM");
			break;
		case 0xaf:
			printf("XRA\tA");
			break;
		case 0xb0:
			printf("ORA\tB");
			break;
		case 0xb1:
			printf("ORA\tC");
			break;
		case 0xb2:
			printf("ORA\tD");
			break;
		case 0xb3:
			printf("ORA\tE");
			break;
		case 0xb4:
			printf("ORA\tH");
			break;
		case 0xb5:
			printf("ORA\tL");
			break;
		case 0xb6:
			printf("ORA\tM");
			break;
		case 0xb7:
			printf("ORA\tA");
			break;
		case 0xb8:
			printf("CMP\tB");
			break;
		case 0xb9:
			printf("CMP\tC");
			break;
		case 0xba:
			printf("CMP\tD");
			break;
		case 0xbb:
			printf("CMP\tE");
			break;
		case 0xbc:
			printf("CMP\tH");
			break;
		case 0xbd:
			printf("CMP\tL");
			break;
		case 0xbe:
			printf("CMP\tM");
			break;
		case 0xbf:
			printf("CMP\tA");
			break;
		case 0xc0:
			printf("RNZ");
			break;
		case 0xc1:
			printf("POP\tB");
			break;
		case 0xc2:
			printf("JNZ\t0x%02x%02X", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xc3:
			printf("JMP\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xc4:
			printf("CNZ\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xc5:
			printf("PUSH\tB");
			break;
		case 0xc6:
			printf("ADI\t#$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0xc7:
			printf("RST\t0");
			break;
		case 0xc8:
			printf("RZ");
			break;
		case 0xc9:
			printf("RET");
			break;
		case 0xca:
			printf("JZ\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xcc:
			printf("CZ\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xcd:
			printf("CALL\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xce:
			printf("ACI\t#$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0xcf:
			printf("RST\t1");
			break;
		case 0xd0:
			printf("RNC");
			break;
		case 0xd1:
			printf("POP\tD");
			break;
		case 0xd2:
			printf("JNC\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xd3:
			printf("OUT\t#$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0xd4:
			printf("CNC\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xd5:
			printf("PUSH\tD");
			break;
		case 0xd6:
			printf("SUI\t#$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0xd7:
			printf("RST\t2");
			break;
		case 0xd8:
			printf("RC");
			break;
		case 0xda:
			printf("JC\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xdb:
			printf("IN\t#$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0xdc:
			printf("CC\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xde:
			printf("SBI\t#$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0xdf:
			printf("RST\t3");
			break;
		case 0xe0:
			printf("RPO");
			break;
		case 0xe1:
			printf("POP\tH");
			break;
		case 0xe2:
			printf("JPO\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xe3:
			printf("XTHL");
			break;
		case 0xe4:
			printf("CPO\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xe5:
			printf("PUSH\tH");
			break;
		case 0xe6:
			printf("ANI\t#$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0xe7:
			printf("RST\t4");
			break;
		case 0xe8:
			printf("RPE");
			break;
		case 0xe9:
			printf("PCHL");
			break;
		case 0xea:
			printf("JPE\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xeb:
			printf("XCHG");
			break;
		case 0xec:
			printf("CPE\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xee:
			printf("XRI\t#$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0xef:
			printf("RST\t5");
			break;
		case 0xf0:
			printf("RP");
			break;
		case 0xf1:
			printf("POP\tPSW");
			break;
		case 0xf2:
			printf("JP\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xf3:
			printf("DI");
			break;
		case 0xf4:
			printf("CP\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xf5:
			printf("PUSH\tPSW");
			break;
		case 0xf6:
			printf("ORD\t#$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0xf7:
			printf("RST\t6");
			break;
		case 0xf8:
			printf("RM");
			break;
		case 0xf9:
			printf("SPHL");
			break;
		case 0xfa:
			printf("JM\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xfb:
			printf("EI");
			break;
		case 0xfc:
			printf("CM\t0x%02x%02x", opcode[2], opcode[1]);
			opbytes = 3;
			break;
		case 0xfe:
			printf("CPI\t#$0x%02x", opcode[1]);
			opbytes = 2;
			break;
		case 0xff:
			printf("RST\t7");
			break;
		default:
			printf("Unknown Instruction: 0x%02x", *opcode);
			break;
	}

	printf("\n");
	return opbytes;
}

void GenInterrupt(State8080 *state, int interrupt_num)
{
	//Push PC
	PUSH(state, ((state->pc & 0xff00) >> 8), (state->pc & 0xff));

	//Set PC to low memory vector
	//RST interrupt_num
	state->pc = 8 * interrupt_num;
	state->int_enable = 0;
}

void ReadFileIntoMemory(State8080 *state, char *filename, uint32_t offset)
{
	FILE *f = fopen(filename, "rb");

	if (f == NULL)
	{
		printf("error opening file: %s\n", filename);
		exit(1);
	}

	fseek(f, 0L, SEEK_END);
	int fsize = ftell(f);
	fseek(f, 0L, SEEK_SET);

	//fread reads data from the given stream into the array pointed to, by ptr (buffer).
	uint8_t *buffer = &state->memory[offset];
	fread(buffer, fsize, 1, f);
	fclose(f);
}

State8080 *Init8080(void)
{
	State8080 *state = calloc(1, sizeof(State8080));
	state->memory = malloc(0x10000); //allocate 16K
	return state;
}