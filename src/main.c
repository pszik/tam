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
        if ((ErrCode = fetchDecode(&Emulator, &Instr))) {
            fprintf(stderr, "failed to fetch instruction (%d)\n", ErrCode);
            return ErrCode;
        }

        if (Instr.Op == HALT) {
            break;
        }

        if ((ErrCode = execute(&Emulator, Instr))) {
            fprintf(stderr, "execution error (%d)\n", ErrCode);
            return ErrCode;
        }
    }
}
