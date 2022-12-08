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
  std::unique_ptr<BaseAST> lOrExp;

  void Dump() const override {
    std::cout << "ExpAST { ";
    lOrExp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override {
    std::pair<bool, int> result = lOrExp->Output();

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
    int cnt_l = cnt - 1;

    std::pair<bool, int> result_r = unaryExp->Output();
    int cnt_r = cnt - 1;

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
      str += std::to_string(cnt_r);
      str += "\n";
      cnt++;

    } else if (result_r.first) {
      str += "  %";
      str += std::to_string(cnt);
      str += " = ";
      str += dic[mulOp];
      str += " %";
      str += std::to_string(cnt_l);
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
      str += std::to_string(cnt_l);
      str += ", %";
      str += std::to_string(cnt_r);
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
    int cnt_l = cnt - 1;

    std::pair<bool, int> result_r = mulExp->Output();
    int cnt_r = cnt - 1;

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
      str += std::to_string(cnt_r);
      str += "\n";
      cnt++;

    } else if (result_r.first) {
      str += "  %";
      str += std::to_string(cnt);
      str += " = ";
      str += dic[addOp];
      str += " %";
      str += std::to_string(cnt_l);
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
      str += std::to_string(cnt_l);
      str += ", %";
      str += std::to_string(cnt_r);
      str += "\n";
      cnt++;
    }

    return std::pair<bool, int>(false, 0);
  }
};

class RelExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> addExp;

  void Dump() const override {
    std::cout << "RelExpAST { ";
    addExp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override { return addExp->Output(); }
};

class RelExpWithOpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> relExp;
  std::string relOp;
  std::unique_ptr<BaseAST> addExp;

  void Dump() const override {
    std::cout << "RelExpWithOpAST { ";
    relExp->Dump();
    std::cout << "RelExpOpAST { " << relOp << " }";
    addExp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override {
    std::pair<bool, int> result_l = relExp->Output();
    int cnt_l = cnt - 1;

    std::pair<bool, int> result_r = addExp->Output();
    int cnt_r = cnt - 1;

    std::unordered_map<std::string, std::string> dic = {
        {"<", "lt"},
        {">", "gt"},
        {"<=", "le"},
        {">=", "ge"},
    };

    return relExp->Output();
  }
};

class EqExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> relExp;

  void Dump() const override {
    std::cout << "EqExpAST { ";
    relExp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override { return relExp->Output(); }
};

class EqExpWithOpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> eqExp;
  std::string eqOp;
  std::unique_ptr<BaseAST> relExp;

  void Dump() const override {
    std::cout << "EqExpWithOpAST { ";
    eqExp->Dump();
    std::cout << "EqExpOpAST { " << eqOp << " }";
    relExp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override {
    std::pair<bool, int> result_l = eqExp->Output();
    int cnt_l = cnt - 1;

    std::pair<bool, int> result_r = relExp->Output();
    int cnt_r = cnt - 1;

    std::unordered_map<std::string, std::string> dic = {
        {"==", "eq"},
        {"!=", "ne"},
    };

    return eqExp->Output();
  }
};

class LAndExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> eqExp;

  void Dump() const override {
    std::cout << "LAndExpAST { ";
    eqExp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override { return eqExp->Output(); }
};

class LAndExpWithOpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> lAndExp;
  std::string lAndOp;
  std::unique_ptr<BaseAST> eqExp;

  void Dump() const override {
    std::cout << "EqExpWithOpAST { ";
    lAndExp->Dump();
    std::cout << "EqExpOpAST { " << lAndOp << " }";
    eqExp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override {
    std::pair<bool, int> result_l = lAndExp->Output();
    int cnt_l = cnt - 1;

    std::pair<bool, int> result_r = eqExp->Output();
    int cnt_r = cnt - 1;

    return lAndExp->Output();
  }
};

class LOrExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> lAndExp;

  void Dump() const override {
    std::cout << "LOrExpAST { ";
    lAndExp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override { return lAndExp->Output(); }
};

class LOrExpWithOpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> lOrExp;
  std::string lOrOp;
  std::unique_ptr<BaseAST> lAndExp;

  void Dump() const override {
    std::cout << "LOrExpWithOpAST { ";
    lOrExp->Dump();
    std::cout << "LOrExpOpAST { " << lOrOp << " }";
    lAndExp->Dump();
    std::cout << " }";
  }

  std::pair<bool, int> Output() const override {
    std::pair<bool, int> result_l = lOrExp->Output();
    int cnt_l = cnt - 1;

    std::pair<bool, int> result_r = lAndExp->Output();
    int cnt_r = cnt - 1;

    return lOrExp->Output();
  }
};