#include <stdio.h>
#include <string.h>
#include <tam/error.h>
#include <tam/tam.h>

void instructionString(Instruction Instr, char *Str) {
    switch (Instr.Op) {
    case LOAD:
        sprintf(Str, "LOAD(%d) %d[%d]", Instr.N, Instr.D, Instr.R);
        break;
    case LOADA:
        sprintf(Str, "LOADA %d[%d]", Instr.D, Instr.R);
        break;
    case LOADI:
        sprintf(Str, "LOADI (%d)", Instr.N);
        break;
    case LOADL:
        sprintf(Str, "LOADL %d", Instr.D);
        break;
    case STORE:
        sprintf(Str, "STORE(%d) %d[%d]", Instr.N, Instr.D, Instr.R);
        break;
    case STOREI:
        sprintf(Str, "STOREI(%d)", Instr.N);
        break;
    case CALL:
        sprintf(Str, "CALL(%d) %d[%d]", Instr.N, Instr.D, Instr.R);
        break;
    case CALLI:
        sprintf(Str, "CALLI");
        break;
    case RETURN:
        sprintf(Str, "RETURN(%d) %d", Instr.N, Instr.D);
        break;
    case PUSH:
        sprintf(Str, "PUSH %d", Instr.D);
        break;
    case POP:
        sprintf(Str, "POP(%d) %d", Instr.N, Instr.D);
        break;
    case JUMP:
        sprintf(Str, "JUMP %d[%d]", Instr.D, Instr.R);
        break;
    case JUMPI:
        sprintf(Str, "JUMPI");
        break;
    case JUMPIF:
        sprintf(Str, "JUMPIF(%d) %d[%d]", Instr.N, Instr.D, Instr.R);
        break;
    case HALT:
        sprintf(Str, "HALT");
        break;
    }
}

int main(int argc, const char **argv) {
    int ErrCode;
    TamEmulator *Emulator = newEmulator();

    int TraceMode =
        !(strncmp("-t", argv[1], 2) && strncmp("--trace", argv[1], 7));
    const char *Filename = TraceMode ? argv[2] : argv[1];

    if ((ErrCode = loadProgram(Emulator, Filename))) {
        fprintf(stderr, "%s\n", errorMessage(ErrCode));
        return ErrCode;
    }

    Instruction Instr;
    while (1) {
        if ((ErrCode = fetchDecode(Emulator, &Instr))) {
            fprintf(stderr, "%s at loc %04x\n", errorMessage(ErrCode),
                    Emulator->Registers[CP]);
            return ErrCode;
        }

        if (TraceMode) {
            char Buf[32];
            instructionString(Instr, Buf);
            printf("0x%04x: %s\n", Emulator->Registers[CP] - 1, Buf);
        }

        if (Instr.Op == HALT) {
            break;
        }

        if ((ErrCode = execute(Emulator, Instr))) {
            fprintf(stderr, "%s at loc %04x\n", errorMessage(ErrCode),
                    Emulator->Registers[CP] - 1);
            return ErrCode;
        }
    }

    free(Emulator);
}
