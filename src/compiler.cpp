#ifndef _COMPILER_CPP_
#define _COMPILER_CPP_

#include <string>
#include <unordered_map>
#include <vector>

#include "astparser.cpp"
#include "vm.cpp"

typedef byte type_t;

static bool strcontains(const char *big, int off, int len, char item) {
  const char *ptr = strchr(big + off, item);
  if (ptr == NULL)
    return false;
  return ptr < big + off + len;
}

static type_t stot(const char *name, int len) {
  if (len < 2 || len > 6)
    return TYPE_NONE;
  switch (*name) {
  case 'u':
    switch (name[1]) {
    case '8':
      if (len != 2)
        return TYPE_NONE; // Not u8
      return MERGE(TYPE_UNSIGNED, FROM_SIZE(8));
    case '1':
      if (len != 3 || name[2] != '6')
        return TYPE_NONE; // Not u16
      return MERGE(TYPE_UNSIGNED, FROM_SIZE(16));
    case '3':
      if (len != 3 || name[2] != '2')
        return TYPE_NONE; // Not u32
      return MERGE(TYPE_UNSIGNED, FROM_SIZE(32));
    case '6':
      if (len != 3 || name[2] != '4')
        return TYPE_NONE; // Not u64
      return MERGE(TYPE_UNSIGNED, FROM_SIZE(64));
    }
    return TYPE_NONE;
  case 'i':
    switch (name[1]) {
    case '8':
      if (len != 2)
        return TYPE_NONE; // Not i8
      return MERGE(TYPE_SIGNED, FROM_SIZE(8));
    case '1':
      if (len != 3 || name[2] != '6')
        return TYPE_NONE; // Not i16
      return MERGE(TYPE_SIGNED, FROM_SIZE(16));
    case '3':
      if (len != 3 || name[2] != '2')
        return TYPE_NONE; // Not i32
      return MERGE(TYPE_SIGNED, FROM_SIZE(32));
    case '6':
      if (len != 3 || name[2] != '4')
        return TYPE_NONE; // Not i64
      return MERGE(TYPE_SIGNED, FROM_SIZE(64));
    }
    return TYPE_NONE;
  case 'f':
    switch (name[1]) {
    case '3':
      if (len != 3 || name[2] != '2')
        return TYPE_NONE; // Not f32
      return MERGE(TYPE_FLOAT, FROM_SIZE(32));
    case '6':
      if (len != 3 || name[2] != '4')
        return TYPE_NONE; // Not f64
      return MERGE(TYPE_FLOAT, FROM_SIZE(64));
    }
    return TYPE_NONE;
  }
  return TYPE_NONE;
}

class Compiler {
  struct AddData {
    int32_t where; // Where to place it
    byte   *what;
    int     length;
  };

  template <class T>
  void addData(const T &d) {
    AddData ad;
    ad.where  = out_buf.size();
    T *buf    = new T[1];
    buf[0]    = d;
    ad.what   = (byte *)buf;
    ad.length = sizeof(T);
    emitNulls(4);
    add_data.push_back(ad);
  }

  bool isValidType(const char *string, int len) {
    return stot(string, len) != TYPE_NONE;
  }

  type_t getType(const Token &tok) {
    return stot(tok.start, tok.length);
  }

  void insertZero(type_t t) {
    switch (t) {
      case MERGE(TYPE_UNSIGNED, FROM_SIZE(8)): {
        uint8_t *x  = (uint8_t *)  (out_buf.data() + out_buf.size());
        emitNulls(1);
        *x = 0;
      } break;
      case MERGE(TYPE_UNSIGNED, FROM_SIZE(16)): {
        uint16_t *x = (uint16_t *) (out_buf.data() + out_buf.size());
        emitNulls(2);
        *x = 0;
      } break;
      case MERGE(TYPE_UNSIGNED, FROM_SIZE(32)): {
        uint32_t *x = (uint32_t *) (out_buf.data() + out_buf.size());
        emitNulls(4);
        *x = 0;
      } break;
      case MERGE(TYPE_UNSIGNED, FROM_SIZE(64)): {
        uint64_t *x = (uint64_t *) (out_buf.data() + out_buf.size());
        emitNulls(8);
        *x = 0;
      } break;
      case MERGE(TYPE_SIGNED, FROM_SIZE(8)): {
        uint8_t *x  = (uint8_t *)  (out_buf.data() + out_buf.size());
        emitNulls(1);
        *x = 0;
      } break;
      case MERGE(TYPE_SIGNED, FROM_SIZE(16)): {
        uint16_t *x = (uint16_t *) (out_buf.data() + out_buf.size());
        emitNulls(2);
        *x = 0;
      } break;
      case MERGE(TYPE_SIGNED, FROM_SIZE(32)): {
        uint32_t *x = (uint32_t *) (out_buf.data() + out_buf.size());
        emitNulls(4);
        *x = 0;
      } break;
      case MERGE(TYPE_SIGNED, FROM_SIZE(64)): {
        uint64_t *x = (uint64_t *) (out_buf.data() + out_buf.size());
        emitNulls(8);
        *x = 0;
      } break;
      case MERGE(TYPE_FLOAT, FROM_SIZE(32)): {
        float *x = (float *) (out_buf.data() + out_buf.size());
        emitNulls(4);
        *x = 0;
      } break;
      case MERGE(TYPE_FLOAT, FROM_SIZE(64)): {
        double *x = (double *) (out_buf.data() + out_buf.size());
        emitNulls(8);
        *x = 0;
      } break;
    }
  }

  void incorporateAddData() {
    for (std::pair<type_t, std::vector<int>> z : zeros) {
      printf("Handling zeros for type: %.2hhX\n", z.first);
      int32_t pos = out_buf.size();
      for (int i : z.second) {
        memcpy(out_buf.data() + i, &pos, 4);
      }

      insertZero(z.first);
    }

    while (!add_data.empty()) {
      AddData &ad  = add_data.back();
      int32_t  pos = out_buf.size();
      emitNulls(ad.length);
      memcpy(out_buf.data() + pos, ad.what, ad.length);
      memcpy(out_buf.data() + ad.where, &pos, 4);
      delete[] ad.what;
      add_data.pop_back();
      printf(
        "Overwrote %d - %d and %d - %d\n",
        pos, pos + ad.length, 
        ad.where, ad.where + 4
      );
    }
  }

  std::vector<AddData> add_data;
  std::vector<byte>    out_buf;

  std::vector<type_t> expr_block_types;
  std::vector<std::vector<int>> expr_block_extjmps;

  // Which types have zeros, and where should the data pointer be placed
  std::unordered_map<type_t, std::vector<int>> zeros;

  void emitByte(byte b) {
    out_buf.push_back(b);
  }

  void emitPair(byte b0, byte b1) {
    out_buf.push_back(b0);
    out_buf.push_back(b1);
  }

  void emitNulls(int num) {
    for (int i = 0; i < num; ++i) {
      out_buf.push_back(0);
    }
  }

  void emitZero(type_t of) {
    emitByte(OPCODE_LOADC);
    zeros[of]; // Neat trick to insert it if it doesn't exist
    zeros.at(of).push_back(out_buf.size()); // Add the current location to the locations for wheres
    emitNulls(4);
  }

  void convert(type_t from, type_t to) {
    if (from == to)
      return;
    emitByte(OPCODE_CONV);
    emitPair(from, to);
  }

  void tempStore(byte reg) {
    emitPair(OPCODE_PUSH, LOWER(reg));
  }

  void tempLoad(byte reg) {
    emitByte(OPCODE_SWAP);
    emitPair(OPCODE_POP, LOWER(reg));
  }

  byte processNum(Token tok) {
    emitByte(OPCODE_LOADC);
    char suffix = tok.start[tok.length - 1];
    if (
      suffix == 'f' || suffix == 'd' ||
      strcontains(tok.start, 0, tok.length, '.')) {
      // It's a float, but which one?
      double num = strtod(tok.start, NULL);
      if (
        suffix == 'f' ||
        suffix != 'd' &&
          abs(num) < 3.4028235677973366e+38 &&
          abs(num) > 1.175494351e-38) {
        // Congratulations! It's a float! I think...
        // It'll convert
        emitByte(FROM_SIZE(32));
        addData<float>(num);
        return MERGE(TYPE_FLOAT, FROM_SIZE(32));
      }
      emitByte(FROM_SIZE(64));
      addData<double>(num);
      return MERGE(TYPE_FLOAT, FROM_SIZE(64));
    } // It's an integer
    uint32_t num = strtoull(tok.start, NULL, 10);
    if (num < 1ull << 8) {
      emitByte(FROM_SIZE(8));
      addData<uint8_t>(num);
      return MERGE(TYPE_UNSIGNED, FROM_SIZE(8));
    }
    if (num < 1ull << 16) {
      emitByte(FROM_SIZE(16));
      addData<uint16_t>(num);
      return MERGE(TYPE_UNSIGNED, FROM_SIZE(16));
    }
    if (num < 1ull << 32) {
      emitByte(FROM_SIZE(32));
      addData<uint32_t>(num);
      return MERGE(TYPE_UNSIGNED, FROM_SIZE(32));
    }
    emitByte(FROM_SIZE(64));
    addData<uint64_t>(num);
    return MERGE(TYPE_UNSIGNED, FROM_SIZE(64));
  }

  type_t processUnary(type_t _type_in, TokenType op) {
    type_t type = _type_in; // Maybe the compiler doesn't like it?
    switch (op) {
      case TokenType::MINUS:
        if (UPPER(type) == TYPE_UNSIGNED) {
          type_t newt = MERGE(TYPE_SIGNED, LOWER(newt));
          convert(type, newt);
          type = newt;
        }

        emitPair(OPCODE_NEG, type);

        return type;
      case TokenType::EX:
        equalsZero(type);
        type = MERGE(TYPE_UNSIGNED, FROM_SIZE(8));
        emitByte(OPCODE_BNOT);
        return type;
      case TokenType::TILDE:
        emitPair(OPCODE_NOT, type);
        return type;
    }

    printf("Invalid unary operator!\n");
    return TYPE_NONE;
  }

  type_t processBinary(type_t _type_in, TokenType op) {
    type_t type = _type_in; // Same as above;
    switch (op) {
    case TokenType::PLUS:
      emitPair(OPCODE_ADD, type);
      return type;
    case TokenType::MINUS:
      emitPair(OPCODE_SUB, type);
      return type;
    case TokenType::STAR:
      emitPair(OPCODE_MUL, type);
      return type;
    case TokenType::SLASH:
      emitPair(OPCODE_DIV, type);
      return type;
    case TokenType::CAR:
      emitPair(OPCODE_XOR, type);
      return type;
    case TokenType::AMP:
      emitPair(OPCODE_AND, type);
      return type;
    case TokenType::PIP:
      emitPair(OPCODE_OR, type);
      return type;
    case TokenType::AMP_AMP:
      emitByte(OPCODE_BAND);
      return MERGE(TYPE_UNSIGNED, FROM_SIZE(8));
    case TokenType::PIP_PIP:
      emitByte(OPCODE_BOR);
      return MERGE(TYPE_UNSIGNED, FROM_SIZE(8));
    case TokenType::EQ_EQUAL:
      emitPair(OPCODE_CMPE, type);
      type = MERGE(TYPE_UNSIGNED, FROM_SIZE(8));
      return type;
    case TokenType::EX_EQUAL:
      emitPair(OPCODE_CMPE, type);
      type = MERGE(TYPE_UNSIGNED, FROM_SIZE(8));
      emitByte(OPCODE_BNOT);
      return type;
    case TokenType::GT:
      emitPair(OPCODE_CMPG, type);
      type = MERGE(TYPE_UNSIGNED, FROM_SIZE(8));
      return type;
    case TokenType::LT_EQUAL:
      emitPair(OPCODE_CMPG, type);
      type = MERGE(TYPE_UNSIGNED, FROM_SIZE(8));
      emitByte(OPCODE_BNOT);
      return type;
    case TokenType::LT:
      emitPair(OPCODE_CMPL, type);
      type = MERGE(TYPE_UNSIGNED, FROM_SIZE(8));
      return type;
    case TokenType::GT_EQUAL:
      emitPair(OPCODE_CMPL, type);
      type = MERGE(TYPE_UNSIGNED, FROM_SIZE(8));
      emitByte(OPCODE_BNOT);
      return type;
    }
    return type;
  }

  void equalsZero(type_t type) {
    if (type == MERGE(TYPE_UNSIGNED, FROM_SIZE(8))) return;
    tempStore(type);
    emitZero (type);
    tempLoad (type);
    emitPair(OPCODE_CMPE, type);
  }

  void evalCondition(ASTNode *node) {
    type_t cond = evalExpr(node);
    equalsZero(cond);
  }

#define CONDITION(type, name, source) \
  if (const type *name = dynamic_cast<const type *>(source))

  type_t evalExpr(const ASTNode *node_base) {
    if (node_base == nullptr) {
      printf("Null node encountered!\n");
      return TYPE_NONE;
    }

    CONDITION(NumberNode, num, node_base) {
      printf("Number: %.*s at %d\n", num->tok.length, num->tok.start, (int) out_buf.size());
      return processNum(num->tok);
    }

    CONDITION(IdentifierNode, id, node_base) {
      std::string name = std::string(id->id, id->length);
      return TYPE_NONE;
    }

    CONDITION(UnaryNode, unary, node_base) {
      type_t type = evalExpr(unary->expr);
      return processUnary(type, unary->op);
    }

    CONDITION(BinaryNode, binary, node_base) {
      type_t left = evalExpr(binary->left);
      tempStore(left);
      type_t right = evalExpr(binary->right);
      type_t best  = best_type(left, right);
      convert(right, best);
      tempLoad(left);
      convert(left, best);
      return processBinary(best, binary->op);
    }

    CONDITION(ExprBlockNode, exprblock, node_base) {
      type_t t = getType(exprblock->type);
      expr_block_types.push_back(t);
      expr_block_extjmps.emplace_back();
      for (ASTNode *node : exprblock->statements) {
        evalStatement(node);
      }
      expr_block_types.pop_back();
      for (int jmp : expr_block_extjmps.back()) {
        *(int32_t *) (out_buf.data() + jmp) = out_buf.size();
      }
      expr_block_extjmps.pop_back();
      return t;
    }

    printf("Invalid expression!\n");
    return TYPE_NONE;
  }

  void evalStatement(ASTNode *node_base) {
    if (node_base == nullptr) {
      printf("Null node encountered!\n");
      return;
    }

    CONDITION(CodeBlockNode, codeblock, node_base) {
      for (ASTNode *node : codeblock->statements) {
        evalStatement(node);
      }
    }

    CONDITION(IfElseNode, ifelse, node_base) {
      evalCondition(ifelse->cond);
      emitByte(OPCODE_JMPZ);
      int to_else_pos = out_buf.size();
      emitNulls(4);

      evalStatement(ifelse->left);

      // If there is an else block
      if (ifelse->right) {
        emitByte(OPCODE_JMP);
        int after_else_pos = out_buf.size();
        emitNulls(4);

        *(int32_t *) (out_buf.data() + to_else_pos) = out_buf.size();

        evalStatement(ifelse->right);

        *(int32_t *) (out_buf.data() + after_else_pos) = out_buf.size();
      } else {
        *(int32_t *) (out_buf.data() + to_else_pos) = out_buf.size();
      }
    }

    CONDITION(DoExprNode, doexpr, node_base) {
      evalExpr(doexpr->expr);
      return;
    }

    CONDITION(YieldNode, yield, node_base) {
      if (expr_block_types.empty()) printf("'yield' statement outside of expression-block\n");
      convert(evalExpr(yield->expr), expr_block_types.back());
      emitByte(OPCODE_JMP);
      expr_block_extjmps.back().push_back(out_buf.size());
      emitNulls(4);
      return;
    }
  }

  void evalTop(CodeBlockNode *code) {
    printf("Code block has %d statements\n", (int) code->statements.size());
    for (ASTNode *node_base : code->statements) {
      // Special nodes:

      // Otherwise:
      evalStatement(node_base);
    }
  }

#undef CONDITION
public:
  void compile(const char *source) {
    Parser parser;
    parser.parse(source);
    parser.top->print(0);
    evalTop(parser.top);
    emitByte(OPCODE_RETURN);
    printf("Program size (-data): %d\n", (int) out_buf.size());
    incorporateAddData();
  }

  const byte *getResultData() const {
    return out_buf.data();
  }
  int getResultSize() const {
    return out_buf.size();
  }
};

#endif // _COMPILER_CPP_