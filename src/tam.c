#include <tam/tam.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <tam/error.h>

int loadProgram(TamEmulator *Emulator, const char *Filename) {
    assert(Emulator);
    assert(Filename);

    memset(Emulator->CodeStore, 0, MEMORY_SIZE * sizeof(CODE_W));
    memset(Emulator->DataStore, 0, MEMORY_SIZE * sizeof(DATA_W));
    memset(Emulator->Registers, 0, 16 * sizeof(ADDRESS));

    FILE *File = fopen(Filename, "rb");
    if (!File) {
        return ErrFileNotFound;
    }

    // get file length
    int FileLength;
    fseek(File, 0, SEEK_END);
    FileLength = ftell(File);
    rewind(File);

    if (FileLength % 4 != 0) {
        return ErrFileLength;
    }

    if (FileLength > MEMORY_SIZE) {
        return ErrFileLength;
    }

    // read bytes
    uint8_t Buf[4];
    for (int i = 0; i < FileLength; ++i) {
        fread(Buf, 4, 1, File);
        Emulator->CodeStore[i] =
            Buf[0] << 24 | Buf[1] << 16 | Buf[2] << 8 | Buf[3];
    }
    Emulator->Registers[CT] = FileLength;

    // set registers
    Emulator->Registers[HB] = MEMORY_SIZE - 1;
    Emulator->Registers[HT] = MEMORY_SIZE - 1;
    Emulator->Registers[PB] = Emulator->Registers[CT];
    Emulator->Registers[PT] = Emulator->Registers[PB] + 29;

    return OK;
}

int fetchDecode(TamEmulator *Emulator, Instruction *Instr) {
    assert(Emulator);
    assert(Instr);

    uint16_t Idx = Emulator->Registers[CP];
    if (Idx >= Emulator->Registers[CT]) {
        return ErrCodeAccessViolation;
    }

    uint32_t Code = Emulator->CodeStore[Idx];
    *Instr = (Instruction){(Code & 0xf0000000) >> 28, (Code & 0x0f000000) >> 24,
                           (Code & 0x00ff0000) >> 16, (Code & 0x0000ffff)};
    Emulator->Registers[CP]++;
    return OK;
}

static int pushData(TamEmulator *Emulator, DATA_W Datum) {
    assert(Emulator);
    if (Emulator->Registers[ST] >= Emulator->Registers[HT]) {
        return ErrStackOverflow;
    }

    ADDRESS Addr = Emulator->Registers[ST]++;
    Emulator->DataStore[Addr] = Datum;
    return OK;
}

/// Attempt to push data onto the stack.
/// @param Emulator emulator to use
/// @param Datum value to push
#define PUSH(Emulator, Datum)                                                  \
    {                                                                          \
        TamError Err;                                                          \
        if ((Err = pushData(Emulator, Datum)))                                 \
            return Err;                                                        \
    }

static int popData(TamEmulator *Emulator, DATA_W *Datum) {
    if (!Emulator->Registers[ST]) {
        return ErrStackUnderflow;
    }
    ADDRESS Addr = --Emulator->Registers[ST];

    if (Datum == NULL) {
        return OK;
    }

    *Datum = Emulator->DataStore[Addr];
    return OK;
}

/// Attempt to pop a value from the stack.
/// @param Emulator emulator to use
/// @param Datum address of variable to store popped value
#define POP(Emulator, Datum)                                                   \
    {                                                                          \
        TamError Err;                                                          \
        if ((Err = popData(Emulator, Datum)))                                  \
            return Err;                                                        \
    }

static ADDRESS calcAddress(TamEmulator *Emulator, Instruction Instr) {
    return Emulator->Registers[Instr.R] + Instr.D;
}

static int execLoad(TamEmulator *Emulator, Instruction Instr) {
    TamError Err;
    ADDRESS BaseAddr = calcAddress(Emulator, Instr);
    for (int i = 0; i < Instr.N; ++i) {
        ADDRESS Addr = BaseAddr + i;
        if (Addr >= Emulator->Registers[ST] &&
            Addr <= Emulator->Registers[HT]) {
            return ErrDataAccessViolation;
        }

        DATA_W Data = Emulator->DataStore[Addr];
        PUSH(Emulator, Data);
    }
    return OK;
}

static int execLoada(TamEmulator *Emulator, Instruction Instr) {
    ADDRESS Addr = calcAddress(Emulator, Instr);
    return pushData(Emulator, (DATA_W)Addr);
}

static int execLoadi(TamEmulator *Emulator, Instruction Instr) {
    ADDRESS BaseAddr;
    POP(Emulator, (DATA_W *)&BaseAddr);

    for (int i = 0; i < Instr.N; ++i) {
        ADDRESS Addr = BaseAddr + i;
        if (Addr >= Emulator->Registers[ST] &&
            Addr <= Emulator->Registers[HT]) {
            return ErrDataAccessViolation;
        }

        DATA_W Data = Emulator->DataStore[Addr];
        PUSH(Emulator, Data);
    }
    return OK;
}

static int execLoadl(TamEmulator *Emulator, Instruction Instr) {
    return pushData(Emulator, Instr.D);
}

static int execStore(TamEmulator *Emulator, Instruction Instr) {
    // pop value
    DATA_W Value[Instr.N];
    for (int i = 0; i < Instr.N; ++i) {
        POP(Emulator, Value + i)
    }

    // store
    ADDRESS BaseAddr = calcAddress(Emulator, Instr);
    for (int i = 0; i < Instr.N; ++i) {
        ADDRESS Addr = BaseAddr + i;
        if (Addr >= Emulator->Registers[ST] &&
            Addr <= Emulator->Registers[HT]) {
            return ErrDataAccessViolation;
        }

        Emulator->DataStore[Addr] = Value[Instr.N - i - 1];
    }

    return OK;
}

static int execStorei(TamEmulator *Emulator, Instruction Instr) {
    // pop value
    DATA_W Value[Instr.N];
    for (int i = 0; i < Instr.N; ++i) {
        POP(Emulator, Value + i);
    }

    // store
    ADDRESS BaseAddr;
    POP(Emulator, (DATA_W *)&BaseAddr);

    for (int i = 0; i < Instr.N; ++i) {
        ADDRESS Addr = BaseAddr + i;
        if (Addr >= Emulator->Registers[ST] &&
            Addr <= Emulator->Registers[HT]) {
            return ErrDataAccessViolation;
        }

        Emulator->DataStore[Addr] = Value[Instr.N - i - 1];
    }

    return OK;
}

static int execCall(TamEmulator *Emulator, Instruction Instr) {
    ADDRESS StaticLink = Emulator->Registers[Instr.N];
    ADDRESS DynamicLink = Emulator->Registers[LB];
    ADDRESS ReturnAddress = Emulator->Registers[CP];

    ADDRESS CallAddr = calcAddress(Emulator, Instr);
    if (CallAddr >= Emulator->Registers[CT]) {
        return ErrCodeAccessViolation;
    }

    PUSH(Emulator, (DATA_W)StaticLink);
    PUSH(Emulator, (DATA_W)DynamicLink);
    PUSH(Emulator, (DATA_W)ReturnAddress);

    Emulator->Registers[LB] = Emulator->Registers[ST] - 3;
    Emulator->Registers[CP] = CallAddr;
    return OK;
}

static int execCallPrimitive(TamEmulator *Emulator, Instruction Instr) {
    TamError Err;
    DATA_W Arg1, Arg2;
    DATA_W *WArg1, *WArg2;
    char C;

    switch (Instr.D) {
    case 1: // id
        break;
    case 2: // not
        POP(Emulator, &Arg1);
        PUSH(Emulator, Arg1 ? 0 : 1);
        break;
    case 3: // and
        POP(Emulator, &Arg1);
        POP(Emulator, &Arg2);
        PUSH(Emulator, Arg1 * Arg2 ? 1 : 0);
        break;
    case 4: // or
        POP(Emulator, &Arg1);
        POP(Emulator, &Arg2);
        PUSH(Emulator, Arg1 + Arg2 ? 1 : 0);
        break;
    case 5: // succ
        POP(Emulator, &Arg1);
        PUSH(Emulator, Arg1 + 1);
        break;
    case 6: // pred
        POP(Emulator, &Arg1);
        PUSH(Emulator, Arg1 - 1);
        break;
    case 7: // neg
        POP(Emulator, &Arg1);
        PUSH(Emulator, -Arg1);
        break;
    case 8: // add
        POP(Emulator, &Arg1);
        POP(Emulator, &Arg2);
        PUSH(Emulator, Arg1 + Arg2);
        break;
    case 9: // sub
        POP(Emulator, &Arg1);
        POP(Emulator, &Arg2);
        PUSH(Emulator, Arg1 - Arg2);
        break;
    case 10: // mult
        POP(Emulator, &Arg1);
        POP(Emulator, &Arg2);
        PUSH(Emulator, Arg1 * Arg2);
        break;
    case 11: // div
        POP(Emulator, &Arg1);
        POP(Emulator, &Arg2);
        PUSH(Emulator, Arg1 / Arg2);
        break;
    case 12: // mod
        POP(Emulator, &Arg1);
        POP(Emulator, &Arg2);
        PUSH(Emulator, Arg1 % Arg2);
        break;
    case 13: // lt
        POP(Emulator, &Arg1);
        POP(Emulator, &Arg2);
        PUSH(Emulator, Arg1 < Arg2 ? 1 : 0);
        break;
    case 14: // le
        POP(Emulator, &Arg1);
        POP(Emulator, &Arg2);
        PUSH(Emulator, Arg1 <= Arg2 ? 1 : 0);
        break;
    case 15: // ge
        POP(Emulator, &Arg1);
        POP(Emulator, &Arg2);
        PUSH(Emulator, Arg1 >= Arg2 ? 1 : 0);
        break;
    case 16: // gt
        POP(Emulator, &Arg1);
        POP(Emulator, &Arg2);
        PUSH(Emulator, Arg1 >= Arg2 ? 1 : 0);
        break;
    case 17: // eq
        POP(Emulator, &Arg1);

        WArg1 = (DATA_W *)malloc(sizeof(DATA_W) * Arg1);
        for (int I = 0; I < Arg1; ++I) {
            POP(Emulator, WArg1 + I);
        }

        WArg2 = (DATA_W *)malloc(sizeof(DATA_W) * Arg1);
        for (int I = 0; I < Arg1; ++I) {
            POP(Emulator, WArg2 + I);
        }

        Arg2 = strncmp((char *)WArg1, (char *)WArg2, Arg1 * sizeof(DATA_W)) ? 0
                                                                            : 1;
        PUSH(Emulator, Arg2);
        free(WArg1);
        free(WArg2);
        break;
    case 18: // neq
        POP(Emulator, &Arg1);

        WArg1 = (DATA_W *)malloc(sizeof(DATA_W) * Arg1);
        for (int I = 0; I < Arg1; ++I) {
            POP(Emulator, WArg1 + I);
        }

        WArg2 = (DATA_W *)malloc(sizeof(DATA_W) * Arg1);
        for (int I = 0; I < Arg1; ++I) {
            POP(Emulator, WArg2 + I);
        }

        Arg2 = strncmp((char *)WArg1, (char *)WArg2, Arg1 * sizeof(DATA_W)) ? 1
                                                                            : 0;
        PUSH(Emulator, Arg2);
        free(WArg1);
        free(WArg2);
        break;
    case 19: // eol
        C = getc(stdin);
        PUSH(Emulator, C == '\n' ? 1 : 0);
        putc(C, stdin);
        break;
    case 20: // eof
        C = getc(stdin);
        PUSH(Emulator, C == EOF ? 1 : 0);
        putc(C, stdin);
        break;
    case 21: // get
        POP(Emulator, &Arg1);
        if (Arg1 >= Emulator->Registers[ST] &&
            Arg1 <= Emulator->Registers[HT]) {
            return ErrDataAccessViolation;
        }

        C = getc(stdin);
        Emulator->DataStore[Arg1] = C;
        break;
    case 22: // put
        POP(Emulator, &Arg1);
        putc(Arg1, stdout);
        break;
    case 23: // geteol
        while ((C = getc(stdin)) != '\n') {
            // pass
        }
        break;
    case 24: // puteol
        putc('\n', stdout);
        break;
    case 25: // getint
        POP(Emulator, &Arg1);
        if (Arg1 >= Emulator->Registers[ST] &&
            Arg1 <= Emulator->Registers[HT]) {
            return ErrDataAccessViolation;
        }

        fscanf(stdin, "%d", (int *)&Arg2);
        Emulator->DataStore[Arg1] = Arg2 & 0xffff;
        break;
    case 26: // putint
        POP(Emulator, &Arg1);
        fprintf(stdout, "%d", Arg1);
        break;
    }
    return OK;
}

static int execReturn(TamEmulator *Emulator, Instruction Instr) {
    // pop result
    DATA_W Result[Instr.N];
    for (int i = 0; i < Instr.N; ++i) {
        POP(Emulator, Result + i);
    }

    // pop frame and arguments
    ADDRESS ReturnAddrAddr = Emulator->Registers[LB] + 2;
    ADDRESS ReturnAddr = Emulator->DataStore[ReturnAddrAddr];
    ADDRESS DynamicLinkAddr = Emulator->Registers[LB] + 1;
    ADDRESS DynamicLink = Emulator->DataStore[DynamicLinkAddr];

    Emulator->Registers[ST] = Emulator->Registers[LB];
    for (int i = 0; i < Instr.D; ++i) {
        POP(Emulator, NULL);
    }

    // push result and update registers
    for (int i = Instr.N - 1; i >= 0; --i) {
        PUSH(Emulator, Result[i]);
    }
    Emulator->Registers[LB] = DynamicLink;
    Emulator->Registers[CP] = ReturnAddr;
    return OK;
}

static int execPush(TamEmulator *Emulator, Instruction Instr) {
    if (Emulator->Registers[ST] + Instr.D >= Emulator->Registers[HT]) {
        return ErrStackOverflow;
    }

    Emulator->Registers[ST] += Instr.D;
    return OK;
}

static int execPop(TamEmulator *Emulator, Instruction Instr) {
    TamError Err;

    // pop value
    DATA_W Value[Instr.N];
    for (int i = 0; i < Instr.N; ++i) {
        POP(Emulator, Value + i);
    }

    // pop spares
    for (int i = 0; i < Instr.D; ++i) {
        POP(Emulator, NULL);
    }

    // push value
    for (int i = Instr.N - 1; i >= 0; --i) {
        PUSH(Emulator, Value[i]);
    }

    return OK;
}

static int execJump(TamEmulator *Emulator, Instruction Instr) {
    ADDRESS Addr = calcAddress(Emulator, Instr);
    if (Addr >= Emulator->Registers[CT]) {
        return ErrCodeAccessViolation;
    }

    Emulator->Registers[CP] = Addr;
    return OK;
}

static int execJumpi(TamEmulator *Emulator, Instruction Instr) {
    ADDRESS Addr;
    POP(Emulator, (DATA_W *)&Addr);

    if (Addr >= Emulator->Registers[CT]) {
        return ErrCodeAccessViolation;
    }

    Emulator->Registers[CP] = Addr;
    return OK;
}

static int execJumpif(TamEmulator *Emulator, Instruction Instr) {
    DATA_W Arg;
    POP(Emulator, &Arg);

    if (Arg != Instr.N) {
        return OK;
    }

    ADDRESS Addr = calcAddress(Emulator, Instr);
    if (Addr >= Emulator->Registers[CT]) {
        return ErrCodeAccessViolation;
    }

    Emulator->Registers[CP] = Addr;
    return OK;
}

int execute(TamEmulator *Emulator, Instruction Instr) {
    switch (Instr.Op) {
    case LOAD:
        return execLoad(Emulator, Instr);
    case LOADA:
        return execLoada(Emulator, Instr);
    case LOADI:
        return execLoadi(Emulator, Instr);
    case LOADL:
        return execLoadl(Emulator, Instr);
    case STORE:
        return execStore(Emulator, Instr);
    case STOREI:
        return execStorei(Emulator, Instr);
    case CALL:
        if (Instr.R == PB && Instr.D > 0 && Instr.D < 29) {
            return execCallPrimitive(Emulator, Instr);
        }
        return execCall(Emulator, Instr);
    case RETURN:
        return execReturn(Emulator, Instr);
    case PUSH:
        return execPush(Emulator, Instr);
    case POP:
        return execPop(Emulator, Instr);
    case JUMP:
        return execJump(Emulator, Instr);
    case JUMPI:
        return execJumpi(Emulator, Instr);
    case JUMPIF:
        return execJumpif(Emulator, Instr);
    case HALT:
        break;
    default:
        fprintf(stderr, "Unrecognised opcode: %d", Instr.Op);
        return 0;
    }

    return OK;
}
