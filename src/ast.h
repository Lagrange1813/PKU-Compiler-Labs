#pragma once
#include <string>
#include <iostream>

// CompUnit  ::= FuncDef;

// FuncDef   ::= FuncType IDENT "(" ")" Block;
// FuncType  ::= "int";

// Block     ::= "{" Stmt "}";
// Stmt      ::= "return" Number ";";
// Number    ::= INT_CONST;

// CompUnitAST { FuncDefAST { FuncTypeAST { int }, main, BlockAST { StmtAST { 0 } } } }

// 所有 AST 的基类
class BaseAST {
 public:
  virtual ~BaseAST() = default;

  virtual void Dump() const = 0;

  virtual void Output() const = 0;
};

class CompUnitAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_def;

  void Dump() const override {
    std::cout << "CompUnitAST { ";
    func_def->Dump();
    std::cout << " }";
  }

  void Output() const override {
    func_def->Output();
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

  void Output() const override {
    std::cout << "fun ";
    std::cout << "@" << ident << "(): ";
    func_type->Output();
    block->Output();
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

    void Output() const override {
      std::cout << "i32" << " ";
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

    void Output() const override {
      std::cout << "{\n";
      std::cout << "\%entry:\n";
      stmt->Output();
      std::cout << "}";
    }
};

class StmtAST: public BaseAST {
  public:
    // std::unique_ptr<BaseAST> number;
    int number;

    void Dump() const override {
      std::cout << "StmtAST { ";
      // number->Dump();
      std::cout << number;
      std::cout << " }";
    }

    void Output() const override {
      std::cout << "  ret " << number << "\n";
    }
};