#ifndef _VM_CPP_
#define _VM_CPP_

#ifndef MAX_STACK_SIZE
#define MAX_STACK_SIZE 256
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef unsigned char byte_t;
typedef int32_t index_t;

// Since they are all the same type and not classes, we can just hug 'em together!
enum : byte_t {
  OPCODE_NULL = 0,
  // The Basics
  OPCODE_PUSH = 11,
  OPCODE_POP,

  OPCODE_CALL = 21,
  OPCODE_RETURN,

  OPCODE_STORE = 31, // Replace the what the left register points to with what's in the right register
  OPCODE_LOAD,  // Replace the left register with what the right register points to
  OPCODE_LOADC, // Replace the left register with a value in the program

  OPCODE_SBP = 41, // Replace the left register with a pointer to a stack position
  OPCODE_SFP, // Same as previous, but offset by the current frame

  OPCODE_SWAP = 51,

  // We never care both about type and register, but we always care about size
  // Neat thing is we can add this to 'registers' to get the proper register!
  REGISTER_LEFT = 0x00,
  REGISTER_RIGHT = 0x08,
  
  TYPE_VOID  = 0x00,
  TYPE_UINT  = 0x01,
  TYPE_SINT  = 0x02,
  TYPE_FLOAT = 0x03,

  SIZE_8  = 0x00,
  SIZE_16 = 0x01,
  SIZE_32 = 0x02,
  SIZE_64 = 0x03,
};

#define UPPER(x) (x >> 4)
#define LOWER(x) (x & 15)
#define MERGE(x, y) ((x << 4) | y)

class VM {
  byte_t stack[MAX_STACK_SIZE];
  index_t frame;
  index_t stack_end;

  byte_t registers[16];

  byte_t *instructions = nullptr;
  index_t prog_counter;

  void push(void *loc, index_t size) {
    if (MAX_STACK_SIZE < stack_end + size) exit(12);
    memcpy(stack + stack_end, loc, size);
    stack_end += size;
  }

  void pop(void *loc, index_t size) {
    if (stack_end < size) exit(11);
    stack_end -= size;
    memcpy(loc, stack + stack_end, size);
  }
public:
  VM() {
    if (sizeof(void *) > 8) {
      printf(" -- Systems above x64 are incompatible with this program! -- \n");
      exit(-20);
    } else if (sizeof(void *) != 8) {
      printf(" -- Non-x64 systems may suffer performance and memory issues! -- \n");
    }
  }

  void init(byte_t *instructs) {
    instructions = instructs;
    memset(stack, 0, MAX_STACK_SIZE);
    memset(registers, 0, 16);

    frame = 0;
    stack_end = 0;
    prog_counter = -1;
  }

  // Increase prog_counter by num_ but also only get the item at the index of the old prog_counter
  #define GET_BYTES(num_) (instructions+((prog_counter+=num_)-num_))
  void execute() {
    if (!instructions) exit(13);
    
    push(&prog_counter, 4);
    push(&frame, 4);
    prog_counter = 0;

    do {
      execute_one(*GET_BYTES(1));
    } while (prog_counter >= 0);
  }

  void execute_one(byte_t instruction) {
    prog_counter++;
    switch (instruction) {
    case OPCODE_SWAP: {
      uint64_t left = * (uint64_t *) registers;
      * (uint64_t *) registers = * (uint64_t *) (registers + 8);
      * (uint64_t *) (registers + 8) = left;
    } break;

    case OPCODE_CALL: {
      push(&prog_counter, 4);
      push(&frame, 4);
      prog_counter = * (index_t *) GET_BYTES(4);
      frame = stack_end;
    } break;
    case OPCODE_RETURN: {
      pop(&frame, 4);
      pop(&prog_counter, 4);
    } break;

    case OPCODE_POP: {
      byte_t reg = *GET_BYTES(1);
      pop(registers + UPPER(reg), LOWER(reg));
    } break;
    case OPCODE_PUSH: {
      byte_t reg = *GET_BYTES(1);
      push(registers + UPPER(reg), LOWER(reg));
    } break;

    case OPCODE_LOAD: {
      byte_t size = *GET_BYTES(1);
      memcpy(registers, * (void **) (registers + 8), size);
    } break;
    case OPCODE_STORE: {
      byte_t size = *GET_BYTES(1);
      memcpy(* (void **) (registers + 8), registers, size);
    } break;
    case OPCODE_LOADC: {
      byte_t size = *GET_BYTES(1);
      memcpy(registers, instructions + (* (index_t *) GET_BYTES(4)), size);
    } break;
    
    case OPCODE_SBP: {
      * (void **) registers = stack + (* (index_t *) GET_BYTES(4));
    } break;
    case OPCODE_SFP: {
      * (void **) registers = stack + frame + (* (index_t *) GET_BYTES(4));
    } break;

    default:
      exit(13);
      break;
    }
  }
  #undef GET_BYTE
};

constexpr byte_t best_type(byte_t a, byte_t b) {
  return 
    // a if a is a better type
    UPPER(a) > UPPER(b) ? a : (
    // Same for b
    UPPER(b) > UPPER(a) ? b : (
    // The types are the same, so choose the max size
    a > b ? a : b
  ));
}

#endif // _VM_CPP_