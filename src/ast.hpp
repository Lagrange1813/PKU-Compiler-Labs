#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "koopa.h"

extern std::string str;
extern int cnt;

// 所有 AST 的基类
class BaseAST {
 public:
  virtual ~BaseAST() = default;

  virtual void Dump() const = 0;
  virtual std::pair<bool, int> Output() const = 0;
};

class CompUnitAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_def;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class DeclAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> constDecl;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class ConstDeclAST : public BaseAST {
 public:
  std::string bType;
  std::vector<std::unique_ptr<BaseAST>> constDefs;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class ConstDefAST : public BaseAST {
 public:
  std::string ident;
  std::unique_ptr<BaseAST> constInitVal;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class ConstInitValAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> constExp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class FuncTypeAST : public BaseAST {
 public:
  std::string type;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class BlockAST : public BaseAST {
 public:
  std::vector<std::unique_ptr<BaseAST>> blockItems;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class BlockItemWithDeclAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> decl;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class BlockItemWithStmtAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> stmt;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class StmtAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class ExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> lOrExp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class LValAST : public BaseAST {
 public:
  std::string ident;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class PrimaryExpWithBrAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class PrimaryExpWithLValAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> lVal;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class PrimaryExpWithNumAST : public BaseAST {
 public:
  int number;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class UnaryExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> primaryExp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class UnaryExpWithOpAST : public BaseAST {
 public:
  char unaryOp;
  std::unique_ptr<BaseAST> unaryExp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class MulExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> unaryExp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class MulExpWithOpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> mulExp;
  char mulOp;
  std::unique_ptr<BaseAST> unaryExp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class AddExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> mulExp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class AddExpWithOpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> addExp;
  char addOp;
  std::unique_ptr<BaseAST> mulExp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class RelExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> addExp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class RelExpWithOpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> relExp;
  std::string relOp;
  std::unique_ptr<BaseAST> addExp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class EqExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> relExp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class EqExpWithOpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> eqExp;
  std::string eqOp;
  std::unique_ptr<BaseAST> relExp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class LAndExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> eqExp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class LAndExpWithOpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> lAndExp;
  std::string lAndOp;
  std::unique_ptr<BaseAST> eqExp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class LOrExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> lAndExp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class LOrExpWithOpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> lOrExp;
  std::string lOrOp;
  std::unique_ptr<BaseAST> lAndExp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class ConstExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};