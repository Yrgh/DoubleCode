#include <bitset>
#include <fstream>
#include <iostream>

#include "astparser.cpp"
#include "compiler.cpp"
#include "vm.cpp"

int main() {
  // Open, get the length of, and read the source code file
  std::ifstream source("C:\\Users\\adyng\\source\\vscode\\DoubleCode\\example.dcs");
  if (!source.is_open()) {
    printf("File could not be opened. Teminating...\n");
    return 2;
  }
  source.seekg(0, std::ios::end);
  int length = source.tellg();
  source.seekg(0, std::ios::beg);
  char *input_buf = new char[length];
  memset(input_buf, 0, length);
  source.read(input_buf, length);
  source.close();

  Parser parser;
  printf("Parsing...\n");
  parser.parse(input_buf);
  printf("Parse done. Printing...\n");
  parser.top->print(0);

  Compiler compiler;
  printf("Compiling...\n");
  if (!compiler.compile(parser.top)) return 1;
  printf("Compilation successful!\n");
  
  // Free those resources
  delete parser.top;
  delete[] input_buf;

  std::cout << "Executing\n";

  VM vm;
  vm.init();
  std::cout << (void *) vm.stack_base << "\n";
  vm.instructions = compiler.resultData();
  vm.instructions_size = compiler.resultSize();
  printf("Program size: %d\n", vm.instructions_size);
  vm.execute();

  std::cout << "Results:\n";
  std::cout << "   Left: 0b" << std::bitset<64>(*(uint64_t *)(vm.registers)) << "\n";
  std::cout << "   Left: " << *(int64_t*)(vm.registers) << "\n";
  std::cout << "   Left: " << *(float *)(vm.registers) << "f\n";
  std::cout << "   Left: " << *(double *)(vm.registers) << "d\n";
  std::cout << "  Right: 0b" << std::bitset<64>(*(uint64_t *)(vm.registers + 8)) << "\n";
  std::cout << "  Right: " << *(int64_t*)(vm.registers + 8) << "\n";
  std::cout << "  Right: " << *(float *)(vm.registers + 8) << "f\n";
  std::cout << "  Right: " << *(double *)(vm.registers + 8) << "d\n";

  return 0;
}