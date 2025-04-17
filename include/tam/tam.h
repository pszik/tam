#ifndef TAM_TAM_H__
#define TAM_TAM_H__

#include <stdint.h>
#include <stdlib.h>

/// Maximum number of addressable words.
#define MEMORY_SIZE 65536

/// Type of words in the code store.
#define CODE_W uint32_t
/// Type of words in the data store.
#define DATA_W int16_t
/// Type of addresses.
#define ADDRESS uint16_t

/// @brief A single TAM emulator.
typedef struct TamEmulator {
    CODE_W CodeStore[MEMORY_SIZE]; ///< Contains the program to execute
    DATA_W DataStore[MEMORY_SIZE]; ///< Contains the stack and global variables
    ADDRESS Registers[16];         ///< Contains register values
} TamEmulator;

/// Allocate a new emulator with all memory zeroed.
/// @return pointer to the emulator
static TamEmulator *newEmulator() {
    return (TamEmulator *)calloc(1, sizeof(TamEmulator));
}

typedef enum Opcode {
    LOAD = 0,
    LOADA,
    LOADI,
    LOADL,
    STORE,
    STOREI,
    CALL,
    CALLI,
    RETURN,
    PUSH = 10,
    POP,
    JUMP,
    JUMPI,
    JUMPIF,
    HALT
} Opcode;

typedef enum Register {
    CB, ///< Code base
    CT, ///< Code top
    PB, ///< Primitive base
    PT, ///< Primitive top
    SB, ///< Stack base
    ST, ///< Stack top
    HB, ///< Heap base
    HT, ///< Heap top
    LB, ///< Local base
    L1, ///< Local 1
    L2, ///< Local 2
    L3, ///< Local 3
    L4, ///< Local 4
    L5, ///< Local 5
    L6, ///< Local 6
    CP  ///< Code pointer (program counter)
} Register;

/// @brief A single 32-bit instruction.
typedef struct Instruction {
    Opcode Op;  ///< Opcode
    Register R; ///< Register
    uint8_t N;  ///< Unsigned operand
    int16_t D;  ///< Signed operand
} Instruction;

/// @brief Read a TAM binary into an emulator's code store.
/// @param[in,out] Emulator emulator to load
/// @param[in] Filename name of file to read from
/// @return 0 if loading failed, 1 otherwise
int loadProgram(TamEmulator *Emulator, const char *Filename);

/// @brief Fetch and decode the next instruction to be executed.
/// @param[in,out] Emulator emulator to use
/// @param[out] Instr pointer to receive the decoded instruction
/// @return 0 if there was an error, 1 otherwise
int fetchDecode(TamEmulator *Emulator, Instruction *Instr);

/// @brief Execute a given instruction on an emulator.
/// @param[in,out] Emulator emulator to use
/// @param Instr instruction to execute
/// @return 0 if there was an error, 1 otherwise
int execute(TamEmulator *Emulator, Instruction Instr);

#endif
