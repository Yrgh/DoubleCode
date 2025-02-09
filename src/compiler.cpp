#ifndef _COMPILER_CPP_
#define _COMPILER_CPP_

#include "vm.cpp"
#include "astparser.cpp"
#include "constpool.cpp"
#include <vector>
#include <iostream>

#define APPLY_UNDONE(dest, op, src) ((dest op##= src) op src)

#define CONSTANT_VAL_TYPE(name) (ASTType{#name, {}, true, 0})

static const ASTType VOID_TYPE = CONSTANT_VAL_TYPE(void);

static bool isPrimitive(const ASTType &type) {
  // Arrays aren't primitives. Primitives don't have arguments
  if (type.arrsize != 0 || type.tempargs.size() != 0) return false;
  // All of them are either 2 or 3
  if (type.name.length() > 3 || type.name.length() < 2) return false;

  if(type.name[0] != 'u' && type.name[0] != 'i' && type.name[0] != 'f') return false;

  std::string length = type.name.substr(1);

  if (length == "8" || length == "16" || length == "32" || length == "64") return true;
  return false;
}

static byte primitiveByte(const ASTType &type) {
  byte value;
  switch (type.name[0]) {
    case 'u':
      value = TYPE_UNSIGNED;
      break;
    case 'i':
      value = TYPE_SIGNED;
      break;
    case 'f':
      value = TYPE_FLOAT;
      break;
  }

  if (type.name.substr(1) == "8")  return MERGE(value, FROM_SIZE(8));
  if (type.name.substr(1) == "32") return MERGE(value, FROM_SIZE(32));
  if (type.name.substr(1) == "64") return MERGE(value, FROM_SIZE(64));
  return MERGE(value, FROM_SIZE(16));
}

static char bestPrimType(char l, char r) {
  switch (l) {
    case 'u':
      return r; // r is always going to be better or equal
    case 'i':
      if (r == 'f') return r; // If r is better...
      return l; // l is better or equal
    case 'f':
      return l; // l is always going to be better or equal
  }
  return 'u';
}

// TODO: Add precomputation (allows for declared constants, significant optimization)
class Compiler {
  std::vector<byte> result;
  std::unordered_map<int32_t, int32_t> constant_indexes;
  ConstantPool constants;

  int stack_global = 0;
  int stack_local  = 0;
  bool in_global = true;

  bool compile_fail;

  template <class T> void insertValue(T val) {
    T &ptr = *(T *)(result.data() + result.size());
    result.insert(result.end(), sizeof(T), 0);
    ptr = val;
  }

  // TODO: Add structure compatiblity
  template <class T> void insertConstant(T val) {
    result.push_back(OPCODE_LOADC);
    result.push_back(sizeof(val));
    constant_indexes.insert({result.size(), constants.addConstant<T>(val)});
    result.insert(result.end(), 4, 0);
  }

  // Returns the stack location of the value
  int modifyStack(int num) {
    if (in_global) return APPLY_UNDONE(stack_global, +, num);
    return APPLY_UNDONE(stack_local, +, num);
  }

  ASTType number(const std::string &str) {
    if (
      str.find('.') != std::string::npos ||
      str.find('f') != std::string::npos
    ) {
      insertConstant<float>(std::stof(str));
      return CONSTANT_VAL_TYPE(f32);
    }

    if (str.find('d') != std::string::npos) {
      insertConstant<double>(std::stod(str));
      return CONSTANT_VAL_TYPE(f64);
    }

    uint64_t val = std::stoull(str);
    
    if (val < 256) {
      insertConstant<uint8_t>(val);
      return {"u8", {}, true, 0};
    }

    if (val < 65536) {
      insertConstant<uint16_t>(val);
      return {"u16", {}, true, 0};
    }

    if (val < 4294967296) {
      insertConstant<uint32_t>(val);
      return {"u32", {}, true, 0};
    }

    insertConstant<uint64_t>(val);
    return {"u64", {}, true, 0};
  }

  ASTType promoteTypes(const ASTType &left, const ASTType &right) {
    bool primleft = isPrimitive(left), primright = isPrimitive(right);

    bool lock = left.locked || right.locked;

    if (primleft && primright) {
      char best = bestPrimType(left.name[0], right.name[0]);

      if (best != left.name[0] || best != right.name[0]) { // There is a better type
        if (best == left.name[0])  return left;
        return right;
      }

      uint8_t leftsize = std::stoi(left.name.substr(1)),
        rightsize = std::stoi(right.name.substr(1));
      return {std::string(1, best) + std::to_string(max(leftsize, rightsize)), {}, lock, 0};
    }

    printf("Compile error: Non-primitive type mismatch\n");
    compile_fail = true;
    return VOID_TYPE;
  }

  int32_t typeSize(const ASTType &type) {
    if (isPrimitive(type)) {
      return std::stoi(type.name.substr(1));
    }

    return 0;
  }

  ASTType compileBinaryOp(const BinaryNode *binop) {
    ASTType left = compileExpression(binop->left);
    byte primleft;

    if (isPrimitive(left)) {
      primleft = primitiveByte(left);
      result.push_back(OPCODE_PUSH);
      result.push_back(LOWER(primleft));
    } else {
      // TODO
    }

    ASTType right = compileExpression(binop->right);
    ASTType best = promoteTypes(left, right);
    byte primbest;

    // Since there are no implicit conversions, left and right are either the same or both primitives
    if (isPrimitive(best)) {
      primbest = primitiveByte(best);
      byte primright = primitiveByte(right);

      if (primright != primbest) {
        result.push_back(OPCODE_CONV);
        result.push_back(primright);
        result.push_back(primbest);
      }

      result.push_back(OPCODE_SWAP);

      result.push_back(OPCODE_POP);
      result.push_back(LOWER(primleft));

      if (primleft != primbest) {
        result.push_back(OPCODE_CONV);
        result.push_back(primleft);
        result.push_back(primbest);
      }

      switch (binop->op) {
        case TokenType::PLUS:
          result.push_back(OPCODE_ADD);
          result.push_back(primbest);
          break;
      }
    } else {
      // TODO
    }

    return best;
  }

  // --- Expressions --
  // NOTE: Primitive values are passed down through the left register, but objects are passed at the top of the stack

#define CHECK_IS_TYPE(oldname, newname, newtype) \
if (const newtype *newname = dynamic_cast<const newtype *>(oldname))

  ASTType compileExpression(const ASTNode *node) {
    CHECK_IS_TYPE(node, num, NumberNode) {
      return number(std::string(num->tok.start, num->tok.length));
    }

    CHECK_IS_TYPE(node, binop, BinaryNode) {
      return compileBinaryOp(binop);
    }

    return VOID_TYPE;
  }

  void compileStatement(const ASTNode *node) {
    CHECK_IS_TYPE(node, vardecl, VarDeclNode) {
      return;
    }

    compileExpression(node);
  }

#undef CHECK_IS_TYPE

  void finishResult() {
    result.push_back(OPCODE_RETURN);

    for (const std::pair<int32_t, int32_t> &replace : constant_indexes) {
      *(int32_t *)(result.data() + replace.first) = replace.second + result.size();
    }

    result.insert(result.end(), constants.storage.begin(), constants.storage.end());
  }

public:
  bool compile(const CodeBlockNode *top) {
    result.clear();
    result.reserve(8); // We need at least 8 bytes: a LOADC instruction (+1+4), a 1-bye constant, and a RETURN instruction
    
    for (const ASTNode *node : top->statements) {
      compile_fail = false;
      compileStatement(node);
      if (compile_fail) {
        result.clear();
        printf("Compilation failed with errors!\n");
        return false;
      }
    }
    
    finishResult();
    return true;
  }

  const byte *resultData() const {
    return result.data();
  }

  int resultSize() const {
    return result.size();
  }
};

#endif