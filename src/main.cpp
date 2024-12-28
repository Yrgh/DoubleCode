/******************************************************************************

                              Online C++ Compiler.
               Code, Compile, Run and Debug C++ program online.
Write your code in this editor and press "Run" button to compile and execute it.

*******************************************************************************/

#include <bitset>
#include <fstream>
#include <iostream>

#include "compiler.cpp"
#include "vm.cpp"

int main() {
  // Open, get the length of, and read the source code file
  std::ifstream source("example.txt");
  source.seekg(0, std::ios::end);
  int length = source.tellg();
  source.seekg(0, std::ios::beg);
  char *input_buf = new char[length];
  memset(input_buf, 0, length);
  source.read(input_buf, length);
  source.close();

  Compiler compiler;
  compiler.compile(input_buf);

  delete[] input_buf; // We don't need the source code anymore

  std::cout << "Executing\n";

  VM vm;
  vm.init();
  vm.instructions = compiler.getResultData();
  vm.instructions_size = compiler.getResultSize();
  printf("Program size: %d\n", vm.instructions_size);
  vm.execute();

  std::cout << "Results:\n";
  std::cout << "   Left: 0b" << std::bitset<64>(*(uint64_t *)(vm.registers)) << "\n";
  std::cout << "   Left: " << *(float *)(vm.registers) << "f\n";
  std::cout << "   Left: " << *(double *)(vm.registers) << "d\n";
  std::cout << "  Right: 0b" << std::bitset<64>(*(uint64_t *)(vm.registers + 8)) << "\n";
  std::cout << "  Right: " << *(float *)(vm.registers + 8) << "f\n";
  std::cout << "  Right: " << *(double *)(vm.registers + 8) << "d\n";

  return 0;
}