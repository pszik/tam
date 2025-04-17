#include <stdio.h>
#include <string.h>
#include <tam/error.h>
#include <tam/tam.h>

void instructionString(Instruction Instr, char *Str) {
    switch (Instr.Op) {
    case LOAD:
        snprintf(Str, 32, "LOAD(%d) %d[%d]", Instr.N, Instr.D, Instr.R);
        break;
    case LOADA:
        snprintf(Str, 32, "LOADA %d[%d]", Instr.D, Instr.R);
        break;
    case LOADI:
        snprintf(Str, 32, "LOADI (%d)", Instr.N);
        break;
    case LOADL:
        snprintf(Str, 32, "LOADL %d", Instr.D);
        break;
    case STORE:
        snprintf(Str, 32, "STORE(%d) %d[%d]", Instr.N, Instr.D, Instr.R);
        break;
    case STOREI:
        snprintf(Str, 32, "STOREI(%d)", Instr.N);
        break;
    case CALL:
        snprintf(Str, 32, "CALL(%d) %d[%d]", Instr.N, Instr.D, Instr.R);
        break;
    case CALLI:
        snprintf(Str, 32, "CALLI");
        break;
    case RETURN:
        snprintf(Str, 32, "RETURN(%d) %d", Instr.N, Instr.D);
        break;
    case PUSH:
        snprintf(Str, 32, "PUSH %d", Instr.D);
        break;
    case POP:
        snprintf(Str, 32, "POP(%d) %d", Instr.N, Instr.D);
        break;
    case JUMP:
        snprintf(Str, 32, "JUMP %d[%d]", Instr.D, Instr.R);
        break;
    case JUMPI:
        snprintf(Str, 32, "JUMPI");
        break;
    case JUMPIF:
        snprintf(Str, 32, "JUMPIF(%d) %d[%d]", Instr.N, Instr.D, Instr.R);
        break;
    case HALT:
        snprintf(Str, 32, "HALT");
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
