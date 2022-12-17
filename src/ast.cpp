#include "ast.hpp"

// Koopa IR 返回值计数器
int cnt = 0;
// if 计数器（用于标定ir中不同if的基本块 then else end）
int if_cnt = -1;
// 当前块
int cur_block = -1;
// 父子块关系记录
std::unordered_map<int, int> parent;
// 记录当前块是否终止
std::vector<bool> is_block_end;

typedef enum {
  CONSTANT,
  VARIABLE,
  UNDEFINED,
} value_type;

typedef struct {
  value_type type;
  int value;
} stored_value;

std::vector<std::unordered_map<std::string, std::unique_ptr<stored_value>>*> symbol_tables;

void insertSymbol(const std::string& key, int value, bool isConst) {
  if (isConst) {
    stored_value* value_to_store = new stored_value();
    value_to_store->value = value;
    value_to_store->type = CONSTANT;
    (*symbol_tables[cur_block])[key] = std::unique_ptr<stored_value>(value_to_store);
  } else {
    stored_value* value_to_store = new stored_value();
    value_to_store->value = value;
    value_to_store->type = VARIABLE;
    (*symbol_tables[cur_block])[key] = std::unique_ptr<stored_value>(value_to_store);
  }
}

std::tuple<value_type, int, int> fetchSymbol(const std::string& key) {
  int cur = cur_block;
  while (cur >= 0) {
    if ((*symbol_tables[cur]).find(key) == (*symbol_tables[cur]).end()) {
      cur = parent[cur];
      continue;
    } else {
      return std::make_tuple((*symbol_tables[cur])[key]->type, (*symbol_tables[cur])[key]->value, cur);
    }
  }
  return std::make_tuple(UNDEFINED, -1, -1);
}

int search(const ConstExpAST* constExp);
int search(const ExpAST* exp);
int search(const LOrExpAST* lOrExp);
int search(const LOrExpWithOpAST* lOrExp);
int search(const LAndExpAST* lAndExp);
int search(const LAndExpWithOpAST* lAndExp);
int search(const EqExpAST* eqExp);
int search(const EqExpWithOpAST* eqExp);
int search(const RelExpAST* relExp);
int search(const RelExpWithOpAST* relExp);
int search(const AddExpAST* addExp);
int search(const AddExpWithOpAST* addExp);
int search(const MulExpAST* mulExp);
int search(const MulExpWithOpAST* mulExp);
int search(const UnaryExpAST* unaryExp);
int search(const UnaryExpWithOpAST* unaryExp);
int search(const PrimaryExpWithBrAST* primaryExp);
int search(const PrimaryExpWithLValAST* primaryExp);
int search(const PrimaryExpWithNumAST* primaryExp);
int search(const LValAST* lVal);

void CompUnitAST::Dump() const {
  std::cout << "CompUnitAST { ";
  func_def->Dump();
  std::cout << " }";
}

std::pair<bool, int> CompUnitAST::Output() const {
  func_def->Output();
  return std::pair<bool, int>(false, 0);
}

void DeclWithConstAST::Dump() const {
  std::cout << "DeclWithConstAST { ";
  constDecl->Dump();
  std::cout << " }";
}

std::pair<bool, int> DeclWithConstAST::Output() const {
  constDecl->Output();
  return std::make_pair(false, 0);
}

void DeclWithVarAST::Dump() const {
  std::cout << "DeclWithVarAST { ";
  varDecl->Dump();
  std::cout << " }";
}

std::pair<bool, int> DeclWithVarAST::Output() const {
  varDecl->Output();
  return std::make_pair(false, 0);
}

void ConstDeclAST::Dump() const {
  std::cout << "ConstDeclAST { ";
  std::cout << "BTypeAST { " << bType << " }";
  for (auto& constDef : constDefList) {
    constDef->Dump();
  }
  std::cout << " }";
}

std::pair<bool, int> ConstDeclAST::Output() const {
  for (auto& constDef : constDefList) {
    constDef->Output();
  }
  return std::pair<bool, int>(false, 0);
}

void ConstDefAST::Dump() const {
  std::cout << "ConstDefAST { ";
  std::cout << "Ident { " << ident << "}";
  constInitVal->Dump();
  std::cout << " }";
}

std::pair<bool, int> ConstDefAST::Output() const {
  std::pair<bool, int> result = constInitVal->Output();
  insertSymbol(ident, result.second, true);
  return std::pair<bool, int>(false, 0);
}

void ConstInitValAST::Dump() const {
  std::cout << "ConstInitValAST { ";
  constExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> ConstInitValAST::Output() const {
  int ret = search((ConstExpAST*)constExp.get());
  return std::pair<bool, int>(true, ret);
}

void VarDeclAST::Dump() const {
  std::cout << "VarDeclAST { ";
  std::cout << "BTypeAST { " << bType << " }";
  for (auto& varDef : varDefList) {
    varDef->Dump();
  }
  std::cout << " }";
}

std::pair<bool, int> VarDeclAST::Output() const {
  for (auto& varDef : varDefList) {
    varDef->Output();
  }
  return std::pair<bool, int>(false, 0);
}

void VarDefAST::Dump() const {
  std::cout << "VarDefAST { ";
  std::cout << "Ident { " << ident << "}";
  std::cout << " }";
}

std::pair<bool, int> VarDefAST::Output() const {
  str += "  @";
  str += ident;
  str += "_";
  str += std::to_string(cur_block);
  str += " = alloc i32\n";

  insertSymbol(ident, 0, false);
  return std::pair<bool, int>(false, 0);
}

void VarDefWithAssignAST::Dump() const {
  std::cout << "VarDefWithAssignAST { ";
  std::cout << "Ident { " << ident << "}";
  initVal->Dump();
  std::cout << " }";
}

std::pair<bool, int> VarDefWithAssignAST::Output() const {
  std::pair<bool, int> result = initVal->Output();
  str += "  @";
  str += ident;
  str += "_";
  str += std::to_string(cur_block);
  str += " = alloc i32\n";

  if (result.first) {
    str += "  store ";
    str += std::to_string(result.second);
    str += ", @";
    str += ident;
    str += "_";
    str += std::to_string(cur_block);
    str += "\n";
  } else {
    str += "  store %";
    str += std::to_string(cnt - 1);
    str += ", @";
    str += ident;
    str += "_";
    str += std::to_string(cur_block);
    str += "\n";
  }

  insertSymbol(ident, 0, false);
  return std::pair<bool, int>(false, 0);
}

void InitValAST::Dump() const {
  std::cout << "InitValAST { ";
  exp->Dump();
  std::cout << " }";
}

std::pair<bool, int> InitValAST::Output() const {
  return exp->Output();
}

void FuncDefAST::Dump() const {
  std::cout << "FuncDefAST { ";
  func_type->Dump();
  std::cout << ", " << ident << ", ";
  block->Dump();
  std::cout << " }";
}

std::pair<bool, int> FuncDefAST::Output() const {
  str += "fun ";
  str += "@";
  str += ident;
  str += "(): ";
  func_type->Output();
  str += "{\n";
  str += "\%entry:\n";
  block->Output();
  str += "}";

  return std::pair<bool, int>(false, 0);
}

void FuncTypeAST::Dump() const {
  std::cout << "FuncTypeAST { ";
  std::cout << type;
  std::cout << " }";
}

std::pair<bool, int> FuncTypeAST::Output() const {
  if (type == "int") {
    str += "i32";
    str += " ";
  }

  return std::pair<bool, int>(false, 0);
}

void BlockAST::Dump() const {
  std::cout << "BlockAST { ";
  for (auto& blockItem : blockItemList) {
    blockItem->Dump();
  }
  std::cout << " }";
}

std::pair<bool, int> BlockAST::Output() const {
  std::unordered_map<std::string, std::unique_ptr<stored_value>> table;

  // cur_table = symbol_tables.size();
  int parent_block = cur_block;
  cur_block = symbol_tables.size();
  parent[cur_block] = parent_block;
  symbol_tables.push_back(&table);
  // block_cnt++;

  // if (is_block_end.size() <= cur_table) {
  //   is_block_end.push_back(false);
  // } else {
  //   is_block_end[cur_table] = false;
  // }

  for (auto& blockItem : blockItemList) {
    // if (is_block_end[cur_table])
    //   return std::make_pair(false, 0);
    blockItem->Output();
  }

  // symbol_tables.pop_back();
  // cur_table = symbol_tables.size() - 1;
  cur_block = parent[cur_block];

  return std::pair<bool, int>(false, 0);
}

void BlockItemWithDeclAST::Dump() const {
  std::cout << "BlockItemWithDeclAST { ";
  decl->Dump();
  std::cout << " }";
}

std::pair<bool, int> BlockItemWithDeclAST::Output() const {
  return decl->Output();
}

void BlockItemWithStmtAST::Dump() const {
  std::cout << "BlockItemWithStmtAST { ";
  stmt->Dump();
  std::cout << " }";
}

std::pair<bool, int> BlockItemWithStmtAST::Output() const {
  return stmt->Output();
}

void StmtWithAssignAST::Dump() const {
  std::cout << "StmtWithAssignAST { ";
  lVal->Dump();
  exp->Dump();
  std::cout << " }";
}

std::pair<bool, int> StmtWithAssignAST::Output() const {
  auto ident = ((LValAST*)lVal.get())->ident;
  auto fetch_result = fetchSymbol(ident);

  if (std::get<0>(fetch_result) == UNDEFINED || std::get<0>(fetch_result) == CONSTANT) {
    assert(false);
  }

  std::pair<bool, int> result = exp->Output();

  if (result.first) {
    str += "  store ";
    str += std::to_string(result.second);
    str += ", @";
    str += ident;
    str += "_";
    str += std::to_string(std::get<2>(fetch_result));
    str += "\n";
  } else {
    str += "  store %";
    str += std::to_string(cnt - 1);
    str += ", @";
    str += ident;
    str += "_";
    str += std::to_string(std::get<2>(fetch_result));
    str += "\n";
  }

  return std::pair<bool, int>(false, 0);
}

void StmtWithExpAST::Dump() const {
  std::cout << "StmtWithExpAST { ";
  if (exp) {
    (*exp)->Dump();
  }
  std::cout << " }";
}

std::pair<bool, int> StmtWithExpAST::Output() const {
  if (exp)
    (*exp)->Output();
  return std::pair<bool, int>(false, 0);
}

void StmtWithBlockAST::Dump() const {
  std::cout << "StmtWithBlockAST { ";
  block->Dump();
  std::cout << " }";
}

std::pair<bool, int> StmtWithBlockAST::Output() const {
  return block->Output();
}

void StmtWithIfAST::Dump() const {
  std::cout << "StmtWithIfAST { ";
  exp->Dump();
  if_stmt->Dump();
  if (else_stmt)
    (*else_stmt)->Dump();
  std::cout << " }";
}

std::pair<bool, int> StmtWithIfAST::Output() const {
  auto result = exp->Output();

  if_cnt++;
  int cur_if = if_cnt;

  if (result.first) {
    str += "  br ";
    str += std::to_string(result.second);
    str += ", %then";
    if (cur_if != 0) {
      str += "_";
      str += std::to_string(cur_if);
    }
    str += else_stmt ? ", %else" : ", %end";
    if (cur_if != 0) {
      str += "_";
      str += std::to_string(cur_if);
    }
    str += "\n";
  } else {
    str += "  br %";
    str += std::to_string(cnt - 1);
    str += ", %then";
    if (cur_if != 0) {
      str += "_";
      str += std::to_string(cur_if);
    }
    str += else_stmt ? ", %else" : ", %end";
    if (cur_if != 0) {
      str += "_";
      str += std::to_string(cur_if);
    }
    str += "\n";
  }

  str += "%then";
  if (cur_if != 0) {
    str += "_";
    str += std::to_string(cur_if);
  }
  str += ":\n";

  // int if_block = cur_table + 1;
  if_stmt->Output();

  // if (!is_block_end[if_block]) {
    str += "  jump %end";
    if (cur_if != 0) {
      str += "_";
      str += std::to_string(cur_if);
    }
    str += "\n";
  // }

  // int else_block = cur_table + 1;

  if (else_stmt) {
    str += "%else";
    if (cur_if != 0) {
      str += "_";
      str += std::to_string(cur_if);
    }
    str += ":\n";

    (*else_stmt)->Output();

    // if (!is_block_end[else_block]) {
      str += "  jump %end";
      if (cur_if != 0) {
        str += "_";
        str += std::to_string(cur_if);
      }
      str += "\n";
    // }
  }

  // if (is_block_end[if_block] && (else_stmt && is_block_end[else_block])) {
    // is_block_end[cur_table] = true;
  // } else {
    str += "%end";
    if (cur_if != 0) {
      str += "_";
      str += std::to_string(cur_if);
    }
    str += ":\n";
  // }

  return std::make_pair(false, 0);
}

void StmtWithReturnAST::Dump() const {
  std::cout << "StmtWithReturnAST { ";
  exp->Dump();
  std::cout << " }";
}

std::pair<bool, int> StmtWithReturnAST::Output() const {
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

  // is_block_end[cur_table] = true;

  return std::make_pair(false, 0);
}

void ExpAST::Dump() const {
  std::cout << "ExpAST { ";
  lOrExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> ExpAST::Output() const {
  return lOrExp->Output();
}

void LValAST::Dump() const {
  std::cout << "LValAST { ";
  std::cout << ident;
  std::cout << " }";
}

std::pair<bool, int> LValAST::Output() const {
  std::tuple<value_type, int, int> result = fetchSymbol(ident);
  if (std::get<0>(result) == CONSTANT)
    return std::pair<bool, int>(true, std::get<1>(result));
  else if (std::get<0>(result) == VARIABLE) {
    str += "  %";
    str += std::to_string(cnt);
    cnt++;
    str += " = load @";
    str += ident;
    str += "_";
    str += std::to_string(std::get<2>(result));
    str += "\n";
  } else {
    assert(false);
  }
  return std::pair<bool, int>(false, 0);
}

void PrimaryExpWithBrAST::Dump() const {
  std::cout << "PrimaryExpWithBrAST { ";
  exp->Dump();
  std::cout << " }";
}

std::pair<bool, int> PrimaryExpWithBrAST::Output() const {
  return exp->Output();
}

void PrimaryExpWithLValAST::Dump() const {
  std::cout << "PrimaryExpWithLValAST { ";
  lVal->Dump();
  std::cout << " }";
}

std::pair<bool, int> PrimaryExpWithLValAST::Output() const {
  return lVal->Output();
}

void PrimaryExpWithNumAST::Dump() const {
  std::cout << "PrimaryExpWithNumAST { ";
  std::cout << number;
  std::cout << " }";
}

std::pair<bool, int> PrimaryExpWithNumAST::Output() const {
  return std::pair<bool, int>(true, number);
}

void UnaryExpAST::Dump() const {
  std::cout << "UnaryExpAST { ";
  primaryExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> UnaryExpAST::Output() const {
  return primaryExp->Output();
}

void UnaryExpWithOpAST::Dump() const {
  std::cout << "UnaryExpWithOpAST { ";
  std::cout << "UnaryOpAST { " << unaryOp << " }";
  unaryExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> UnaryExpWithOpAST::Output() const {
  std::pair<bool, int> result = unaryExp->Output();

  if (result.first) {
    if (unaryOp == '!') {
      str += "  %";
      str += std::to_string(cnt);
      str += " = eq ";
      str += std::to_string(result.second);
      str += ", 0\n";
      cnt++;
    } else if (unaryOp == '-') {
      str += "  %";
      str += std::to_string(cnt);
      str += " = sub 0, ";
      str += std::to_string(result.second);
      str += "\n";
      cnt++;
    } else if (unaryOp == '+') {
      return std::pair<bool, int>(true, result.second);
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

void MulExpAST::Dump() const {
  std::cout << "MulExpAST { ";
  unaryExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> MulExpAST::Output() const {
  return unaryExp->Output();
}

void MulExpWithOpAST::Dump() const {
  std::cout << "MulExpWithOpAST { ";
  mulExp->Dump();
  std::cout << "MulExpOpAST { " << mulOp << " }";
  unaryExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> MulExpWithOpAST::Output() const {
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

void AddExpAST::Dump() const {
  std::cout << "AddExpAST { ";
  mulExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> AddExpAST::Output() const {
  return mulExp->Output();
}

void AddExpWithOpAST::Dump() const {
  std::cout << "AddExpWithOpAST { ";
  addExp->Dump();
  std::cout << "AddExpOpAST { " << addOp << " }";
  mulExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> AddExpWithOpAST::Output() const {
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

void RelExpAST::Dump() const {
  std::cout << "RelExpAST { ";
  addExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> RelExpAST::Output() const {
  return addExp->Output();
}

void RelExpWithOpAST::Dump() const {
  std::cout << "RelExpWithOpAST { ";
  relExp->Dump();
  std::cout << "RelExpOpAST { " << relOp << " }";
  addExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> RelExpWithOpAST::Output() const {
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

  if (result_l.first && result_r.first) {
    str += "  %";
    str += std::to_string(cnt);
    str += " = ";
    str += dic[relOp];
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
    str += dic[relOp];
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
    str += dic[relOp];
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
    str += dic[relOp];
    str += " %";
    str += std::to_string(cnt_l);
    str += ", %";
    str += std::to_string(cnt_r);
    str += "\n";
    cnt++;
  }

  return std::pair<bool, int>(false, 0);
}

void EqExpAST::Dump() const {
  std::cout << "EqExpAST { ";
  relExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> EqExpAST::Output() const {
  return relExp->Output();
}

void EqExpWithOpAST::Dump() const {
  std::cout << "EqExpWithOpAST { ";
  eqExp->Dump();
  std::cout << "EqExpOpAST { " << eqOp << " }";
  relExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> EqExpWithOpAST::Output() const {
  std::pair<bool, int> result_l = eqExp->Output();
  int cnt_l = cnt - 1;

  std::pair<bool, int> result_r = relExp->Output();
  int cnt_r = cnt - 1;

  std::unordered_map<std::string, std::string> dic = {
      {"==", "eq"},
      {"!=", "ne"},
  };

  if (result_l.first && result_r.first) {
    str += "  %";
    str += std::to_string(cnt);
    str += " = ";
    str += dic[eqOp];
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
    str += dic[eqOp];
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
    str += dic[eqOp];
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
    str += dic[eqOp];
    str += " %";
    str += std::to_string(cnt_l);
    str += ", %";
    str += std::to_string(cnt_r);
    str += "\n";
    cnt++;
  }

  return std::pair<bool, int>(false, 0);
}

void LAndExpAST::Dump() const {
  std::cout << "LAndExpAST { ";
  eqExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> LAndExpAST::Output() const {
  return eqExp->Output();
}

/// @brief Convert non-0/1 input to 0/1
/// @param input
/// @return bool
bool filter(int input) {
  if (input == 0 || input == 1) {
    return true;
  }

  str += "  %";
  str += std::to_string(cnt);
  str += " = ";
  str += "ne";
  str += " ";
  str += std::to_string(input);
  str += ", 0\n";
  cnt++;

  return false;
}

void LAndExpWithOpAST::Dump() const {
  std::cout << "EqExpWithOpAST { ";
  lAndExp->Dump();
  std::cout << "EqExpOpAST { " << lAndOp << " }";
  eqExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> LAndExpWithOpAST::Output() const {
  str += "  %result_";
  str += std::to_string(if_cnt + 1);
  str += " = alloc i32";
  str += "\n";
  str += "  store 0, %result_";
  str += std::to_string(if_cnt + 1);
  str += "\n";

  auto result_l = lAndExp->Output();
  int cnt_l = cnt - 1;

  if (result_l.first) {
    str += "  %";
    str += std::to_string(cnt);
    cnt++;
    str += " = ne ";
    str += std::to_string(result_l.second);
    str += ", 0\n";
  } else {
    str += "  %";
    str += std::to_string(cnt);
    cnt++;
    str += " = ne %";
    str += std::to_string(cnt_l);
    str += ", 0\n";
  }

  if_cnt++;
  int cur_if = if_cnt;

  str += "  br %";
  str += std::to_string(cnt - 1);
  str += ", %then";
  str += "_";
  str += std::to_string(cur_if);
  str += ", %end";
  str += "_";
  str += std::to_string(cur_if);
  str += "\n";

  str += "%then";
  str += "_";
  str += std::to_string(cur_if);
  str += ":\n";

  auto result_r = eqExp->Output();
  int cnt_r = cnt - 1;

  if (result_r.first) {
    str += "  %";
    str += std::to_string(cnt);
    cnt++;
    str += " = ne ";
    str += std::to_string(result_r.second);
    str += ", 0\n";
  } else {
    str += "  %";
    str += std::to_string(cnt);
    cnt++;
    str += " = ne %";
    str += std::to_string(cnt_r);
    str += ", 0\n";
  }

  str += "  store %";
  str += std::to_string(cnt - 1);
  str += ", @result_";
  str += std::to_string(cur_if);
  str += "\n";

  str += "  jump %end";
  str += "_";
  str += std::to_string(cur_if);
  str += "\n";

  str += "%end";
  str += "_";
  str += std::to_string(cur_if);
  str += ":\n";

  str += "  %";
  str += std::to_string(cnt);
  cnt++;
  str += " = load @result_";
  str += std::to_string(cur_if);
  str += "\n";

  return std::make_pair(false, 0);
}

void LOrExpAST::Dump() const {
  std::cout << "LOrExpAST { ";
  lAndExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> LOrExpAST::Output() const {
  return lAndExp->Output();
}

void LOrExpWithOpAST::Dump() const {
  std::cout << "LOrExpWithOpAST { ";
  lOrExp->Dump();
  std::cout << "LOrExpOpAST { " << lOrOp << " }";
  lAndExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> LOrExpWithOpAST::Output() const {
  str += "  %result_";
  str += std::to_string(if_cnt + 1);
  str += " = alloc i32";
  str += "\n";
  str += "  store 1, %result_";
  str += std::to_string(if_cnt + 1);
  str += "\n";

  auto result_l = lOrExp->Output();
  int cnt_l = cnt - 1;

  if (result_l.first) {
    str += "  %";
    str += std::to_string(cnt);
    cnt++;
    str += " = eq ";
    str += std::to_string(result_l.second);
    str += ", 0\n";
  } else {
    str += "  %";
    str += std::to_string(cnt);
    cnt++;
    str += " = eq %";
    str += std::to_string(cnt_l);
    str += ", 0\n";
  }

  if_cnt++;
  int cur_if = if_cnt;

  str += "  br %";
  str += std::to_string(cnt - 1);
  str += ", %then";
  str += "_";
  str += std::to_string(cur_if);
  str += ", %end";
  str += "_";
  str += std::to_string(cur_if);
  str += "\n";

  str += "%then";
  str += "_";
  str += std::to_string(cur_if);
  str += ":\n";

  auto result_r = lAndExp->Output();
  int cnt_r = cnt - 1;

  if (result_r.first) {
    str += "  %";
    str += std::to_string(cnt);
    cnt++;
    str += " = ne ";
    str += std::to_string(result_r.second);
    str += ", 0\n";
  } else {
    str += "  %";
    str += std::to_string(cnt);
    cnt++;
    str += " = ne %";
    str += std::to_string(cnt_r);
    str += ", 0\n";
  }

  str += "  store %";
  str += std::to_string(cnt - 1);
  str += ", @result_";
  str += std::to_string(cur_if);
  str += "\n";

  str += "  jump %end";
  str += "_";
  str += std::to_string(cur_if);
  str += "\n";

  str += "%end";
  str += "_";
  str += std::to_string(cur_if);
  str += ":\n";

  str += "  %";
  str += std::to_string(cnt);
  cnt++;
  str += " = load @result_";
  str += std::to_string(cur_if);
  str += "\n";

  return std::make_pair(false, 0);
}

void ConstExpAST::Dump() const {
  std::cout << "ConstExpAST { ";
  exp->Dump();
  std::cout << " }";
}

std::pair<bool, int> ConstExpAST::Output() const {
  return exp->Output();
}

int search(const ConstExpAST* constExp) {
  return search((ExpAST*)constExp->exp.get());
}

int search(const ExpAST* exp) {
  auto lOrExp = exp->lOrExp.get();
  if (typeid(*lOrExp) == typeid(LOrExpAST))
    return search((LOrExpAST*)lOrExp);
  else if (typeid(*lOrExp) == typeid(LOrExpWithOpAST))
    return search((LOrExpWithOpAST*)lOrExp);
  return 0;
}

int search(const LOrExpAST* lOrExp) {
  auto lAndExp = lOrExp->lAndExp.get();
  if (typeid(*lAndExp) == typeid(LAndExpAST))
    return search((LAndExpAST*)lAndExp);
  else if (typeid(*lAndExp) == typeid(LAndExpWithOpAST))
    return search((LAndExpWithOpAST*)lAndExp);
  return 0;
}

int search(const LOrExpWithOpAST* lOrExp) {
  int lhs = 0;
  auto exp_l = lOrExp->lOrExp.get();
  if (typeid(*exp_l) == typeid(LOrExpAST))
    lhs = search((LOrExpAST*)exp_l);
  else if (typeid(*exp_l) == typeid(LOrExpWithOpAST))
    lhs = search((LOrExpWithOpAST*)exp_l);

  int rhs = 0;
  auto exp_r = lOrExp->lAndExp.get();
  if (typeid(*exp_r) == typeid(LAndExpAST))
    rhs = search((LAndExpAST*)exp_r);
  else if (typeid(*exp_r) == typeid(LAndExpWithOpAST))
    rhs = search((LAndExpWithOpAST*)exp_r);

  return lhs || rhs;
}

int search(const LAndExpAST* lAndExp) {
  auto eqExp = lAndExp->eqExp.get();
  if (typeid(*eqExp) == typeid(EqExpAST))
    return search((EqExpAST*)eqExp);
  else if (typeid(*eqExp) == typeid(EqExpWithOpAST))
    return search((EqExpWithOpAST*)eqExp);
  return 0;
}

int search(const LAndExpWithOpAST* lAndExp) {
  int lhs = 0;
  auto exp_l = lAndExp->lAndExp.get();
  if (typeid(*exp_l) == typeid(LAndExpAST))
    lhs = search((LAndExpAST*)exp_l);
  else if (typeid(*exp_l) == typeid(LAndExpWithOpAST))
    lhs = search((LAndExpWithOpAST*)exp_l);

  int rhs = 0;
  auto exp_r = lAndExp->eqExp.get();
  if (typeid(*exp_r) == typeid(EqExpAST))
    rhs = search((EqExpAST*)exp_r);
  else if (typeid(*exp_r) == typeid(EqExpWithOpAST))
    rhs = search((EqExpWithOpAST*)exp_r);

  return lhs && rhs;
}

int search(const EqExpAST* eqExp) {
  auto relExp = eqExp->relExp.get();
  if (typeid(*relExp) == typeid(RelExpAST))
    return search((RelExpAST*)relExp);
  else if (typeid(*relExp) == typeid(RelExpWithOpAST))
    return search((RelExpWithOpAST*)relExp);
  return 0;
}

int search(const EqExpWithOpAST* eqExp) {
  int lhs = 0;
  auto exp_l = eqExp->eqExp.get();
  if (typeid(*exp_l) == typeid(EqExpAST))
    lhs = search((EqExpAST*)exp_l);
  else if (typeid(*exp_l) == typeid(EqExpWithOpAST))
    lhs = search((EqExpWithOpAST*)exp_l);

  int rhs = 0;
  auto exp_r = eqExp->relExp.get();
  if (typeid(*exp_r) == typeid(RelExpAST))
    rhs = search((RelExpAST*)exp_r);
  else if (typeid(*exp_r) == typeid(RelExpWithOpAST))
    rhs = search((RelExpWithOpAST*)exp_r);

  if (eqExp->eqOp == "==")
    return lhs == rhs;
  else if (eqExp->eqOp == "!=")
    return lhs != rhs;
  else
    assert(false);

  return 0;
}

int search(const RelExpAST* relExp) {
  auto addExp = relExp->addExp.get();
  if (typeid(*addExp) == typeid(AddExpAST))
    return search((AddExpAST*)addExp);
  else if (typeid(*addExp) == typeid(AddExpWithOpAST))
    return search((AddExpWithOpAST*)addExp);
  return 0;
}

int search(const RelExpWithOpAST* relExp) {
  int lhs = 0;
  auto exp_l = relExp->relExp.get();
  if (typeid(*exp_l) == typeid(RelExpAST))
    lhs = search((RelExpAST*)exp_l);
  else if (typeid(*exp_l) == typeid(RelExpWithOpAST))
    lhs = search((RelExpWithOpAST*)exp_l);

  int rhs = 0;
  auto exp_r = relExp->addExp.get();
  if (typeid(*exp_r) == typeid(AddExpAST))
    rhs = search((AddExpAST*)exp_r);
  else if (typeid(*exp_r) == typeid(AddExpWithOpAST))
    rhs = search((AddExpWithOpAST*)exp_r);

  if (relExp->relOp == "<")
    return lhs < rhs;
  else if (relExp->relOp == ">")
    return lhs > rhs;
  else if (relExp->relOp == "<=")
    return lhs <= rhs;
  else if (relExp->relOp == ">=")
    return lhs >= rhs;
  else
    assert(false);

  return 0;
}

int search(const AddExpAST* addExp) {
  auto mulExp = addExp->mulExp.get();
  if (typeid(*mulExp) == typeid(MulExpAST))
    return search((MulExpAST*)mulExp);
  else if (typeid(*mulExp) == typeid(MulExpWithOpAST))
    return search((MulExpWithOpAST*)mulExp);
  return 0;
}

int search(const AddExpWithOpAST* addExp) {
  int lhs = 0;
  auto exp_l = addExp->addExp.get();
  if (typeid(*exp_l) == typeid(AddExpAST))
    lhs = search((AddExpAST*)exp_l);
  else if (typeid(*exp_l) == typeid(AddExpWithOpAST))
    lhs = search((AddExpWithOpAST*)exp_l);

  int rhs = 0;
  auto exp_r = addExp->mulExp.get();
  if (typeid(*exp_r) == typeid(MulExpAST))
    rhs = search((MulExpAST*)exp_r);
  else if (typeid(*exp_r) == typeid(MulExpWithOpAST))
    rhs = search((MulExpWithOpAST*)exp_r);

  if (addExp->addOp == '+')
    return lhs + rhs;
  else if (addExp->addOp == '-')
    return lhs - rhs;
  else
    assert(false);

  return 0;
}

int search(const MulExpAST* mulExp) {
  auto unaryExp = mulExp->unaryExp.get();
  if (typeid(*unaryExp) == typeid(UnaryExpAST))
    return search((UnaryExpAST*)unaryExp);
  else if (typeid(*unaryExp) == typeid(UnaryExpWithOpAST))
    return search((UnaryExpWithOpAST*)unaryExp);
  return 0;
}

int search(const MulExpWithOpAST* mulExp) {
  int lhs = 0;
  auto exp_l = mulExp->mulExp.get();
  if (typeid(*exp_l) == typeid(MulExpAST))
    lhs = search((MulExpAST*)exp_l);
  else if (typeid(*exp_l) == typeid(MulExpWithOpAST))
    lhs = search((MulExpWithOpAST*)exp_l);

  int rhs = 0;
  auto exp_r = mulExp->unaryExp.get();
  if (typeid(*exp_r) == typeid(UnaryExpAST))
    rhs = search((UnaryExpAST*)exp_r);
  else if (typeid(*exp_r) == typeid(UnaryExpWithOpAST))
    rhs = search((UnaryExpWithOpAST*)exp_r);

  if (mulExp->mulOp == '*')
    return lhs * rhs;
  else if (mulExp->mulOp == '/')
    return lhs / rhs;
  else if (mulExp->mulOp == '%')
    return lhs % rhs;
  else
    assert(false);

  return 0;
}

int search(const UnaryExpAST* unaryExp) {
  auto primaryExp = unaryExp->primaryExp.get();
  if (typeid(*primaryExp) == typeid(PrimaryExpWithBrAST))
    return search((PrimaryExpWithBrAST*)primaryExp);
  else if (typeid(*primaryExp) == typeid(PrimaryExpWithLValAST))
    return search((PrimaryExpWithLValAST*)primaryExp);
  else if (typeid(*primaryExp) == typeid(PrimaryExpWithNumAST))
    return search((PrimaryExpWithNumAST*)primaryExp);
  return 0;
}

int search(const UnaryExpWithOpAST* unaryExp) {
  auto exp = unaryExp->unaryExp.get();
  int number = search((UnaryExpAST*)exp);

  if (unaryExp->unaryOp == '+')
    return number;
  else if (unaryExp->unaryOp == '-')
    return -number;
  else if (unaryExp->unaryOp == '!')
    return !number;
  else
    assert(false);

  return 0;
}

int search(const PrimaryExpWithBrAST* primaryExp) {
  return search((ExpAST*)primaryExp->exp.get());
}

int search(const PrimaryExpWithLValAST* primaryExp) {
  return search((LValAST*)primaryExp->lVal.get());
}

int search(const PrimaryExpWithNumAST* primaryExp) {
  return primaryExp->number;
}

int search(const LValAST* lVal) {
  auto result = fetchSymbol(lVal->ident);
  if (std::get<0>(result) == CONSTANT)
    return std::get<1>(result);
  assert(false);
  return 0;
}