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
  // The Basics
  OPCODE_PUSH,
  OPCODE_POP,

  OPCODE_CALL,
  OPCODE_RETURN,

  OPCODE_STORE, // Replace the what the left register points to with what's in the right register
  OPCODE_LOAD,  // Replace the right register with what the left register points to
  OPCODE_LOADC, // Replace the left register with a value in the program

  OPCODE_SBP, // Replace the left register with a pointer to a stack position
  OPCODE_SFP, // Same as previous, but offset by the current frame

  TYPE_VOID  = 0x00,
  TYPE_UINT  = 0x01,
  TYPE_SINT  = 0x02,
  TYPE_FLOAT = 0x03,

  SIZE_8  = 0x00,
  SIZE_16 = 0x01,
  SIZE_32 = 0x02,
  SIZE_64 = 0x03,
};

class VM {
  byte_t stack[MAX_STACK_SIZE] = {};
  index_t frame = 0;
  index_t stack_end = 0;

  byte_t *instructions = nullptr;
  index_t prog_counter = 0;
public:
  VM() {
    if (sizeof(void *) > 8) {
      printf(" -- Systems above x64 are incompatible with this program! -- \n");
      exit(-20);
    } else if (sizeof(void *) != 8) {
      printf(" -- Non-x64 systems may suffer performance and memory issues! -- \n");
    }
  }

  // Increase prog_counter by num_ but also only get the item at the index of the old prog_counter
  #define GET_BYTE(num_) (instructions+((prog_counter+=num_)-num_))
  void execute_one() {
    switch (*GET_BYTE(1)) {
    
    default:
      exit(13); // Sorry all you treskidekaphobes. It the first number I chose
      break;
    }
  }
  #undef GET_BYTE
};

#endif // _VM_CPP_