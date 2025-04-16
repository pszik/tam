#ifndef TAM_TAM_H__
#define TAM_TAM_H__

#include <stdint.h>

/// Maximum number of addressable words.
#define MEMORY_SIZE 65536

/// @brief A single TAM emulator.
typedef struct {
    uint32_t CodeStore[MEMORY_SIZE]; ///< Contains the program to execute
    int16_t DataStore[MEMORY_SIZE]; ///< Contains the stack and global variables
    uint16_t Registers[16];         ///< Contains register values
} TamEmulator;

typedef enum {
    LOAD,
    LOADA,
    LOADI,
    LOADL,
    STORE,
    STOREI,
    CALL,
    CALLI,
    RETURN,
    NOOP,
    PUSH,
    POP,
    JUMP,
    JUMPI,
    JUMPIF,
    HALT
} Opcode;

typedef enum {
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
typedef struct {
    Opcode Op;  ///< Opcode
    Register R; ///< Register
    uint8_t N;  ///< Unsigned operand
    int16_t D;  ///< Signed operand
} Instruction;

/// @brief Read a TAM binary into an emulator's code store.
/// @param[in,out] Emulator emulator to load
/// @param Filename name of file to read from
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