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
    uint32_t Code = Emulator->CodeStore[Idx];
    *Instr = (Instruction){(Code & 0xf0000000) >> 28, (Code & 0x0f000000) >> 24,
                           (Code & 0x00ff0000) >> 16, (Code & 0x0000ffff)};
    return 1;
}

int execute(TamEmulator *Emulator, Instruction Instr) { return 0; }