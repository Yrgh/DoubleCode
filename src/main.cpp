#include <iostream>
#include <vm.cpp>

int main(int argv, char **argc) {
  VM vm;
  byte_t instructions[] = {
    OPCODE_RETURN,
  };
  vm.init(instructions);
  vm.execute();
  std::cout << "Done\n";
  return 0;
}