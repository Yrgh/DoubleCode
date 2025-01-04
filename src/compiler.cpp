#include "astparser.cpp"
#include <string>
#include <vector>

struct IRTypeInfo {
  bool locked;
  std::string base;
  std::vector<IRTypeInfo> params;

  void print() const {
    if (locked) printf("locked ");
    printf("%.*s", (int) base.length(), base.c_str());
    if (!params.empty()) {
      printf("<");
      for (int i = 0; i < params.size() - 1; ++i) {
        params[i].print();
        printf(", ")
      }
      params.back().print();
      printf(">");
    }
  }
};

struct IRNode {
  virtual ~IRNode() {}
  virtual void print(int indent) const {
    printIndent(indent);
    printf("INVALID IR!\n");
  }
};

enum class BinOpType {
  ADD,
  SUB,
  MUL,
  DIV,
  BITAND,
  BITOR,
  BITXOR,
};

struct IRBinaryOp : IRNode {
  IRNode *left, *right;
  IRTypeInfo leftType, rightType, resultType;
  BinOpType op;

  ~IRBinaryOp() {
    delete left;
    delete right;
  }

  void print(int indent) const override {
    left->print(indent + 1);
    printIndent(indent);
    switch (op) {
      case BinOpType::ADD:
        printf("+\n");
        break;
      case BinOpType::SUB:
        printf("-\n");
        break;
      case BinOpType::MUL:
        printf("*\n");
        break;
      case BinOpType::DIV:
        printf("/\n");
        break;
      case BinOpType::BITAND:
        printf("&\n");
        break;
      case BinOpType::BITOR:
        printf("|\n");
        break;
      case BinOpType::BITXOR:
        printf("^\n");
        break;
    }
    right->print(indent + 1);
  }
};

enum class UnOpType {
  NEG,
  INV,
  NOT,
};

struct IRUnaryOp : IRNode {
  IRNode *expr;
  IRTypeInfo inType, outType;
  UnOpType op;

  ~IRUnaryOp() {
    delete expr;
  }

  void print(int indent) const override {
    printIndent(indent);
    switch (op) {
      case UnOpType::NEG:
        printf("-\n");
        break;
      case UnOpType::INV:
        printf("~\n");
        break;
      case UnOpType::NOT:
        printf("!\n");
        break;
    }
    expr->print(indent + 1);
  }
}

enum class CondOpType {
  EQUALS,
  NEQUALS,
  LESS,
  NLESS, // >=
  GREAT,
  NGREAT, // <=
  OR,
  AND,
};

struct IRBinaryOp : IRNode {
  IRNode *left, *right;
  IRTypeInfo leftType, rightType;
  CondOpType op;

  ~IRBinaryOp() {
    delete left;
    delete right;
  }

  void print(int indent) const override {
    left->print(indent + 1);
    printIndent(indent);
    switch (op) {
      case CondOpType::EQUALS:
        printf("==\n");
        break;
      case CondOpType::NEQUALS:
        printf("!=\n");
        break;
      case CondOpType::LESS:
        printf("<\n");
        break;
      case CondOpType::NLESS:
        printf(">=\n");
        break;
      case CondOpType::GREAT:
        printf(">\n");
        break;
      case CondOpType::NGREAT:
        printf("<=\n");
        break;
      case CondOpType::OR:
        printf("||\n");
        break;
      case CondOpType::AND:
        printf("&&\n");
        break;
    }
    right->print(indent + 1);
  }
};

struct IRVarDecl : IRNode {
  std::string name
  IRTypeInfo type;

  IRNode *expr;

  ~IRVarDecl() {
    delete expr;
  }

  void print(int indent) const override {
    printIndent(indent);
    printf("var %.*s : ", name.length(), name.c_str());
    type.print();
    printf(" = \n");
    expr->print(indent + 1);
  }
};

struct IRNullExpr : IRNode {
  IRTypeInfo type;

  void print(int indent) const override {
    printIndent(indent);
    printf("(");
    type.print();
    printf(") (0)\n");
  }
};

struct IRDestination : IRNode {
  void print(int indent) const override {
    printIndent(indent);
    printf("INVALID DESTINATION\n");
  }
};

struct IRIdentifierAccess : IRNode {
  std::string name;
  IRTypeInfo type;

  void print(int indent) const override {
    printIndent(indent)
    printf("%.*s (", name.length(), name.c_str());
    type.print();
    printf(")\n");
  }
};

struct IRSubAccess : IRNode {
  IRDestination *origin;
  std::string field;
  IRTypeInfo type;

  ~IRSubAccess() {
    delete origin;
  }

  void print(int indent) const override {
    origin->print(indent + 1);
    printf(".%.*s (", field.length(), field.c_str());
    type.print();
    printf(")\n");
  }
};

struct IRAssignExpr : IRNode {
  IRDestination *left;
  IRNode *right;

  ~IRAssignExpr() {
    delete left;
    delete right;
  }

  void print(int indent) const override {
    left->print(indent + 1);
    printf("=\n");
    right->print(indent + 1);
  }
};

struct IRCodeBlock : IRNode {
  std::vector<IRNode *> statements;

  ~IRCodeBlock() {
    for (IRNode *node : statements) {
      delete node;
    }
  }

  void print(int indent) const override {
    for (IRNode *node : statements) {
      node->print(indent + 1);
    }
  }
};

class Compiler {
  IRNode *compileStatement(const ASTNode *node_base) {

  }

public:
  IRCodeBlock *top;

  void compile(CodeBlockNode *root) {
    top = new IRCodeBlock;
    
    for (const ASTNode *node : root) {
      top->statements.push_back(compileStatement(node));
    }
  }
};

