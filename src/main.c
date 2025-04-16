#include <stdio.h>
#include <tam/tam.h>

int main(int argc, const char **argv) {
    TamEmulator Emulator;
    if (!loadProgram(&Emulator, argv[1])) {
        fprintf(stderr, "Failed to load program\n");
        return 1;
    }

    Instruction Instr;
    while (1) {
        if (!fetchDecode(&Emulator, &Instr)) {
            fprintf(stderr, "Failed to fetch instruction\n");
            return 2;
        }

        if (Instr.Op == HALT) {
            break;
        }

        if (!execute(&Emulator, Instr)) {
            fprintf(stderr, "Execution error\n");
            return 3;
        }
    }

    printf("Hello, tam!\n");
}