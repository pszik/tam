#include <stdio.h>
#include <tam/error.h>
#include <tam/tam.h>

int main(int argc, const char **argv) {
    int ErrCode;
    TamEmulator Emulator;

    if ((ErrCode = loadProgram(&Emulator, argv[1]))) {
        fprintf(stderr, "%s\n", errorMessage(ErrCode));
        return ErrCode;
    }

    Instruction Instr;
    while (1) {
        if ((ErrCode = fetchDecode(&Emulator, &Instr))) {
            fprintf(stderr, "%s at loc %02x\n", errorMessage(ErrCode),
                    Emulator.Registers[CP]);
            return ErrCode;
        }

        if (Instr.Op == HALT) {
            break;
        }

        if ((ErrCode = execute(&Emulator, Instr))) {
            fprintf(stderr, "%s at loc %02x\n", errorMessage(ErrCode),
                    Emulator.Registers[CP] - 1);
            return ErrCode;
        }
    }
}
