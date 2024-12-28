#ifndef _VM_CPP_
#define _VM_CPP_

#ifndef MAX_STACK_SIZE
#define MAX_STACK_SIZE 256
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define SWITCH_CASE(_case, _code) \
case _case: \
{_code} break;

/* Exit codes:
0 - Ok
1 - Memory access bounds check failed
2 - Invalid argument
3 - Memory allocation failed

10 - Invalid instruction
11 - Invalid SPECCALL id
12 - Invalid instruction parameter

20 - Invalid execution state
*/

typedef unsigned char byte;

enum : byte { // If no register is specified, assume left
  //OPCODE_NULL,

  OPCODE_CALL, // 1
  OPCODE_RETURN, // 2
  
  OPCODE_SPP, // 3 - Sets register to pointer to stack data
  OPCODE_FPP, // 4 - Sets register to pointer to stack frame data
  
  OPCODE_STORE, // 5 - Indirect store to the pointer in the register
  OPCODE_LOAD, // 6 - Indirect load from the pointer in the register
  OPCODE_LOADC,  // 7 - Load a constant
  
  OPCODE_SWAP, // 8 - Swaps the registers
  
  OPCODE_CONV, // 9 - Convert left from type to type
  
  OPCODE_JMP, // 10 - Sets the program counter
  OPCODE_JMPZ, // 11 - JMP if register is zero (if false)
  OPCODE_JMPNZ, // 12 - JMP if register is not zero (if true)
  
  OPCODE_CMPE, // 13 - Register = 0 if left == right
  OPCODE_CMPL, // 14 - Register = 0 if left > right for a given type
  OPCODE_CMPG, // 15 - Register = 0 if left < right for a given type
  // Use these with bitwise operations
  
  OPCODE_PUSH, // 16 - Pushes a register
  OPCODE_POP,  // 17 - Pops a register
  
  // Arithmetc
  OPCODE_ADD, // 18
  OPCODE_SUB, // 19
  OPCODE_MUL, // 20
  OPCODE_DIV, // 21
  OPCODE_NEG, // 22
  // Float-specific arithmetic
  OPCODE_FFLOOR, // 23
  OPCODE_FCEIL, // 24
  OPCODE_FTRIG, // 25 - Like SPECCALL, but for trig functions
  
  // The following expect a size parameter
  OPCODE_AND, // 26 - Bitwise AND
  OPCODE_OR,  // 27 - Bitwise OR
  OPCODE_XOR, // 28 - Bitwise XOR
  OPCODE_NOT, // 29 - Reverse bits of register

  // Boolean operations
  OPCODE_BAND, // 30
  OPCODE_BOR, // 31
  OPCODE_BNOT, // 32
  
  OPCODE_SPECCALL, // 33 - Call a VM function of given ID

  REG_LEFT  = 0x00,
  REG_RIGHT = 0x08,
  
  TYPE_NONE     = 0x00,
  TYPE_UNSIGNED = 0x01,
  TYPE_SIGNED   = 0x02,
  TYPE_FLOAT    = 0x03,
  
  TYPE_SIZE_8  = 0x00,
  TYPE_SIZE_16 = 0x01,
  TYPE_SIZE_32 = 0x02,
  TYPE_SIZE_64 = 0x03,
};

#define FROM_SIZE(size) (TYPE_SIZE_##size)

#define UPPER(x) (x >> 4)
#define LOWER(x) (x & 0x0F)
#define MERGE(u, d) ((u << 4) | d)

#define TYPE_TO_LEFT(x)  MERGE(REG_LEFT , LOWER(x))
#define TYPE_TO_RIGHT(x) MERGE(REG_RIGHT, LOWER(x))

template<class T> constexpr T max(T a, T b) { return a > b ? a : b; }
template<class T> constexpr T min(T a, T b) { return a < b ? a : b; }

constexpr byte best_type(byte a, byte b) {
  // If (a_up > b_up) return a;
  // If (b_up > a_up) return b;
  // // a_up == b_up
  // return MERGE(a_up, max(a_low, b_low))
  return UPPER(a) > UPPER(b) ? a : (UPPER(b) > UPPER(a) ? b : MERGE(UPPER(a), max(LOWER(a), LOWER(b))));
}

static void swap_u64(uint64_t *a, uint64_t *b) {
  uint64_t t = *a;
  *a = *b;
  *b = t;
}

struct VM {
  const byte *instructions = nullptr;
  int instructions_size = 0;
  int prog_counter = 0;
  byte registers[8*2] = {};
  byte stack_base[MAX_STACK_SIZE] = {};
  int32_t stack_end;
  int32_t stack_frame;
  
  #define stack_ptr (stack_base + stack_end)
  #define frame_ptr (stack_base + stack_frame)
  
  void push(const void *data, int size) {
    if (stack_base + MAX_STACK_SIZE < stack_ptr) exit(1);
    memcpy(stack_ptr, data, size);
    stack_end += size;
  }
  
  void pop(void *dest, int size) {
    if (stack_base + size > stack_ptr) exit(1);
    stack_end -= size;
    memcpy(dest, stack_ptr, size);
  }
  
  #define GET_BYTES(num) (instructions + (prog_counter+=num)-num)
  void execute_one() {
    printf("prog_counter: %d\n", prog_counter);
    if (prog_counter >= instructions_size) exit(20);
    byte opcode = *GET_BYTES(1);
    printf("opcode: %d\n", (int) opcode);
    switch(opcode) {
      SWITCH_CASE(OPCODE_LOADC, {
        char size = 1 << (*GET_BYTES(1));
        int32_t pos = *(int32_t *) GET_BYTES(4);
        if (instructions_size < pos + size) exit(1);
        memcpy(registers, instructions + pos, size);
      })
      
      SWITCH_CASE(OPCODE_SWAP, {
        swap_u64(
          (uint64_t *) registers,
          (uint64_t *) (registers + 8)
        );
      })
      
      #define APPLY_OPB(type, op) \
      do { \
        *(type *) registers op##= *(type *) (registers + 8); \
        break; \
      } while(false)
      
      #define APPLY_OPU(type, op) \
      do { \
        *(type *) registers = op (*(type *) registers); \
        break; \
      } while(false)
      
      #define APPLY_OPC(type, op) \
      do { \
        *(uint8_t *) registers = \
          (*(type *) registers) op \
          (*(type *) (registers + 8)); \
        break; \
      } while(false)
      
      #define OP_CASE(name, op, ub) \
      SWITCH_CASE(OPCODE_##name, { \
        switch (*GET_BYTES(1)) { \
          case MERGE(TYPE_UNSIGNED, FROM_SIZE(8)): \
            APPLY_OP##ub(uint8_t , op); \
            break; \
          case MERGE(TYPE_UNSIGNED, FROM_SIZE(16)): \
            APPLY_OP##ub(uint16_t, op); \
            break; \
          case MERGE(TYPE_UNSIGNED, FROM_SIZE(32)): \
            APPLY_OP##ub(uint32_t, op); \
            break; \
          case MERGE(TYPE_UNSIGNED, FROM_SIZE(64)): \
            APPLY_OP##ub(uint64_t, op); \
            break; \
          case MERGE(TYPE_SIGNED, FROM_SIZE(8)): \
            APPLY_OP##ub( int8_t , op); \
            break; \
          case MERGE(TYPE_SIGNED, FROM_SIZE(16)): \
            APPLY_OP##ub( int16_t, op); \
            break; \
          case MERGE(TYPE_SIGNED, FROM_SIZE(32)): \
            APPLY_OP##ub( int32_t, op); \
            break; \
          case MERGE(TYPE_SIGNED, FROM_SIZE(64)): \
            APPLY_OP##ub( int64_t, op); \
            break; \
          case MERGE(TYPE_FLOAT, FROM_SIZE(32)): \
            APPLY_OP##ub(float, op); \
            break; \
          case MERGE(TYPE_FLOAT, FROM_SIZE(64)): \
            APPLY_OP##ub(double, op); \
            break; \
        } \
      })
      
      OP_CASE(ADD, +, B)
      OP_CASE(SUB, -, B)
      OP_CASE(MUL, *, B)
      OP_CASE(DIV, /, B)
      
      OP_CASE(NEG, -, U)
      
      OP_CASE(CMPE, ==, C)
      OP_CASE(CMPL, < , C)
      OP_CASE(CMPG, > , C)
      
      #undef OP_CASE
      #define OP_CASE(name, op, ub) \
      SWITCH_CASE(OPCODE_##name, { \
        switch (*GET_BYTES(1)) { \
          case MERGE(TYPE_UNSIGNED, FROM_SIZE(8)): \
            APPLY_OP##ub(uint8_t , op); \
            break; \
          case MERGE(TYPE_UNSIGNED, FROM_SIZE(16)): \
            APPLY_OP##ub(uint16_t, op); \
            break; \
          case MERGE(TYPE_UNSIGNED, FROM_SIZE(32)): \
            APPLY_OP##ub(uint32_t, op); \
            break; \
          case MERGE(TYPE_UNSIGNED, FROM_SIZE(64)): \
            APPLY_OP##ub(uint64_t, op); \
            break; \
          case MERGE(TYPE_SIGNED, FROM_SIZE(8)): \
            APPLY_OP##ub(uint8_t , op); \
            break; \
          case MERGE(TYPE_SIGNED, FROM_SIZE(16)): \
            APPLY_OP##ub(uint16_t, op); \
            break; \
          case MERGE(TYPE_SIGNED, FROM_SIZE(32)): \
            APPLY_OP##ub(uint32_t, op); \
            break; \
          case MERGE(TYPE_SIGNED, FROM_SIZE(64)): \
            APPLY_OP##ub(uint64_t, op); \
            break; \
          case MERGE(TYPE_FLOAT, FROM_SIZE(32)): \
            APPLY_OP##ub(uint32_t, op); \
            break; \
          case MERGE(TYPE_FLOAT, FROM_SIZE(64)): \
            APPLY_OP##ub(uint64_t, op); \
            break; \
        } \
      })
      
      OP_CASE(XOR, ^, B)
      OP_CASE(AND, &, B)
      OP_CASE(OR , |, B)
      OP_CASE(NOT, ~, U)

      SWITCH_CASE(OPCODE_BAND, {
        APPLY_OPC(uint8_t, &&);
      })

      SWITCH_CASE(OPCODE_BOR, {
        APPLY_OPC(uint8_t, ||);
      })

      SWITCH_CASE(OPCODE_BNOT, {
        APPLY_OPU(uint8_t, !);
      })
      
      SWITCH_CASE(OPCODE_RETURN, {
        pop(&prog_counter, 4);
        pop(&stack_frame, 4);
        printf("Return\n");
      })
      
      SWITCH_CASE(OPCODE_CALL, {
        push(&stack_frame, 4);
        push(&prog_counter, 4);
        prog_counter = *(int32_t *) GET_BYTES(4);
        stack_frame = stack_end;
        printf("Call (%d)\n", (int) OPCODE_CALL);
      })
      
      SWITCH_CASE(OPCODE_PUSH, {
        byte reg = *GET_BYTES(1);
        push(registers + UPPER(reg), 1 << LOWER(reg));
        printf("Push 0x%.2hhX\n", reg);
      })
      
      SWITCH_CASE(OPCODE_POP, {
        byte reg = *GET_BYTES(1);
        pop(registers + UPPER(reg), 1 << LOWER(reg));
        printf("Pop 0x%.2hhX\n", reg);
      })
      
      SWITCH_CASE(OPCODE_LOAD, {
        char size = 1 << (*GET_BYTES(1));
        memcpy(registers, *(void **) registers, size);
      })
      
      SWITCH_CASE(OPCODE_STORE, {
        char size = 1 << (*GET_BYTES(1));
        memcpy(*(void **) registers, registers, size);
      })
      
      SWITCH_CASE(OPCODE_SPP, {
        int32_t index = *(int32_t *) GET_BYTES(4);
        * (void **) registers = stack_base + index;
      })
      
      SWITCH_CASE(OPCODE_FPP, {
        int32_t index = *(int32_t *) GET_BYTES(4);
        * (void **) registers = frame_ptr + index;
      })
      
      SWITCH_CASE(OPCODE_JMP, {
        prog_counter = *(int32_t *) GET_BYTES(4);
        printf("JMP triggered\n");
      })
      
      SWITCH_CASE(OPCODE_JMPZ, {
        printf("JMPZ...\n");
        int32_t pos = *(int32_t *) GET_BYTES(4);
        printf("pos = %d\n", pos);
        if (*(uint8_t *) registers) return;
        printf("...triggered\n");
        prog_counter = pos;
      })

      SWITCH_CASE(OPCODE_JMPNZ, {
        printf("JMPNZ...\n");
        int32_t pos = *(int32_t *) GET_BYTES(4);
        printf("pos = %d\n", pos);
        if (*(uint8_t *) registers == 0) return;
        printf("...triggered\n");
        prog_counter = pos;
      })

      default:
        printf("Expected valid instruction\n", opcode);
        exit(11);
    }
  }
  
  void execute() {
    prog_counter = -10;
    push(&stack_frame, 4);
    push(&prog_counter, 4);
    prog_counter = 0;
    do {
      execute_one();
    } while(prog_counter >= 0);
  }
  
  void init() {
    if (sizeof(void *) != 8) exit(-20);
    stack_end   = 0;
    stack_frame = 0;
  }
  #undef GET_BYTES
  #undef APPLY_OPU
  #undef APPLY_OPB
  #undef OP_CASE
};

#undef SWITCH_CASE

#endif // _VM_CPP_