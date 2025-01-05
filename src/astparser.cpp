#ifndef _AST_CPP_
#define _AST_CPP_

#include <stdio.h>

#include <vector>

#include "lexer.cpp"

static void printIndent(int indent) {
  for (int i = 0; i < indent; ++i) {
    printf("  ");
  }
}

static void printOp(TokenType op) {
  switch (op) {
  case TokenType::PLUS:
    printf("+");
    break;
  case TokenType::MINUS:
    printf("-");
    break;
  case TokenType::SLASH:
    printf("/");
    break;
  case TokenType::STAR:
    printf("*");
    break;
  case TokenType::PLUS_EQ:
    printf("+=");
    break;
  case TokenType::MINUS_EQ:
    printf("-=");
    break;
  case TokenType::SLASH_EQ:
    printf("/=");
    break;
  case TokenType::STAR_EQ:
    printf("*=");
    break;
  case TokenType::EX:
    printf("!");
    break;
  case TokenType::EQ:
    printf("=");
    break;
  case TokenType::EQ_EQUAL:
    printf("==");
    break;
  case TokenType::EX_EQUAL:
    printf("!=");
    break;
  case TokenType::GT:
    printf(">");
    break;
  case TokenType::GT_EQUAL:
    printf(">=");
    break;
  case TokenType::LT:
    printf("<");
    break;
  case TokenType::LT_EQUAL:
    printf("<=");
    break;
  case TokenType::CAR:
    printf("^");
    break;
  case TokenType::AMP:
    printf("&");
    break;
  case TokenType::PIP:
    printf("|");
    break;
  case TokenType::DOT:
    printf(".");
    break;
  default:
    printf("??: %hhu", op);
  }
}

struct ASTNode {
  virtual ~ASTNode() = default;
  virtual void print(int indent) const {
    printIndent(indent);
    printf("INVALID NODE!\n");
  }
};

struct ASTType {
  Token name;
  std::vector<ASTType> tempargs;
  bool locked;

  ASTType() = default;
  ASTType(const ASTType &) = default;

  void print() const {
    if (locked) printf("lock ");
    printf("%.*s", name.length, name.start);
    if (!tempargs.empty()) {
      printf("<");
      for (int i = 0; i < tempargs.size() - 1; ++i) {
        tempargs[i].print();
        printf(", ");
      }
      tempargs.back().print();
      printf(">");
    }
  }
};

struct NumberNode : ASTNode {
  Token tok;
  explicit NumberNode(Token tk) : tok(tk) {}

  void print(int indent) const override {
    printIndent(indent);
    printf("%.*s\n", tok.length, tok.start);
  }
};

struct BinaryNode : ASTNode {
  TokenType op;
  ASTNode  *left, *right;

  explicit BinaryNode(TokenType op, ASTNode *left, ASTNode *right) : left(left), right(right), op(op) {}

  ~BinaryNode() {
    if (left)
      delete left;
    if (right)
      delete right;
  }

  void print(int indent) const override {
    left->print(indent + 1);
    printIndent(indent);
    printOp(op);
    printf("\n");
    right->print(indent + 1);
  }
};

struct UnaryNode : ASTNode {
  TokenType op;
  ASTNode  *expr;

  explicit UnaryNode(TokenType op, ASTNode *expr) : expr(expr), op(op) {}

  ~UnaryNode() {
    if (expr)
      delete expr;
  }

  void print(int indent) const override {
    printIndent(indent);
    printOp(op);
    printf("\n");
    expr->print(indent + 1);
  }
};

struct IdentifierNode : ASTNode {
  const char *id;
  int         length;
  explicit IdentifierNode(const char *name, int len) : id(name), length(len) {}

  void print(int indent) const override {
    printIndent(indent);
    printf("%.*s\n", length, id);
  }
};

struct CodeBlockNode : ASTNode {
  std::vector<ASTNode *> statements;

  ~CodeBlockNode() {
    for (ASTNode *node : statements) {
      delete node;
    }
  }

  void print(int indent) const override {
    printIndent(indent);
    printf("{\n");
    for (const ASTNode *node : statements) {
      if (node == nullptr) {
        printIndent(indent + 1);
        printf("NULL NODE!\n");
      } else
        node->print(indent + 1);
    }
    printIndent(indent);
    printf("}\n");
  }
};

struct ExprBlockNode : ASTNode {
  std::vector<ASTNode *> statements;
  ASTType type;

  ~ExprBlockNode() {
    for (ASTNode *node : statements) {
      delete node;
    }
  }

  void print(int indent) const override {
    printIndent(indent);
    type.print();
    printf(" : {\n");
    for (const ASTNode *node : statements) {
      if (node == nullptr) {
        printIndent(indent + 1);
        printf("NULL NODE!\n");
      } else
        node->print(indent + 1);
    }
    printIndent(indent);
    printf("}\n");
  }
};

struct DoExprNode : ASTNode {
  ASTNode *expr;

  DoExprNode(ASTNode *e) : expr(e) {}

  ~DoExprNode() {
    delete expr;
  }

  void print(int indent) const override {
    printIndent(indent);
    printf("do\n");
    expr->print(indent + 1);
  }
};

struct YieldNode : ASTNode {
  ASTNode *expr;

  YieldNode(ASTNode *node) : expr(node) {}

  ~YieldNode() {
    delete expr;
  }

  void print(int indent) const override {
    printIndent(indent);
    printf("yield\n");
    expr->print(indent + 1);
  }
};

struct IfElseNode : ASTNode {
  ASTNode *cond,
      *left,  // The "if" part
      *right; // The "else" part

  ~IfElseNode() {
    if (cond)
      delete cond;
    if (left)
      delete left;
    if (right)
      delete right;
  }

  void print(int indent) const override {
    printIndent(indent);
    printf("If\n");
    cond->print(indent + 1);
    printIndent(indent);
    printf("Then\n");
    left->print(indent + 1);
    if (right) {
      printIndent(indent);
      printf("Then\n");
      right->print(indent + 1);
    }
  }
};

struct VarDeclNode : ASTNode {
  ASTType type;
  Token name;
  ASTNode *expr;

  ~VarDeclNode() {
    if (expr) delete expr;
  }

  VarDeclNode(ASTType &t, Token v_name, ASTNode *e) : 
    type(t),
    name(v_name),
    expr(e)
  {}

  void print(int indent) const override {
    printIndent(indent);
    printf("var ");
    type.print();
    printf(" %.*s\n", name.length, name.start);

    if (expr) expr->print(indent + 1);
  }
};

class Parser {
  Lexer       lexer;
  Token       previous, current;
  int         silence               = 0;
  bool        statementStatus       = true; // True = OK, False = Bad
  bool        statementHadBadDetect = false;
  const char *source_code;
  int depth = 0;

  void advance() {
    previous = current;
    current  = lexer.getNext();
  }

  void error(const char *message) const {
    if (silence < 1)
      printf("Parse Error (depth: %d): %s", depth, message);
  }

  void logToken(const Token &tok) {
    printf("at:\n[line %d]: '%.*s'\n", tok.line, tok.length, tok.start);
  }

  bool expect(TokenType expected, const char *errmsg) {
    if (current.type == expected) {
      advance();
      return true;
    }
    if (silence > 0)
      return false;
    error(errmsg);
    logToken(current);
    return false;
  }

  void quiet() {
    silence++;
  }

  void unquiet() {
    silence--;
  }

  ASTType parseType() {
    ASTType result;
    if (current.type == TokenType::KEY_LOCK) {
      result.locked = true;
      advance();
    }

    result.name = current;
    advance();

    if (current.type == TokenType::LT) {
      advance();

      while (current.type != TokenType::EOF_TOKEN) {
        result.tempargs.push_back(parseType());

        if (current.type == TokenType::GT) break;

        expect(TokenType::COMMA, "Expected ',' between template arguments\n");
      }
      advance();
    }
  }

  int getPrec(TokenType type) {
    switch (type) {
      case TokenType::COMMA:
        return 0;
      case TokenType::EQ:
      case TokenType::SLASH_EQ:
      case TokenType::STAR_EQ:
      case TokenType::PLUS_EQ:
      case TokenType::MINUS_EQ:
        return 1;
      case TokenType::PIP_PIP:
        return 2;
      case TokenType::AMP_AMP:
        return 3;
      case TokenType::PIP:
        return 4;
      case TokenType::CAR:
        return 5;
      case TokenType::AMP:
        return 6;
      case TokenType::EQ_EQUAL:
      case TokenType::EX_EQUAL:
        return 7;
      case TokenType::GT:
      case TokenType::LT:
      case TokenType::GT_EQUAL:
      case TokenType::LT_EQUAL:
        return 8;
      case TokenType::PLUS:
      case TokenType::MINUS:
        return 9;
      case TokenType::STAR:
      case TokenType::SLASH:
        return 10;
      case TokenType::DOT:
        return 11;
      default:
        return -1;
    }
  }

  ASTNode *parsePrimary() {
    switch (current.type) {
      case TokenType::NUMBER: {
        advance();
        return new NumberNode(previous);
      }
      case TokenType::IDENTIFIER: {
        // The Dilemma: Is it a type or a name? 
        advance();

        // Type : {/*block*/}
        /*if (current.type == TokenType::COLON) {
          ExprBlockNode *block = new ExprBlockNode;
          block->type = previous;
          advance(); // Now we can consume the colon token

          expect(TokenType::LEFT_CURLY, "Expected '{'\n");

          while (
              current.type != TokenType::RIGHT_CURLY &&
              current.type != TokenType::EOF_TOKEN) {
            ASTNode *newstate = parseStatement();
            if (statementStatus)
              block->statements.push_back(newstate);
            statementStatus = true;
          }

          if (current.type == TokenType::EOF_TOKEN)
            error("Unterminated expression block\n");
          else
            advance(); // Only stops for two reasons
          return block;
        }
        // If it ain't a block type, it be a variable name
        return new IdentifierNode(previous.start, previous.length);*/
      }
      case TokenType::LEFT_ROUND: {
        advance();
        ASTNode *result = parseExpr();
        expect(TokenType::RIGHT_ROUND, "Expected ')'\n");
        return result;
      }
      case TokenType::KEY_IF: {
      }
      // Unary operators
      case TokenType::EX:
      case TokenType::TILDE:
      case TokenType::MINUS: {
        advance();
        TokenType op = previous.type;
        return new UnaryNode(op, parsePrimary());
      }
      default:
        error("Invalid expression!\n");
        statementStatus = false;
        return nullptr;
      }
  }

  ASTNode *parseBinaryRHS(int min_prec, ASTNode *lhs) {
    while (true) {
      int op_prec = getPrec(current.type);
      if (op_prec < min_prec)
        break;

      Token op = current;
      advance();

      ASTNode *rhs       = parsePrimary();
      int      next_prec = getPrec(current.type);
      if (op_prec < next_prec) {
        rhs = parseBinaryRHS(op_prec + 1, rhs);
      }

      lhs = new BinaryNode(op.type, lhs, rhs);
    }
    return lhs;
  }

  ASTNode *parseExpr() {
    ASTNode *left = parsePrimary();
    return parseBinaryRHS(0, left);
  }

  ASTNode *parseStatement() {
    depth++;
    while (current.type != TokenType::EOF_TOKEN) {
      ASTNode *out;
      advance();
      bool need_semi = true;
      switch (previous.type) {
        case TokenType::KEY_DO:
          out = new DoExprNode(parseExpr());
          break;
        case TokenType::KEY_IF: {
          IfElseNode *ifelse = new IfElseNode();

          expect(TokenType::LEFT_ROUND, "Expected '(' after 'if'\n");
          ifelse->cond = parseExpr();
          expect(TokenType::RIGHT_ROUND, "Expected ')' after if condition\n");

          ifelse->left = parseStatement();
          need_semi = false;
          if (current.type == TokenType::KEY_ELSE) {
            advance();
            ifelse->right = parseStatement();
          }
          out = ifelse;
        } break;
        case TokenType::KEY_YIELD: {
          out = new YieldNode(parseExpr());
        } break;
        case TokenType::LEFT_CURLY: {
          CodeBlockNode *code = new CodeBlockNode;

          while (
            current.type != TokenType::EOF_TOKEN &&
            current.type != TokenType::RIGHT_CURLY
          ) {
            ASTNode *newstate = parseStatement();
            if (statementStatus)
              code->statements.push_back(newstate);
            statementStatus = true;
          }

          if (current.type == TokenType::EOF_TOKEN)
            error("Unterminated code block\n");
          else
            advance(); // Only stops for two reasons

          out = code;
          need_semi = false;
        } break;
        case TokenType::KEY_VAR: {
          if (current.type != TokenType::IDENTIFIER) {
            error("Expected type after variable declaration\n");
            statementHadBadDetect = true;
            continue;
          }
          
          ASTType type = parseType();

          if (current.type != TokenType::IDENTIFIER) {
            error("Expected name after type in variable declaration\n");
            statementHadBadDetect = true;
            continue;
          }

          advance();

          if (current.type == TokenType::EQ) {
            Token name = previous;
            advance();
            out = new VarDeclNode(type, name, parseExpr());
          } else {
            out = new VarDeclNode(type, previous, nullptr);
          }
        } break;
        default:
          if (!statementHadBadDetect) {
            error("Expected statement\n");
            statementHadBadDetect = true;
          }
          continue;
      }
      statementHadBadDetect = false;
      if (need_semi)
        expect(TokenType::SEMI, "Expected ';' after statement\n");
      depth--;
      return out;
    }

    printf("Parsing failed! Program could not stablize\n");
    statementStatus       = false;
    statementHadBadDetect = false;
    depth--;
    return nullptr;
  }

public:
  CodeBlockNode *top;

  void parse(const char *source) {
    source_code = source;
    lexer.init(source);

    advance();

    // Equivalent to a program...
    //   as long as you assume the top node has program ability (eg. functions)
    top = new CodeBlockNode;

    while (current.type != TokenType::EOF_TOKEN) {
      ASTNode *newstate = parseStatement();
      if (statementStatus)
        top->statements.push_back(newstate);
      statementStatus = true;
    }
  }
};

#endif // _AST_CPP_