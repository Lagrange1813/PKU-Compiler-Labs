#pragma once
#include <string>
#include <iostream>
#include "koopa.h"

extern std::string str;
extern int cnt;

// 所有 AST 的基类
class BaseAST {
  public:
    bool ret = false;

    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;

    virtual std::pair<bool, int> Output() const = 0;
};

class CompUnitAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override {
        std::cout << "CompUnitAST { ";
        func_def->Dump();
    std::cout << " }";
  }

    std::pair<bool, int> Output() const override {
    func_def->Output();
    return std::pair<bool, int>(false, 0);
  }
};

class FuncDefAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump() const override {
      std::cout << "FuncDefAST { ";
      func_type->Dump();
      std::cout << ", " << ident << ", ";
      block->Dump();
      std::cout << " }";
    }

    std::pair<bool, int> Output() const override {
      str += "fun ";
      str += "@";
      str += ident;
      str += "(): ";
      func_type->Output();
      block->Output();

      return std::pair<bool, int>(false, 0);
    }
};

class FuncTypeAST: public BaseAST {
  public:
    std::string type;
    
    void Dump() const override {
      std::cout << "FuncTypeAST { ";
      std::cout << type;
      std::cout << " }";
    }

    std::pair<bool, int> Output() const override {
      if (type == "int") {
        str += "i32";
        str += " ";
      }

      return std::pair<bool, int>(false, 0);
    }
};

class BlockAST: public BaseAST {
  public:
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override {
      std::cout << "BlockAST { ";
      stmt->Dump();
      std::cout << " }";
    }

    std::pair<bool, int> Output() const override {
      str += "{\n";
      str += "\%entry:\n";
      stmt->Output();
      str += "}";

      return std::pair<bool, int>(false, 0);
    }
};

class StmtAST: public BaseAST {
  public:
    std::unique_ptr<BaseAST> exp;

    void Dump() const override {
      std::cout << "StmtAST { ";
      exp->Dump();
      std::cout << " }";
    }

    std::pair<bool, int> Output() const override {
      exp->ret = true;

      exp->Output();
      str += "  ret %";
      str += std::to_string(cnt - 1);
      str += "\n";

      return std::pair<bool, int>(false, 0);
    }
};

class ExpAST: public BaseAST {
  public:
    std::unique_ptr<BaseAST> unaryExp;

    void Dump() const override {
      std::cout << "ExpAST { ";
      unaryExp->Dump();
      std::cout << " }";
    }

    std::pair<bool, int> Output() const override {
      unaryExp->ret = this->ret;

      unaryExp->Output();

      return std::pair<bool, int>(false, 0);
    }
};

class PrimaryExpWithBrAST: public BaseAST {
  public:
    std::unique_ptr<BaseAST> exp;
    
    void Dump() const override {
      std::cout << "PrimaryExpWithBrAST { ";
      exp->Dump();
      std::cout << " }";
    }

    std::pair<bool, int> Output() const override {
      exp->Output();

      return std::pair<bool, int>(false, 0);
    }
};

class PrimaryExpWithNumAST: public BaseAST {
  public:
    int number; 

    void Dump() const override {
      std::cout << "PrimaryExpWithNumAST { ";
      std::cout << number;
      std::cout << " }";
    }

    std::pair<bool, int> Output() const override {
      return std::pair<bool, int>(true, number);
    }
};

class UnaryExpAST: public BaseAST {
  public:
    std::unique_ptr<BaseAST> primaryExp;

    void Dump() const override {
      std::cout << "UnaryExpAST { ";
      primaryExp->Dump();
      std::cout << " }";
    }

    std::pair<bool, int> Output() const override {
      primaryExp->Output();

      return std::pair<bool, int>(false, 0);
    }
};

class UnaryOpPlusAST: public BaseAST {
  public:
    char op;

    void Dump() const override {
      std::cout << "UnaryOpPlusAST { ";
      std::cout << op;
      std::cout << " }";
    }

    std::pair<bool, int> Output() const override {
      return std::pair<bool, int>(false, 0);
    }
};

class UnaryOpMinusAST: public BaseAST {
  public:
    char op;

    void Dump() const override {
      std::cout << "UnaryOpMinusAST { ";
      std::cout << op;
      std::cout << " }";
    }

    std::pair<bool, int> Output() const override {
      str += "  %1 = sub 0, %0\n";

      return std::pair<bool, int>(false, 0);
    }
};

class UnaryOpNotAST: public BaseAST {
  public:
    char op;

    void Dump() const override {
      std::cout << "UnaryOpNotAST { ";
      std::cout << op;
      std::cout << " }";
    }

    std::pair<bool, int> Output() const override {
      str += "  %0 = eq 6, 0\n";

      return std::pair<bool, int>(false, 0);
    }
};

class UnaryExpWithOpAST: public BaseAST {
  public:
    std::unique_ptr<BaseAST> unaryOp;
    std::unique_ptr<BaseAST> unaryExp;

    void Dump() const override {
      std::cout << "UnaryExpWithOpAST { ";
      unaryOp->Dump();
      unaryExp->Dump();
      std::cout << " }";
    }

    std::pair<bool, int> Output() const override {
      unaryOp->Output();
      unaryExp->Output();

      return std::pair<bool, int>(false, 0);
    }
};