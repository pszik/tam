#include <tam/tam.h>

#include <stdio.h>

int loadProgram(TamEmulator *Emulator, const char *Filename) {
    FILE *File = fopen(Filename, "rb");
    if (!File) {
        return 0;
    }

    // get file length
    int FileLength;
    fseek(File, 0, SEEK_END);
    FileLength = ftell(File);
    rewind(File);

    if (FileLength % 4 != 0) {
        return 0;
    }

    if (FileLength > MEMORY_SIZE) {
        return 0;
    }

    // read bytes
    uint8_t Buf[4];
    for (int i = 0; i < FileLength; ++i) {
        fread(Buf, 4, 1, File);
        Emulator->CodeStore[i] =
            Buf[0] << 24 | Buf[1] << 16 | Buf[2] << 8 | Buf[3];
    }
    Emulator->Registers[CT] = FileLength;

    return 1;
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
    if (!popData(Emulator, &BaseAddr)) {
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
    if (!popData(Emulator, &BaseAddr)) {
        return 0;
    }

    for (int i = 0; i < Instr.N; ++i) {
        ADDRESS Addr = BaseAddr + i;
        Emulator->DataStore[Addr] = Value[Instr.N - i - 1];
    }

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
    case HALT:
        break;
    default:
        fprintf(stderr, "Unrecognised opcode: %d", Instr.Op);
        return 0;
    }

    return 1;
}
