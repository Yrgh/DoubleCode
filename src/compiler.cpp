#ifndef _COMPILER_CPP_
#define _COMPILER_CPP_

#include "vm.cpp"
#include "astparser.cpp"
#include "constpool.cpp"
#include <vector>
#include <iostream>

#define ADD_UNDONE(dest, src) ((dest += src) - src)

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

  struct VarInfo {
    bool is_global;
    bool is_prim;
    byte prim; // Optional
    int location;
    int size;
    ASTType type;
  };

  std::unordered_map<std::string, VarInfo> variables;
  std::vector<std::string> global_stack;
  std::vector<std::vector<std::string>> local_stack;

  struct ExprBlockInfo {
    std::vector<int> jump_inserts;
    const ExprBlockNode *block;

    ExprBlockInfo(const ExprBlockNode *ptr) :
      jump_inserts(),
      block(ptr)
    {}
  };

  std::vector<ExprBlockInfo> expr_blocks;

  int stack_global = 0;
  int stack_local  = 0;
  bool is_global = true;

  bool compile_fail;

  int emitPush(byte reg) {
    result.push_back(OPCODE_PUSH);
    result.push_back(reg);
    if (is_global) return ADD_UNDONE(stack_global, LOWER(reg));
    return ADD_UNDONE(stack_local, LOWER(reg));
  }

  void emitPop(byte reg) {
    result.push_back(OPCODE_POP);
    result.push_back(reg);
    if (is_global) stack_global -= LOWER(reg);
    else stack_local -= LOWER(reg);
  }

  int emitReserve(int size) {
    result.push_back(OPCODE_RESERVE);
    insertValue<int16_t>(size);
    if (is_global) return ADD_UNDONE(stack_global, size);
    return ADD_UNDONE(stack_local, size);
  }

  void emitRelease(int size) {
    result.push_back(OPCODE_RELEASE);
    insertValue<int16_t>(size);
    if (is_global) stack_global -= size;
    else stack_local -= size;
  }

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
    if (type.ref) return 8;
    
    if (isPrimitive(type)) {
      return std::stoi(type.name.substr(1)) / 8;
    }
    
    return 0;
  }

  void derefPrim(ASTType &type, byte p) {
    if (!type.ref) return;
    type.ref = false;

    result.push_back(OPCODE_LOAD);
    result.push_back(LOWER(p));
  }
  
  void applyOpPrimitive(TokenType op, byte type) {
    switch (op) {
      case TokenType::PLUS_EQ:
      case TokenType::PLUS:
        result.push_back(OPCODE_ADD);
        result.push_back(type);
        break;
    }
  }

  ASTType compileAssignOp(const BinaryNode * binop) {
    ASTType left = compileExpression(binop->left);
    byte primleft;
    if (!left.ref || left.locked) {
      printf("Expected an unlocked reference on the left of assignment operator");
      compile_fail = true;
      return VOID_TYPE;
    }
    
    bool isprim = isPrimitive(left);

    int location;

    if (isprim) {
      primleft = primitiveByte(left);
      location = emitPush(sizeof(void *));
    } else {

    }

    ASTType right = compileExpression(binop->right);

    if (isprim) {
      if (!isPrimitive(right)) {
        printf("Object assigned to primitive\n");
        compile_fail = true;
        return VOID_TYPE;
      }

      byte primright = primitiveByte(right);
      derefPrim(right, primright);

      if (primleft != primright) {
        result.push_back(OPCODE_CONV);
        result.push_back(primright);
        result.push_back(primleft);
      }

      result.push_back(OPCODE_SWAP);
      
      // For example, +=
      if (binop->op != TokenType::EQ) {
        // Step 1 is to get the pointer without popping the stack
        if (is_global) {
          result.push_back(OPCODE_SPP);
        } else {
          result.push_back(OPCODE_FPP);
        }
        insertValue<int32_t>(location);

        result.push_back(OPCODE_LOAD);
        result.push_back(sizeof(void *));

        result.push_back(OPCODE_LOAD);
        result.push_back(LOWER(primleft));

        applyOpPrimitive(binop->op, primleft);

        result.push_back(OPCODE_SWAP);
      }

      emitPop(sizeof(void *));

      result.push_back(OPCODE_STORE);

      // This should keep the pointer in the left register, so we don't need to do anything else to return the reference
    } else {
      if (left != right) {
        printf("Type mismatch in assignment operator\n");
        compile_fail = true;
        return VOID_TYPE;
      }
    }

    // We still want reference and unlocked

    return left;
  }

  ASTType compileBinaryOp(const BinaryNode *binop) {
    ASTType left = compileExpression(binop->left);
    byte primleft;

    if (isPrimitive(left)) {
      primleft = primitiveByte(left);
      derefPrim(left, primleft);
      emitPush(LOWER(primleft));
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
      derefPrim(right, primright);

      if (primright != primbest) {
        result.push_back(OPCODE_CONV);
        result.push_back(primright);
        result.push_back(primbest);
      }

      result.push_back(OPCODE_SWAP);

      emitPop(LOWER(primleft));

      if (primleft != primbest) {
        result.push_back(OPCODE_CONV);
        result.push_back(primleft);
        result.push_back(primbest);
      }

      applyOpPrimitive(binop->op, primbest);
    } else {
      // TODO
    }

    best.locked = true;
    return best;
  }

  void compileVarDecl(const VarDeclNode *vardecl) {
    std::string name = tokenToString(vardecl->name);
      
    if (variables.count(name) > 0) {
      printf("Variable already exists\n");
      compile_fail = true;
      return;
    }

    VarInfo &info = variables[name];

    info.type = vardecl->type;
    info.is_global = is_global;
    info.size = typeSize(vardecl->type);

    if (is_global) {
      global_stack.push_back(name);
    } else {
      local_stack.back().push_back(name);
    }

    if (info.type.ref) {
      if (!vardecl->init) {
        printf("References must be initialized\n");
        compile_fail = true;
        return;
      }

      ASTType res = compileExpression(vardecl->init);
      
      // Make sure the types line up...

      if (res != info.type) {
        printf("Conflicting reference and initializer\n");
        compile_fail = true;
        return;
      }
      
      if (!res.ref) {
        printf("References must be initialized with a reference\n");
        compile_fail = true;
        return;
      }

      // We actually don't care about the data, just the pointer.
      
      info.location = emitPush(sizeof(void *));

      return;
    }

    if (isPrimitive(vardecl->type)) {
      byte prim = primitiveByte(vardecl->type);

      info.is_prim = true;
      info.prim = prim;

      if (vardecl->init) {
        ASTType res = compileExpression(vardecl->init);

        if (!isPrimitive(res)) {
          printf("Assigning non-primitive to primitive value\n");
          compile_fail = true;
          return;
        }

        byte resprim = primitiveByte(res);
        derefPrim(res, resprim);

        if (res != vardecl->type) {
          // Convert the result
          result.push_back(OPCODE_CONV);
          result.push_back(resprim);
          result.push_back(prim);
        }
      } else {
        switch (UPPER(prim)) {
          case TYPE_SIGNED:
          case TYPE_UNSIGNED:
            number("0");
            break;
          case TYPE_FLOAT:
            if (LOWER(prim) == FROM_SIZE(32)) number("0f");
            else number("0d");
            break;
        }
      }

      // Either way, we push it to the stack
     info.location = emitPush(LOWER(prim));
    } else {
      info.is_prim = false;
    }
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
      if (binop->op >= TokenType::EQ) return compileAssignOp(binop);
      return compileBinaryOp(binop);
    }

    CHECK_IS_TYPE(node, id, IdentifierNode) {
      std::string name = tokenToString(id->tok);
      const VarInfo &info = variables.at(name);
      ASTType new_type = info.type;
      
      if (info.is_global)
        result.push_back(OPCODE_SPP);
      else
        result.push_back(OPCODE_FPP);
      insertValue<int32_t>(info.location);

      if (info.type.ref) {
        if (info.is_prim) {
          result.push_back(OPCODE_LOAD);
          result.push_back(LOWER(info.prim));
        }
      } else
        new_type.ref = true;

      return new_type;
    }

    CHECK_IS_TYPE(node, eb, ExprBlockNode) {
      expr_blocks.emplace_back(eb);

      for (const ASTNode *node : eb->statements) {
        compileStatement(node);
      }

      for (int pos : expr_blocks.back().jump_inserts) {
        *(int32_t *)(result.data() + pos) = result.size();
      }
      
      expr_blocks.pop_back();
      
      return eb->type;
    }

    return VOID_TYPE;
  }

  void compileStatement(const ASTNode *node) {
    CHECK_IS_TYPE(node, vardecl, VarDeclNode) {
      compileVarDecl(vardecl);

      return;
    }

    CHECK_IS_TYPE(node, yld, YieldNode) {
      if (expr_blocks.empty()) {
        printf("Cannot use yield outside of expression-block\n");
        compile_fail = true;
        return;
      }

      ExprBlockInfo &info = expr_blocks.back();

      bool isprim = isPrimitive(info.block->type);
      byte prim;

      if (isprim) {
        prim = primitiveByte(info.block->type);
      }

      ASTType res = compileExpression(yld->expr);

      if (res != info.block->type) {
        if (isprim && isPrimitive(res)) {
          byte primres = primitiveByte(res);

          result.push_back(OPCODE_CONV);
          result.push_back(primres);
          result.push_back(prim);
        } else {
          printf("Yield type mismatch\n");
          compile_fail = true;
          return;
        }
      }

      // NOTE: TODO NOW
      if (isprim) {
        
      } else {
        
      }

      return;
    }

    compileExpression(node);
    result.push_back(OPCODE_PRINT); // NOTE: Remove this
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
    result.reserve(32);
    stack_global = 0;
    stack_local = 0;

    for (const ASTNode *node : top->statements) {
      compile_fail = false;
      compileStatement(node);
      if (compile_fail) {
        result.clear();
        printf("Compilation failed with errors!\n");
        return false;
      }
    }

    while (!global_stack.empty()) {
      const std::string &name = global_stack.back();
      VarInfo &info = variables[name];

      if (info.type.ref) {
        emitPop(sizeof(void *));
      } else if (info.is_prim) {
        emitPop(LOWER(info.prim));
      } else {
        emitRelease(info.size);
      }

      global_stack.pop_back();
      // Not neccessary?
      //variables.erase(name);
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