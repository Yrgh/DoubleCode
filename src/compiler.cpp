#include "vm.cpp"
#include "astparser.cpp"
#include "constpool.cpp"
#include <vector>

class Compiler {
  std::vector<byte> result;
  ConstantPool constants;

  void compileStatement(const ASTNode *node) {

  }

  void finishResult() {
    result.push_back(OPCODE_RETURN);

    result.insert(result.end(), constants.storage.begin(), constants.storage.end());
  }

public:
  void compile(const CodeBlockNode *top) {
    result.clear();
    result.reserve(8); // We need at least 8 bytes: a LOADC instruction (+1+4), a 1-bye constant, and a RETURN instruction

    for (const ASTNode *node : top->statements) {
      compileStatement(node);
    }
    
    finishResult();
  }

  const byte *resultData() const {
    return result.data();
  }

  int resultSize() const {
    return result.size();
  }
};