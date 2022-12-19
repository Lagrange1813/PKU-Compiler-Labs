#pragma once
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "koopa.h"

extern std::string str;

// 所有 AST 的基类
class BaseAST {
 public:
  virtual ~BaseAST() = default;

  // Print AST Structures
  virtual void Dump() const = 0;
  // Output Koopa IR
  virtual std::pair<bool, int> Output() const = 0;
};

class CompUnitAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> sub;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class CompUnitSubAST : public BaseAST {
 public:
  std::optional<std::unique_ptr<BaseAST>> compUnit;
  std::unique_ptr<BaseAST> func_def;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class DeclWithConstAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> constDecl;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class DeclWithVarAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> varDecl;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class ConstDeclAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> bType;
  std::vector<std::unique_ptr<BaseAST>> constDefList;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class BTypeAST : public BaseAST {
 public:
  std::string type;

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

class VarDeclAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> bType;
  std::vector<std::unique_ptr<BaseAST>> varDefList;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class VarDefAST : public BaseAST {
 public:
  std::string ident;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class VarDefWithAssignAST : public BaseAST {
 public:
  std::string ident;
  std::unique_ptr<BaseAST> initVal;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class InitValAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::optional<std::unique_ptr<BaseAST>> params;
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

class FuncFParamsAST : public BaseAST {
 public:
  std::vector<std::unique_ptr<BaseAST>> paramList;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class FuncFParamAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> bType;
  std::string ident;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class BlockAST : public BaseAST {
 public:
  std::vector<std::unique_ptr<BaseAST>> blockItemList;

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

class StmtWithAssignAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> lVal;
  std::unique_ptr<BaseAST> exp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class StmtWithExpAST : public BaseAST {
 public:
  std::optional<std::unique_ptr<BaseAST>> exp;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class StmtWithBlockAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> block;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class StmtWithIfAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> if_stmt;
  std::optional<std::unique_ptr<BaseAST>> else_stmt;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class StmtWithWhileAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> stmt;

  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class StmtWithBreakAST : public BaseAST {
 public:
  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class StmtWithContinueAST : public BaseAST {
 public:
  void Dump() const override;
  std::pair<bool, int> Output() const override;
};

class StmtWithReturnAST : public BaseAST {
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

class UnaryExpWithFuncAST : public BaseAST {
 public:
  std::string ident;
  std::optional<std::unique_ptr<BaseAST>> params;

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

class FuncRParamsAST : public BaseAST {
 public:
  std::vector<std::unique_ptr<BaseAST>> paramList;

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