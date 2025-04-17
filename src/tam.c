#include <tam/tam.h>

#include <stdio.h>
#include <string.h>
#include <tam/error.h>

int loadProgram(TamEmulator *Emulator, const char *Filename) {
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

    return OK;
}

int fetchDecode(TamEmulator *Emulator, Instruction *Instr) {
    uint16_t Idx = Emulator->Registers[CP]++;
    if (Idx >= Emulator->Registers[CT]) {
        return ErrCodeAccessViolation;
    }

    uint32_t Code = Emulator->CodeStore[Idx];
    *Instr = (Instruction){(Code & 0xf0000000) >> 28, (Code & 0x0f000000) >> 24,
                           (Code & 0x00ff0000) >> 16, (Code & 0x0000ffff)};
    return OK;
}

static int pushData(TamEmulator *Emulator, DATA_W Datum) {
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

    if (Datum == NULL) {
        return OK;
    }

    ADDRESS Addr = --Emulator->Registers[ST];
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
    return 1;
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

    switch (Instr.D) {
    case 1: // id
        break;
    case 2: // not
        POP(Emulator, &Arg1);
        PUSH(Emulator, Arg1 ? 0 : 1);
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
    return 0;
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
    return 1;
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

    return 1;
}
