#include <stdio.h>
#include <tam/tam.h>

int main(int argc, const char **argv) {
    int ErrCode;
    TamEmulator Emulator;

    ErrCode = loadProgram(&Emulator, argv[1]);
    if (ErrCode) {
        fprintf(stderr, "failed to load program (%d)\n", ErrCode);
        return ErrCode;
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
