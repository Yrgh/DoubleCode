#ifndef _AST_CPP_
#define _AST_CPP_

#include <stdio.h>
#include <string>
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
  std::string name;
  std::vector<ASTType> tempargs;
  bool locked = false;
  int arrsize = 0; // Not an array

  ASTType() = default;
  ASTType(const ASTType &) = default;

  void print() const {
    if (locked) printf("lock ");
    printf("%.*s", name.length(), name.c_str());
    if (!tempargs.empty()) {
      printf("<");
      for (int i = 0; i < tempargs.size() - 1; ++i) {
        tempargs[i].print();
        printf(", ");
      }
      tempargs.back().print();
      printf(">");
    }
    if (arrsize > 0) {
      printf("[%d]", arrsize);
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
    expr->print(indent);
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
  ASTNode *init; // Either a CodeBlockNode (representing a set of parameters) or an expression

  ~VarDeclNode() {
    if (init) delete init;
  }

  VarDeclNode(ASTType &t, Token v_name, ASTNode *e) : 
    type(t),
    name(v_name),
    init(e)
  {}

  void print(int indent) const override {
    printIndent(indent);
    printf("var ");
    type.print();
    printf(" %.*s\n", name.length, name.start);

    if (init) init->print(indent + 1);
  }
};

class Parser {
  Lexer       lexer;
  Token       previous, current;
  bool        statementStatus       = true; // True = OK, False = Bad
  const char *source_code;

  void advance() {
    previous = current;
    current  = lexer.getNext();
  }

  void error(const char *message) const {
    printf("Parse Error: %s", message);
  }

  void logToken(const Token &tok) {
    printf("at:\n[line %d]: '%.*s'\n", tok.line, tok.length, tok.start);
  }

  bool expect(TokenType expected, const char *errmsg) {
    if (current.type == expected) {
      advance();
      return true;
    }
    statementStatus = false;
    error(errmsg);
    logToken(current);
    return false;
  }

  ASTType parseType() {
    ASTType result;
    if (current.type == TokenType::KEY_LOCK) {
      result.locked = true;
      advance();
    }

    if (current.type != TokenType::IDENTIFIER &&
      current.type != TokenType::SPEC_SHARED &&
      current.type != TokenType::SPEC_UNIQUE
    ) {
      error("Expected identifier at the beginning of type\n");
      logToken(current);
    }
    
    result.name = std::string(current.start, current.length);
    advance();

    if (
      (
        previous.type == TokenType::SPEC_SHARED ||
        previous.type == TokenType::SPEC_UNIQUE
      ) &&
      current.type == TokenType::LEFT_SQUARE
    ) {
      advance();
      expect(TokenType::RIGHT_SQUARE, "Expected '[]', but found '['\n");
      result.name.append("[]");
    }

    if (current.type == TokenType::LT) {
      advance();

      if (current.type == TokenType::GT) {
        advance();
        return result; // Empty ("<>")
      }

      while (current.type != TokenType::EOF_TOKEN) {

        result.tempargs.push_back(parseType());

        if (current.type == TokenType::GT) break;

        expect(TokenType::COMMA, "Expected ',' between template arguments\n");
      }
      advance();
    }

    return result;
  }

  bool skipType() {
    if (current.type == TokenType::KEY_LOCK) {
      advance();
    }

    if (current.type != TokenType::IDENTIFIER &&
      current.type != TokenType::SPEC_SHARED &&
      current.type != TokenType::SPEC_UNIQUE
    ) return false;
    
    advance();

    if (
      (
        previous.type == TokenType::SPEC_SHARED ||
        previous.type == TokenType::SPEC_UNIQUE
      ) &&
      current.type == TokenType::LEFT_SQUARE
    ) {
      advance();
      if (current.type != TokenType::RIGHT_SQUARE) return false;
      advance();
    }

    if (current.type == TokenType::LT) {
      advance();

      if (current.type == TokenType::GT) {
        advance();
        return true; // Empty ("<>")
      }
      
      while (current.type != TokenType::EOF_TOKEN) {

        if (!skipType()) return false;

        if (current.type == TokenType::GT) break;

        if (current.type != TokenType::COMMA) return false;
      }
      advance();
    }

    return true;
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

  ASTNode *parseExprBlock() {
    ExprBlockNode *exprblock = new ExprBlockNode;
    exprblock->type = parseType();

    // ':'
    advance();
    expect(TokenType::LEFT_CURLY, "Expected '{' in expr-block\n");

    bool state = statementStatus;
    while (
      current.type != TokenType::EOF_TOKEN &&
      current.type != TokenType::RIGHT_CURLY
    ) {
      statementStatus = true;
      ASTNode *node = parseStatement();
      if (statementStatus)
        exprblock->statements.push_back(node);
      else delete node;
    }
    statementStatus = state;

    expect(TokenType::RIGHT_CURLY, "Unterminated expr-block\n");
    return exprblock;
  }

  ASTNode *parsePrimary() {
    switch (current.type) {
      case TokenType::NUMBER: {
        advance();
        return new NumberNode(previous);
      }
      // They are just noteworthy identifiers
      case TokenType::SPEC_SHARED:
      case TokenType::SPEC_UNIQUE:
      case TokenType::IDENTIFIER: {
        // Try if it is a Expr-block or an identifier
        // - If we see an invalid template, it has to be an identifier (valid templates: "<x0, x1...>")
        // - If we see no template or a valid template, then check for ':'. It indicates an expr-block
        // - If we see a valid template, but no ':', something went wrong.

        // Capture the lexer's state
        Lexer lexstate = lexer;
        Token prev = previous, curr = current;

        #define REVERT { lexer = lexstate; previous = prev; current = curr; }
        #define PARSE_ID { return new IdentifierNode(current.start, current.length); }

        bool valid_type = skipType();
        
        if (valid_type && current.type == TokenType::COLON) {
          REVERT
          return parseExprBlock();
        }

        REVERT
        PARSE_ID

        #undef REVERT
        #undef PARSE_ID
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
    ASTNode *out;
    bool need_semi = true;
    switch (current.type) {
      case TokenType::KEY_IF: {
        advance();

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
        advance();
        
        out = new YieldNode(parseExpr());
      } break;
      case TokenType::LEFT_CURLY: {
        advance();
        
        CodeBlockNode *code = new CodeBlockNode;

        while (
          current.type != TokenType::EOF_TOKEN &&
          current.type != TokenType::RIGHT_CURLY
        ) {
          ASTNode *newstate = parseStatement();
          if (statementStatus)
            code->statements.push_back(newstate);
          else delete newstate;
          statementStatus = true;
        }

        expect(TokenType::RIGHT_CURLY, "Unterminated code block\n");

        out = code;
        need_semi = false;
      } break;
      case TokenType::KEY_VAR: {
        advance();
        
        Lexer lexstate = lexer;
        Token prev = previous, curr = current;
        if (!skipType()) {
          error("Expected identifier at the beginning of type\n");
          logToken(current);
        }
        lexer = lexstate;
        previous = prev;
        current = curr;
        
        ASTType type = parseType();

        if (current.type != TokenType::IDENTIFIER) {
          error("Expected name after type in variable declaration\n");
          break;
        }

        advance();

        if (current.type == TokenType::EQ) {
          Token name = previous;
          advance();
          out = new VarDeclNode(type, name, parseExpr());
        } else if (current.type == TokenType::LEFT_SQUARE) {
          Token name = previous;
          advance();

          CodeBlockNode *constructor = new CodeBlockNode;

          while (
            current.type != TokenType::EOF_TOKEN &&
            current.type != TokenType::RIGHT_SQUARE
          ) {
            ASTNode *newexpr = parseExpr();
            if (statementStatus)
              constructor->statements.push_back(newexpr);
            else delete newexpr;
            statementStatus = true;
          }

          expect(TokenType::RIGHT_SQUARE, "Unterminated construction block\n");

          out = new VarDeclNode(type, name, constructor);
        } else {
          out = new VarDeclNode(type, previous, nullptr);
        }
      } break;
      default:
        out = new DoExprNode(parseExpr());
        break;
        /*if (!statementHadBadDetect) {
          error("Expected statement\n");
          statementHadBadDetect = true;
        }
        continue;*/
    }
    if (need_semi)
      expect(TokenType::SEMI, "Expected ';' after statement\n");
    return out;

    printf("Parsing failed! Program could not stablize\n");
    statementStatus = false;
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
      else delete newstate;
      statementStatus = true;
    }
  }
};

#endif // _AST_CPP_