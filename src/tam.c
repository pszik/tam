#include <tam/tam.h>

#include <stdio.h>
#include <tam/error.h>

int loadProgram(TamEmulator *Emulator, const char *Filename) {
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
        return 0;
    }

    uint32_t Code = Emulator->CodeStore[Idx];
    *Instr = (Instruction){(Code & 0xf0000000) >> 28, (Code & 0x0f000000) >> 24,
                           (Code & 0x00ff0000) >> 16, (Code & 0x0000ffff)};
    return 1;
}

static int pushData(TamEmulator *Emulator, DATA_W Datum) {
    if (Emulator->Registers[ST] >= Emulator->Registers[HT]) {
        return 0;
    }

    ADDRESS Addr = Emulator->Registers[ST]++;
    Emulator->DataStore[Addr] = Datum;
    return 1;
}

static int popData(TamEmulator *Emulator, DATA_W *Datum) {
    if (!Emulator->Registers[ST]) {
        return 0;
    }

    ADDRESS Addr = --Emulator->Registers[ST];
    *Datum = Emulator->DataStore[Addr];
    return 1;
}

static ADDRESS calcAddress(TamEmulator *Emulator, Instruction Instr) {
    return Emulator->Registers[Instr.R] + Instr.D;
}

static int execLoad(TamEmulator *Emulator, Instruction Instr) {
    ADDRESS BaseAddr = calcAddress(Emulator, Instr);
    for (int i = 0; i < Instr.N; ++i) {
        ADDRESS Addr = BaseAddr + i;
        DATA_W Data = Emulator->DataStore[Addr];
        if (!pushData(Emulator, Data)) {
            return 0;
        }
    }
    return 1;
}

static int execLoada(TamEmulator *Emulator, Instruction Instr) {
    ADDRESS Addr = calcAddress(Emulator, Instr);
    pushData(Emulator, (DATA_W)Addr);
    return 0;
}

static int execLoadi(TamEmulator *Emulator, Instruction Instr) {
    ADDRESS BaseAddr;
    if (!popData(Emulator, (DATA_W *)&BaseAddr)) {
        return 0;
    }

    for (int i = 0; i < Instr.N; ++i) {
        ADDRESS Addr = BaseAddr + i;
        DATA_W Data = Emulator->DataStore[Addr];
        pushData(Emulator, Data);
    }
    return 1;
}

static int execLoadl(TamEmulator *Emulator, Instruction Instr) {
    return pushData(Emulator, Instr.D);
}

static int execStore(TamEmulator *Emulator, Instruction Instr) {
    // pop value
    DATA_W Value[Instr.N];
    for (int i = 0; i < Instr.N; ++i) {
        if (!popData(Emulator, Value + i)) {
            return 0;
        }
    }

    // store
    ADDRESS BaseAddr = calcAddress(Emulator, Instr);
    for (int i = 0; i < Instr.N; ++i) {
        ADDRESS Addr = BaseAddr + i;
        Emulator->DataStore[Addr] = Value[Instr.N - i - 1];
    }

    return 1;
}

static int execStorei(TamEmulator *Emulator, Instruction Instr) {
    // pop value
    DATA_W Value[Instr.N];
    for (int i = 0; i < Instr.N; ++i) {
        if (!popData(Emulator, Value + i)) {
            return 0;
        }
    }

    // store
    ADDRESS BaseAddr;
    if (!popData(Emulator, (DATA_W *)&BaseAddr)) {
        return 0;
    }

    for (int i = 0; i < Instr.N; ++i) {
        ADDRESS Addr = BaseAddr + i;
        Emulator->DataStore[Addr] = Value[Instr.N - i - 1];
    }

    return 1;
}

static int execJump(TamEmulator *Emulator, Instruction Instr) {
    ADDRESS Addr = calcAddress(Emulator, Instr);
    if (Addr >= Emulator->Registers[CT]) {
        return 0;
    }

    Emulator->Registers[CP] = Addr;
    return 1;
}

static int execJumpi(TamEmulator *Emulator, Instruction Instr) {
    ADDRESS Addr;
    if (!popData(Emulator, (DATA_W *)&Addr)) {
        return 0;
    }

    if (Addr >= Emulator->Registers[CT]) {
        return 0;
    }

    Emulator->Registers[CP] = Addr;
    return 0;
}

static int execJumpif(TamEmulator *Emulator, Instruction Instr) {
    DATA_W Arg;
    if (!popData(Emulator, &Arg)) {
        return 0;
    }

    if (Arg != Instr.N) {
        return 1;
    }

    ADDRESS Addr = calcAddress(Emulator, Instr);
    if (Addr >= Emulator->Registers[CT]) {
        return 0;
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
