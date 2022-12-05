#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
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

class FuncTypeAST : public BaseAST {
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

class BlockAST : public BaseAST {
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

class StmtAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;

  void Dump() const override {
    std::cout << "StmtAST { ";
    exp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override {
    exp->ret = true;

    std::pair<bool, int> result = exp->Output();

    if (result.first) {
      str += "  ret ";
      str += std::to_string(result.second);
      str += "\n";
    } else {
      str += "  ret %";
      str += std::to_string(cnt - 1);
      str += "\n";
    }

    return std::pair<bool, int>(false, 0);
  }
};

class ExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> addExp;

  void Dump() const override {
    std::cout << "ExpAST { ";
    addExp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override {
    std::pair<bool, int> result = addExp->Output();

    return result;
  }
};

class PrimaryExpWithBrAST : public BaseAST {
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

class PrimaryExpWithNumAST : public BaseAST {
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

class UnaryExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> primaryExp;

  void Dump() const override {
    std::cout << "UnaryExpAST { ";
    primaryExp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override {
    std::pair<bool, int> result = primaryExp->Output();
    return result;
  }
};

class UnaryExpWithOpAST : public BaseAST {
 public:
  char unaryOp;
  std::unique_ptr<BaseAST> unaryExp;

  void Dump() const override {
    std::cout << "UnaryExpWithOpAST { ";
    std::cout << "UnaryOpAST { " << unaryOp << " }";
    unaryExp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override {
    std::pair<bool, int> result = unaryExp->Output();

    if (result.first) {
      if (unaryOp == '!') {
        str += "  %";
        str += std::to_string(cnt);
        str += " = eq ";
        str += std::to_string(6);
        str += ", 0\n";
        cnt++;
      } else if (unaryOp == '-') {
        str += "  %";
        str += std::to_string(cnt);
        str += " = sub 0, ";
        str += std::to_string(6);
        str += "\n";
        cnt++;
      }
    } else {
      if (unaryOp == '!') {
        str += "  %";
        str += std::to_string(cnt);
        str += " = eq %";
        str += std::to_string(cnt - 1);
        str += ", 0\n";
        cnt++;
      } else if (unaryOp == '-') {
        str += "  %";
        str += std::to_string(cnt);
        str += " = sub 0, %";
        str += std::to_string(cnt - 1);
        str += "\n";
        cnt++;
      }
    }

    return std::pair<bool, int>(false, 0);
  }
};

class MulExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> unaryExp;

  void Dump() const override {
    std::cout << "MulExpAST { ";
    unaryExp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override { return unaryExp->Output(); }
};

class MulExpWithOpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> mulExp;
  char mulOp;
  std::unique_ptr<BaseAST> unaryExp;

  void Dump() const override {
    std::cout << "MulExpWithOpAST { ";
    mulExp->Dump();
    std::cout << "MulExpOpAST { " << mulOp << " }";
    unaryExp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override {
    std::pair<bool, int> result_l = mulExp->Output();
    std::pair<bool, int> result_r = unaryExp->Output();

    std::unordered_map<char, std::string> dic = {
        {'*', "mul"},
        {'/', "div"},
        {'%', "mod"},
    };

    if (result_l.first && result_r.first) {
      str += "  %";
      str += std::to_string(cnt);
      str += " = ";
      str += dic[mulOp];
      str += " ";
      str += std::to_string(result_l.second);
      str += ", ";
      str += std::to_string(result_r.second);
      str += "\n";
      cnt++;

    } else if (result_l.first) {
      str += "  %";
      str += std::to_string(cnt);
      str += " = ";
      str += dic[mulOp];
      str += " ";
      str += std::to_string(result_l.second);
      str += ", %";
      str += std::to_string(cnt - 1);
      str += "\n";
      cnt++;

    } else if (result_r.first) {
      str += "  %";
      str += std::to_string(cnt);
      str += " = ";
      str += dic[mulOp];
      str += " %";
      str += std::to_string(cnt - 1);
      str += ", ";
      str += std::to_string(result_r.second);
      str += "\n";
      cnt++;

    } else {
      str += "  %";
      str += std::to_string(cnt);
      str += " = ";
      str += dic[mulOp];
      str += " %";
      str += std::to_string(cnt - 1);
      str += ", %";
      str += std::to_string(cnt - 2);
      str += "\n";
      cnt++;
    }

    return std::pair<bool, int>(false, 0);
  }
};

class AddExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> mulExp;

  void Dump() const override {
    std::cout << "AddExpAST { ";
    mulExp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override { return mulExp->Output(); }
};

class AddExpWithOpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> addExp;
  char addOp;
  std::unique_ptr<BaseAST> mulExp;

  void Dump() const override {
    std::cout << "AddExpWithOpAST { ";
    addExp->Dump();
    std::cout << "AddExpOpAST { " << addOp << " }";
    mulExp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override {
    std::pair<bool, int> result_l = addExp->Output();
    std::pair<bool, int> result_r = mulExp->Output();

    std::unordered_map<char, std::string> dic = {
        {'+', "add"},
        {'-', "sub"},
    };

    if (result_l.first && result_r.first) {
      str += "  %";
      str += std::to_string(cnt);
      str += " = ";
      str += dic[addOp];
      str += " ";
      str += std::to_string(result_l.second);
      str += ", ";
      str += std::to_string(result_r.second);
      str += "\n";
      cnt++;

    } else if (result_l.first) {
      str += "  %";
      str += std::to_string(cnt);
      str += " = ";
      str += dic[addOp];
      str += " ";
      str += std::to_string(result_l.second);
      str += ", %";
      str += std::to_string(cnt - 1);
      str += "\n";
      cnt++;

    } else if (result_r.first) {
      str += "  %";
      str += std::to_string(cnt);
      str += " = ";
      str += dic[addOp];
      str += " %";
      str += std::to_string(cnt - 1);
      str += ", ";
      str += std::to_string(result_r.second);
      str += "\n";
      cnt++;

    } else {
      str += "  %";
      str += std::to_string(cnt);
      str += " = ";
      str += dic[addOp];
      str += " %";
      str += std::to_string(cnt - 1);
      str += ", %";
      str += std::to_string(cnt - 2);
      str += "\n";
      cnt++;
    }

    return std::pair<bool, int>(false, 0);
  }
};
