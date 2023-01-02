#include "ast.hpp"

// Koopa IR 返回值计数器
int cnt = 0;
// if 计数器（用于标定ir中不同if的基本块 then else end）
int if_cnt = -1;
// 记录 while 当前层数序号
int while_level = -1;
// while 计数器（标定不同while的基本块）
int while_cnt = -1;
// 当前块
int cur_block = 0;
// 用于全局变量生成不同 ir
bool is_global_area = false;
// 父子块关系记录
std::unordered_map<int, int> parent;
// 记录当前块是否终止
std::vector<bool> is_block_end;
// 记录 while_level 与 while_cnt 对应关系
std::unordered_map<int, int> level_to_cnt;

typedef enum {
  CONSTANT,
  VARIABLE,

  FUNCTION,
  UNDEFINED,
} value_type;

typedef enum {
  INT,
  VOID,
  UND,
} func_type;

typedef struct {
  value_type type;
  union Value {
    func_type type;
    int val;
  } value;
} stored_object;

std::vector<std::unordered_map<std::string, std::unique_ptr<stored_object>>*> symbol_tables;

void insertSymbol(const std::string& key, value_type type, int value, func_type func_type) {
  switch (type) {
    case CONSTANT: {
      stored_object* object_to_store = new stored_object();
      object_to_store->type = CONSTANT;
      object_to_store->value.val = value;
      (*symbol_tables[cur_block])[key] = std::unique_ptr<stored_object>(object_to_store);
      break;
    }
    case VARIABLE: {
      stored_object* object_to_store = new stored_object();
      object_to_store->type = VARIABLE;
      object_to_store->value.val = value;
      (*symbol_tables[cur_block])[key] = std::unique_ptr<stored_object>(object_to_store);
      break;
    }
    case FUNCTION: {
      stored_object* object_to_store = new stored_object();
      object_to_store->type = FUNCTION;
      object_to_store->value.type = func_type;
      (*symbol_tables[cur_block])[key] = std::unique_ptr<stored_object>(object_to_store);
      break;
    }
    default:
      break;
  }
}

// Value type, Constant value, Varible level, Func type
std::tuple<value_type, int, int, func_type> fetchSymbol(const std::string& key) {
  int cur = cur_block;
  while (cur >= 0) {
    if ((*symbol_tables[cur]).find(key) == (*symbol_tables[cur]).end()) {
      cur = parent[cur];
      // cur--;
      continue;
    } else {
      if ((*symbol_tables[cur])[key]->type == CONSTANT || (*symbol_tables[cur])[key]->type == VARIABLE) {
        return std::make_tuple((*symbol_tables[cur])[key]->type, (*symbol_tables[cur])[key]->value.val, cur, UND);
      } else if ((*symbol_tables[cur])[key]->type == FUNCTION) {
        return std::make_tuple((*symbol_tables[cur])[key]->type, 0, cur, (*symbol_tables[cur])[key]->value.type);
      } else
        assert(false);
    }
  }
  // Mark here
  str += "\tUNDEFINED\n";
  return std::make_tuple(UNDEFINED, -1, -1, UND);
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
  sub->Dump();
  std::cout << "} ";
}

std::pair<bool, int> CompUnitAST::Output() const {
  std::unordered_map<std::string, std::unique_ptr<stored_object>> table;
  symbol_tables.push_back(&table);
  parent[0] = -1;

  str += "decl @getint(): i32\n";
  str += "decl @getch(): i32\n";
  str += "decl @getarray(*i32): i32\n";
  str += "decl @putint(i32)\n";
  str += "decl @putch(i32)\n";
  str += "decl @putarray(i32, *i32)\n";
  str += "decl @starttime()\n";
  str += "decl @stoptime()\n\n";

  insertSymbol("getint", FUNCTION, 0, INT);
  insertSymbol("getch", FUNCTION, 0, INT);
  insertSymbol("getarray", FUNCTION, 0, INT);
  insertSymbol("putint", FUNCTION, 0, VOID);
  insertSymbol("putch", FUNCTION, 0, VOID);
  insertSymbol("putarray", FUNCTION, 0, VOID);
  insertSymbol("starttime", FUNCTION, 0, VOID);
  insertSymbol("stoptime", FUNCTION, 0, VOID);

  sub->Output();
  return std::pair<bool, int>(false, 0);
}

void CompUnitSubWithDeclAST::Dump() const {
  std::cout << "CompUnitSubWithDeclAST { ";
  if (compUnit)
    (*compUnit)->Dump();
  decl->Dump();
  std::cout << "} ";
}

std::pair<bool, int> CompUnitSubWithDeclAST::Output() const {
  if (compUnit)
    (*compUnit)->Output();

  // auto decl_p = decl.get();
  // if (typeid(*decl_p) == typeid(DeclWithVarAST)) {
  // 全局区域
  is_global_area = true;
  decl->Output();
  is_global_area = false;
  // } else {
  //   decl->Output();
  // }

  return std::pair<bool, int>(false, 0);
}

void CompUnitSubWithFuncAST::Dump() const {
  std::cout << "CompUnitSubWithFuncAST { ";
  if (compUnit)
    (*compUnit)->Dump();
  func_def->Dump();
  std::cout << "} ";
}

std::pair<bool, int> CompUnitSubWithFuncAST::Output() const {
  if (compUnit)
    (*compUnit)->Output();
  func_def->Output();
  return std::pair<bool, int>(false, 0);
}

void DeclWithConstAST::Dump() const {
  std::cout << "DeclWithConstAST { ";
  constDecl->Dump();
  std::cout << "} ";
}

std::pair<bool, int> DeclWithConstAST::Output() const {
  constDecl->Output();
  return std::make_pair(false, 0);
}

void DeclWithVarAST::Dump() const {
  std::cout << "DeclWithVarAST { ";
  varDecl->Dump();
  std::cout << "} ";
}

std::pair<bool, int> DeclWithVarAST::Output() const {
  varDecl->Output();
  return std::make_pair(false, 0);
}

void ConstDeclAST::Dump() const {
  std::cout << "ConstDeclAST { ";
  std::cout << "BTypeAST { " << bType << " } ";
  for (auto& constDef : constDefList) {
    constDef->Dump();
  }
  std::cout << "} ";
}

std::pair<bool, int> ConstDeclAST::Output() const {
  for (auto& constDef : constDefList) {
    constDef->Output();
  }
  return std::pair<bool, int>(false, 0);
}

void ConstDefAST::Dump() const {
  std::cout << "ConstDefAST { ";
  std::cout << "Ident { " << ident << " } ";
  if (constExp)
    (*constExp)->Dump();
  constInitVal->Dump();
  std::cout << "} ";
}

std::pair<bool, int> ConstDefAST::Output() const {
  if (constExp) {
    auto size = search((ConstExpAST*)constExp->get());
    auto result = ((ConstInitValWithListAST*)constInitVal.get())->prepare();
    insertSymbol(ident, VARIABLE, 0, UND);
    if (is_global_area) {
      str += "global @";
      str += ident;
      str += "_";
      str += std::to_string(cur_block);
      str += " = alloc [i32, ";
      str += std::to_string(size);
      str += "], {";
      for (int i = 0; i < size; i++) {
        if (i < result.size())
          str += std::to_string(result[i]);
        else
          str += "0";
        if (i != size - 1)
          str += ", ";
      }
      str += "}\n\n";
    } else {
      str += "\t@";
      str += ident;
      str += "_";
      str += std::to_string(cur_block);
      str += " = alloc [i32, ";
      str += std::to_string(size);
      str += "]\n";

      for (int i = 0; i < size; i++) {
        str += "\t%";
        str += std::to_string(cnt);
        cnt++;
        str += " = getelemptr @";
        str += ident;
        str += "_";
        str += std::to_string(cur_block);
        str += ", ";
        str += std::to_string(i);
        str += "\n";

        str += "\tstore ";

        if (i < result.size())
          str += std::to_string(result[i]);
        else
          str += "0";

        str += ", %";
        str += std::to_string(cnt - 1);
        str += "\n";
      }
    }
  } else {
    std::pair<bool, int> result = constInitVal->Output();
    insertSymbol(ident, CONSTANT, result.second, UND);
  }
  return std::pair<bool, int>(false, 0);
}

void ConstInitValAST::Dump() const {
  std::cout << "ConstInitValAST { ";
  constExp->Dump();
  std::cout << "} ";
}

std::pair<bool, int> ConstInitValAST::Output() const {
  int ret = search((ConstExpAST*)constExp.get());
  return std::pair<bool, int>(true, ret);
}

void ConstInitValWithListAST::Dump() const {
  std::cout << "ConstInitValWithListAST { ";
  for (auto& constExp : constExpList)
    constExp->Dump();
  std::cout << "} ";
}

std::pair<bool, int> ConstInitValWithListAST::Output() const {
  return std::pair<bool, int>(false, 0);
}

std::vector<int> ConstInitValWithListAST::prepare() {
  std::vector<int> vec;
  for (auto& constExp : constExpList) {
    auto result = search((ConstExpAST*)constExp.get());
    vec.push_back(result);
  }
  return vec;
}

void VarDeclAST::Dump() const {
  std::cout << "VarDeclAST { ";
  std::cout << "BTypeAST { " << bType << " } ";
  for (auto& varDef : varDefList) {
    varDef->Dump();
  }
  std::cout << "} ";
}

std::pair<bool, int> VarDeclAST::Output() const {
  for (auto& varDef : varDefList) {
    varDef->Output();
  }
  return std::pair<bool, int>(false, 0);
}

void VarDefAST::Dump() const {
  std::cout << "VarDefAST { ";
  std::cout << "Ident { " << ident << " } ";
  if (constExp)
    (*constExp)->Dump();
  std::cout << "} ";
}

std::pair<bool, int> VarDefAST::Output() const {
  if (constExp) {
    auto size = search((ConstExpAST*)constExp->get());
    insertSymbol(ident, VARIABLE, 0, UND);
    if (is_global_area) {
      str += "global @";
      str += ident;
      str += "_";
      str += std::to_string(cur_block);
      str += " = alloc [i32, ";
      str += std::to_string(size);
      str += "], zeroinit\n\n";
    } else {
      str += "\t@";
      str += ident;
      str += "_";
      str += std::to_string(cur_block);
      str += " = alloc [i32, ";
      str += std::to_string(size);
      str += "]\n";
    }
  } else {
    if (is_global_area) {
      str += "global @";
      str += ident;
      str += "_";
      str += std::to_string(cur_block);
      str += " = alloc i32, zeroinit\n\n";
    } else {
      str += "\t@";
      str += ident;
      str += "_";
      str += std::to_string(cur_block);
      str += " = alloc i32\n";
    }
  }

  insertSymbol(ident, VARIABLE, 0, UND);
  return std::pair<bool, int>(false, 0);
}

void VarDefWithAssignAST::Dump() const {
  std::cout << "VarDefWithAssignAST { ";
  std::cout << "Ident { " << ident << "} ";
  if (constExp)
    (*constExp)->Dump();
  initVal->Dump();
  std::cout << " } ";
}

std::pair<bool, int> VarDefWithAssignAST::Output() const {
  if (constExp) {
    auto size = search((ConstExpAST*)constExp->get());
    auto result = ((InitValWithListAST*)initVal.get())->prepare();
    insertSymbol(ident, VARIABLE, 0, UND);
    if (is_global_area) {
      str += "global @";
      str += ident;
      str += "_";
      str += std::to_string(cur_block);
      str += " = alloc [i32, ";
      str += std::to_string(size);
      str += "], {";
      for (int i = 0; i < size; i++) {
        if (i < result.size()) {
          if (!result[i].first)
            assert(false);
          str += std::to_string(result[i].second);
        } else {
          str += "0";
        }
        if (i != size - 1)
          str += ", ";
      }
      str += "}\n\n";
    } else {
      str += "\t@";
      str += ident;
      str += "_";
      str += std::to_string(cur_block);
      str += " = alloc [i32, ";
      str += std::to_string(size);
      str += "]\n";

      for (int i = 0; i < size; i++) {
        str += "\t%";
        str += std::to_string(cnt);
        cnt++;
        str += " = getelemptr @";
        str += ident;
        str += "_";
        str += std::to_string(cur_block);
        str += ", ";
        str += std::to_string(i);
        str += "\n";

        str += "\tstore ";

        if (i < result.size()) {
          if (result[i].first) {
            str += std::to_string(result[i].second);
          } else {
            str += "%";
            str += std::to_string(result[i].second);
          }
        } else {
          str += "0";
        }

        str += ", %";
        str += std::to_string(cnt - 1);
        str += "\n";
      }
    }
  } else {
    std::pair<bool, int> result = initVal->Output();
    if (is_global_area)
      str += "global @";
    else
      str += "\t@";
    str += ident;
    str += "_";
    str += std::to_string(cur_block);
    str += " = alloc i32\n";

    if (is_global_area) {
      str.pop_back();
      if (result.first) {
        str += ", ";
        str += std::to_string(result.second);
        str += "\n\n";
      } else {
        assert(false);
      }
    } else {
      if (result.first) {
        str += "\tstore ";
        str += std::to_string(result.second);
        str += ", @";
        str += ident;
        str += "_";
        str += std::to_string(cur_block);
        str += "\n";
      } else {
        str += "\tstore %";
        str += std::to_string(cnt - 1);
        str += ", @";
        str += ident;
        str += "_";
        str += std::to_string(cur_block);
        str += "\n";
      }
    }

    insertSymbol(ident, VARIABLE, 0, UND);
  }
  return std::pair<bool, int>(false, 0);
}

void InitValAST::Dump() const {
  std::cout << "InitValAST { ";
  exp->Dump();
  std::cout << "} ";
}

std::pair<bool, int> InitValAST::Output() const {
  return exp->Output();
}

void InitValWithListAST::Dump() const {
  std::cout << "InitValWithListAST { ";
  for (auto& exp : expList)
    exp->Dump();
  std::cout << "} ";
}

std::pair<bool, int> InitValWithListAST::Output() const {
  return std::make_pair(false, 0);
}

std::vector<std::pair<bool, int>> InitValWithListAST::prepare() {
  std::vector<std::pair<bool, int>> vec;
  for (auto& exp : expList) {
    auto result = exp->Output();
    int cur = cnt - 1;
    if (result.first) {
      vec.push_back(std::make_pair(true, result.second));
    } else {
      vec.push_back(std::make_pair(false, cur));
    }
  }
  return vec;
}

void FuncDefAST::Dump() const {
  std::cout << "FuncDefAST { ";
  std::cout << "FuncTypeAST { " << funcType << " } ";
  std::cout << "Ident { " << ident << " } ";
  if (params)
    (*params)->Dump();
  block->Dump();
  std::cout << "} ";
}

std::pair<bool, int> FuncDefAST::Output() const {
  // 向当前符号表中插入该函数定义
  func_type ty = UND;
  if (funcType == "int")
    ty = INT;
  else if (funcType == "void")
    ty = VOID;
  insertSymbol(ident, FUNCTION, 0, ty);

  // 清空计数器
  cnt = 0;

  // 为整个函数添加符号表，便于参数定义
  std::unordered_map<std::string, std::unique_ptr<stored_object>> table;

  int parent_block = cur_block;
  cur_block = symbol_tables.size();
  parent[cur_block] = parent_block;
  symbol_tables.push_back(&table);

  is_block_end.push_back(false);

  str += "fun ";
  str += "@";
  str += ident;
  str += "(";
  if (params)
    (*params)->Output();
  str += ")";
  if (funcType == "int")
    str += ": i32";
  str += " {\n";
  str += "\%entry:\n";

  // 函数参数及 Block 输出
  if (params)
    ((FuncFParamsAST*)(*params).get())->declare();
  block->Output();

  // 无返回值补 ret
  if (funcType == "void" && !is_block_end[cur_block])
    str += "\tret\n";

  str += "}\n\n";

  cur_block = parent[cur_block];
  return std::pair<bool, int>(false, 0);
}

void FuncFParamsAST::Dump() const {
  std::cout << "FuncFParamsAST { ";
  for (auto& param : paramList) {
    param->Dump();
  }
  std::cout << "} ";
}

std::pair<bool, int> FuncFParamsAST::Output() const {
  for (int i = 0; i < paramList.size(); i++) {
    paramList[i]->Output();
    if (i != paramList.size() - 1)
      str += ", ";
  }
  return std::pair<bool, int>(false, 0);
}

void FuncFParamsAST::declare() {
  for (auto& param : paramList) {
    ((FuncFParamAST*)param.get())->declare();
  }
}

void FuncFParamAST::Dump() const {
  std::cout << "FuncFParamAST { ";
  std::cout << "BTypeAST { " << bType << " } ";
  std::cout << "Ident { " << ident << " } ";
  std::cout << "} ";
}

std::pair<bool, int> FuncFParamAST::Output() const {
  str += "@";
  str += ident;
  if (bType == "int")
    str += ": i32";
  else
    assert(false);
  return std::pair<bool, int>(false, 0);
}

void FuncFParamAST::declare() {
  str += "\t@";
  str += ident;
  str += "_";
  str += std::to_string(cur_block);
  str += " = alloc i32\n";
  str += "\tstore @";
  str += ident;
  str += ", @";
  str += ident;
  str += "_";
  str += std::to_string(cur_block);
  str += "\n";

  insertSymbol(ident, VARIABLE, 0, UND);
}

void BlockAST::Dump() const {
  std::cout << "BlockAST { ";
  for (auto& blockItem : blockItemList) {
    blockItem->Dump();
  }
  std::cout << "} ";
}

std::pair<bool, int> BlockAST::Output() const {
  std::unordered_map<std::string, std::unique_ptr<stored_object>> table;

  int parent_block = cur_block;
  cur_block = symbol_tables.size();
  parent[cur_block] = parent_block;
  symbol_tables.push_back(&table);

  is_block_end.push_back(false);

  for (auto& blockItem : blockItemList) {
    if (is_block_end[cur_block])
      break;
    blockItem->Output();
  }

  if (parent_block != 0) {
    is_block_end[parent_block] = is_block_end[cur_block];
  }
  cur_block = parent[cur_block];
  return std::pair<bool, int>(false, 0);
}

void BlockItemWithDeclAST::Dump() const {
  std::cout << "BlockItemWithDeclAST { ";
  decl->Dump();
  std::cout << "} ";
}

std::pair<bool, int> BlockItemWithDeclAST::Output() const {
  return decl->Output();
}

void BlockItemWithStmtAST::Dump() const {
  std::cout << "BlockItemWithStmtAST { ";
  stmt->Dump();
  std::cout << "} ";
}

std::pair<bool, int> BlockItemWithStmtAST::Output() const {
  return stmt->Output();
}

void StmtWithAssignAST::Dump() const {
  std::cout << "StmtWithAssignAST { ";
  lVal->Dump();
  exp->Dump();
  std::cout << "} ";
}

std::pair<bool, int> StmtWithAssignAST::Output() const {
  auto ident = ((LValAST*)lVal.get())->ident;
  auto fetch_result = fetchSymbol(ident);

  if (std::get<0>(fetch_result) != VARIABLE) {
    assert(false);
  }

  std::pair<bool, int> result = exp->Output();

  if (result.first) {
    str += "\tstore ";
    str += std::to_string(result.second);
    str += ", @";
    str += ident;
    str += "_";
    str += std::to_string(std::get<2>(fetch_result));
    str += "\n";
  } else {
    str += "\tstore %";
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
  std::cout << "} ";
}

std::pair<bool, int> StmtWithExpAST::Output() const {
  if (exp)
    (*exp)->Output();
  return std::pair<bool, int>(false, 0);
}

void StmtWithBlockAST::Dump() const {
  std::cout << "StmtWithBlockAST { ";
  block->Dump();
  std::cout << "} ";
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
  std::cout << "} ";
}

std::pair<bool, int> StmtWithIfAST::Output() const {
  auto result = exp->Output();

  if_cnt++;
  int cur_if = if_cnt;

  if (result.first) {
    str += "\tbr ";
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
    str += "\tbr %";
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

  if_stmt->Output();

  if (!is_block_end[cur_block]) {
    str += "\tjump %end";
    if (cur_if != 0) {
      str += "_";
      str += std::to_string(cur_if);
    }
    str += "\n";
  }

  // int else_block = cur_table + 1;
  bool if_end = is_block_end[cur_block];
  is_block_end[cur_block] = false;

  bool else_end = false;

  if (else_stmt) {
    str += "%else";
    if (cur_if != 0) {
      str += "_";
      str += std::to_string(cur_if);
    }
    str += ":\n";

    (*else_stmt)->Output();

    if (!is_block_end[cur_block]) {
      str += "\tjump %end";
      if (cur_if != 0) {
        str += "_";
        str += std::to_string(cur_if);
      }
      str += "\n";
    } else {
      else_end = true;
    }
  }

  is_block_end[cur_block] = false;

  if (if_end && (else_stmt && else_end)) {
    is_block_end[cur_block] = true;
  } else {
    str += "%end";
    if (cur_if != 0) {
      str += "_";
      str += std::to_string(cur_if);
    }
    str += ":\n";
  }

  return std::make_pair(false, 0);
}

void StmtWithWhileAST::Dump() const {
  std::cout << "StmtWithWhileAST { ";
  exp->Dump();
  stmt->Dump();
  std::cout << "} ";
}

std::pair<bool, int> StmtWithWhileAST::Output() const {
  while_level++;
  while_cnt++;
  int cur_while = while_cnt;

  level_to_cnt[while_level] = while_cnt;

  str += "\tjump %while_";
  str += std::to_string(cur_while);
  str += "_entry\n";

  str += "%while_";
  str += std::to_string(cur_while);
  str += "_entry:\n";

  auto result = exp->Output();

  if (result.first) {
    str += "\tbr ";
    str += std::to_string(result.second);
    str += ", %while_";
    str += std::to_string(cur_while);
    str += "_body, %while_";
    str += std::to_string(cur_while);
    str += "_end\n";
  } else {
    str += "\tbr %";
    str += std::to_string(cnt - 1);
    str += ", %while_";
    str += std::to_string(cur_while);
    str += "_body, %while_";
    str += std::to_string(cur_while);
    str += "_end\n";
  }

  str += "%while_";
  str += std::to_string(cur_while);
  str += "_body:\n";

  stmt->Output();

  if (!is_block_end[cur_block]) {
    str += "\tjump %while_";
    str += std::to_string(cur_while);
    str += "_entry";
    str += "\n";
  }

  is_block_end[cur_block] = false;

  str += "%while_";
  str += std::to_string(cur_while);
  str += "_end:\n";

  level_to_cnt.erase(while_level);
  while_level--;

  return std::make_pair(false, 0);
}

void StmtWithBreakAST::Dump() const {
  std::cout << "StmtWithBreakAST ";
}

std::pair<bool, int> StmtWithBreakAST::Output() const {
  if (while_level < 0)
    assert(false);

  str += "\tjump %while_";
  str += std::to_string(level_to_cnt[while_level]);
  str += "_end";
  str += "\n";

  is_block_end[cur_block] = true;

  return std::make_pair(false, 0);
}

void StmtWithContinueAST::Dump() const {
  std::cout << "StmtWithReturnAST ";
}

std::pair<bool, int> StmtWithContinueAST::Output() const {
  if (while_level < 0)
    assert(false);

  str += "\tjump %while_";
  str += std::to_string(level_to_cnt[while_level]);
  str += "_entry";
  str += "\n";

  is_block_end[cur_block] = true;

  return std::make_pair(false, 0);
}

void StmtWithReturnAST::Dump() const {
  std::cout << "StmtWithReturnAST { ";
  if (exp)
    (*exp)->Dump();
  std::cout << "} ";
}

std::pair<bool, int> StmtWithReturnAST::Output() const {
  if (exp) {
    std::pair<bool, int> result = (*exp)->Output();

    if (result.first) {
      str += "\tret ";
      str += std::to_string(result.second);
      str += "\n";
    } else {
      str += "\tret %";
      str += std::to_string(cnt - 1);
      str += "\n";
    }
  } else {
    str += "\tret\n";
  }

  is_block_end[cur_block] = true;

  return std::make_pair(false, 0);
}

void ExpAST::Dump() const {
  std::cout << "ExpAST { ";
  lOrExp->Dump();
  std::cout << "} ";
}

std::pair<bool, int> ExpAST::Output() const {
  return lOrExp->Output();
}

void LValAST::Dump() const {
  std::cout << "LValAST { ";
  std::cout << ident;
  if (exp)
    (*exp)->Dump();
  std::cout << " } ";
}

std::pair<bool, int> LValAST::Output() const {
  if (exp) {
    auto result = fetchSymbol(ident);
    auto size = (*exp)->Output();
    int exp_cnt = cnt - 1;
    if (std::get<0>(result) == UNDEFINED)
      assert(false);
    str += "\t%";
    str += std::to_string(cnt);
    cnt++;
    str += " = getelemptr @";
    str += ident;
    str += "_";
    str += std::to_string(std::get<2>(result));
    str += ", ";
    if (size.first) {
      str += std::to_string(size.second);
    } else {
      str += "%";
      str += std::to_string(exp_cnt);
    }
    str += "\n";

    str += "\t%";
    str += std::to_string(cnt);
    cnt++;
    str += " = load %";
    // cnt - 2 必为 getelemptr 返回值
    str += std::to_string(cnt - 2);
    str += "\n";

  } else {
    auto result = fetchSymbol(ident);
    if (std::get<0>(result) == CONSTANT)
      return std::pair<bool, int>(true, std::get<1>(result));
    else if (std::get<0>(result) == VARIABLE) {
      str += "\t%";
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
  }

  return std::pair<bool, int>(false, 0);
}

void PrimaryExpWithBrAST::Dump() const {
  std::cout << "PrimaryExpWithBrAST { ";
  exp->Dump();
  std::cout << "} ";
}

std::pair<bool, int> PrimaryExpWithBrAST::Output() const {
  return exp->Output();
}

void PrimaryExpWithLValAST::Dump() const {
  std::cout << "PrimaryExpWithLValAST { ";
  lVal->Dump();
  std::cout << "} ";
}

std::pair<bool, int> PrimaryExpWithLValAST::Output() const {
  return lVal->Output();
}

void PrimaryExpWithNumAST::Dump() const {
  std::cout << "PrimaryExpWithNumAST { ";
  std::cout << number;
  std::cout << " } ";
}

std::pair<bool, int> PrimaryExpWithNumAST::Output() const {
  return std::pair<bool, int>(true, number);
}

void UnaryExpAST::Dump() const {
  std::cout << "UnaryExpAST { ";
  primaryExp->Dump();
  std::cout << "} ";
}

std::pair<bool, int> UnaryExpAST::Output() const {
  return primaryExp->Output();
}

void UnaryExpWithFuncAST::Dump() const {
  std::cout << "UnaryExpWithFuncAST { ";
  std::cout << "Ident { " << ident << " } ";
  if (params)
    (*params)->Dump();
  std::cout << "} ";
}

std::pair<bool, int> UnaryExpWithFuncAST::Output() const {
  std::vector<std::pair<bool, int>> list;

  if (params) {
    list = ((FuncRParamsAST*)(*params).get())->prepare();
  }

  auto result = fetchSymbol(ident);

  switch (std::get<3>(result)) {
    case VOID: {
      str += "\tcall @";
      break;
    }
    case INT: {
      str += "\t%";
      str += std::to_string(cnt);
      cnt++;
      str += " = call @";
      break;
    }
    default:
      assert(false);
      break;
  }

  str += ident;
  str += "(";

  // 准备参数
  if (params) {
    for (int i = 0; i < list.size(); i++) {
      if (list[i].first) {
        str += std::to_string(list[i].second);
      } else {
        str += "%";
        str += std::to_string(list[i].second);
      }
      if (i != list.size() - 1)
        str += ", ";
    }
  }

  str += ")\n";

  return std::make_pair(false, 0);
}

void UnaryExpWithOpAST::Dump() const {
  std::cout << "UnaryExpWithOpAST { ";
  std::cout << "UnaryOpAST { " << unaryOp << " } ";
  unaryExp->Dump();
  std::cout << "} ";
}

std::pair<bool, int> UnaryExpWithOpAST::Output() const {
  std::pair<bool, int> result = unaryExp->Output();

  if (result.first) {
    if (unaryOp == '!') {
      str += "\t%";
      str += std::to_string(cnt);
      str += " = eq ";
      str += std::to_string(result.second);
      str += ", 0\n";
      cnt++;
    } else if (unaryOp == '-') {
      str += "\t%";
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
      str += "\t%";
      str += std::to_string(cnt);
      str += " = eq %";
      str += std::to_string(cnt - 1);
      str += ", 0\n";
      cnt++;
    } else if (unaryOp == '-') {
      str += "\t%";
      str += std::to_string(cnt);
      str += " = sub 0, %";
      str += std::to_string(cnt - 1);
      str += "\n";
      cnt++;
    }
  }

  return std::pair<bool, int>(false, 0);
}

void FuncRParamsAST::Dump() const {
  std::cout << "FuncRParamsAST { ";
  for (auto& param : paramList) {
    param->Dump();
  }
  std::cout << "} ";
}

std::pair<bool, int> FuncRParamsAST::Output() const {
  return std::make_pair(false, 0);
}

std::vector<std::pair<bool, int>> FuncRParamsAST::prepare() {
  std::vector<std::pair<bool, int>> vec;
  for (auto& param : paramList) {
    auto result = param->Output();
    int cur = cnt - 1;
    if (result.first) {
      vec.push_back(std::make_pair(true, result.second));
    } else {
      vec.push_back(std::make_pair(false, cur));
    }
  }
  return vec;
}

void MulExpAST::Dump() const {
  std::cout << "MulExpAST { ";
  unaryExp->Dump();
  std::cout << "} ";
}

std::pair<bool, int> MulExpAST::Output() const {
  return unaryExp->Output();
}

void MulExpWithOpAST::Dump() const {
  std::cout << "MulExpWithOpAST { ";
  mulExp->Dump();
  std::cout << "MulExpOpAST { " << mulOp << " } ";
  unaryExp->Dump();
  std::cout << "} ";
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
    str += "\t%";
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
    str += "\t%";
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
    str += "\t%";
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
    str += "\t%";
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
  std::cout << "} ";
}

std::pair<bool, int> AddExpAST::Output() const {
  return mulExp->Output();
}

void AddExpWithOpAST::Dump() const {
  std::cout << "AddExpWithOpAST { ";
  addExp->Dump();
  std::cout << "AddExpOpAST { " << addOp << " } ";
  mulExp->Dump();
  std::cout << "} ";
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
    str += "\t%";
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
    str += "\t%";
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
    str += "\t%";
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
    str += "\t%";
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
  std::cout << "} ";
}

std::pair<bool, int> RelExpAST::Output() const {
  return addExp->Output();
}

void RelExpWithOpAST::Dump() const {
  std::cout << "RelExpWithOpAST { ";
  relExp->Dump();
  std::cout << "RelExpOpAST { " << relOp << " } ";
  addExp->Dump();
  std::cout << "} ";
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
    str += "\t%";
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
    str += "\t%";
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
    str += "\t%";
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
    str += "\t%";
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
  std::cout << "} ";
}

std::pair<bool, int> EqExpAST::Output() const {
  return relExp->Output();
}

void EqExpWithOpAST::Dump() const {
  std::cout << "EqExpWithOpAST { ";
  eqExp->Dump();
  std::cout << "EqExpOpAST { " << eqOp << " } ";
  relExp->Dump();
  std::cout << "} ";
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
    str += "\t%";
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
    str += "\t%";
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
    str += "\t%";
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
    str += "\t%";
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
  std::cout << "} ";
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

  str += "\t%";
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
  std::cout << "EqExpOpAST { " << lAndOp << " } ";
  eqExp->Dump();
  std::cout << "} ";
}

std::pair<bool, int> LAndExpWithOpAST::Output() const {
  str += "\t%result_";
  str += std::to_string(if_cnt + 1);
  str += " = alloc i32";
  str += "\n";
  str += "\tstore 0, %result_";
  str += std::to_string(if_cnt + 1);
  str += "\n";

  if_cnt++;
  int cur_if = if_cnt;

  auto result_l = lAndExp->Output();
  int cnt_l = cnt - 1;

  if (result_l.first) {
    str += "\t%";
    str += std::to_string(cnt);
    cnt++;
    str += " = ne ";
    str += std::to_string(result_l.second);
    str += ", 0\n";
  } else {
    str += "\t%";
    str += std::to_string(cnt);
    cnt++;
    str += " = ne %";
    str += std::to_string(cnt_l);
    str += ", 0\n";
  }

  str += "\tbr %";
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
    str += "\t%";
    str += std::to_string(cnt);
    cnt++;
    str += " = ne ";
    str += std::to_string(result_r.second);
    str += ", 0\n";
  } else {
    str += "\t%";
    str += std::to_string(cnt);
    cnt++;
    str += " = ne %";
    str += std::to_string(cnt_r);
    str += ", 0\n";
  }

  str += "\tstore %";
  str += std::to_string(cnt - 1);
  str += ", %result_";
  str += std::to_string(cur_if);
  str += "\n";

  str += "\tjump %end";
  str += "_";
  str += std::to_string(cur_if);
  str += "\n";

  str += "%end";
  str += "_";
  str += std::to_string(cur_if);
  str += ":\n";

  str += "\t%";
  str += std::to_string(cnt);
  cnt++;
  str += " = load %result_";
  str += std::to_string(cur_if);
  str += "\n";

  return std::make_pair(false, 0);
}

void LOrExpAST::Dump() const {
  std::cout << "LOrExpAST { ";
  lAndExp->Dump();
  std::cout << "} ";
}

std::pair<bool, int> LOrExpAST::Output() const {
  return lAndExp->Output();
}

void LOrExpWithOpAST::Dump() const {
  std::cout << "LOrExpWithOpAST { ";
  lOrExp->Dump();
  std::cout << "LOrExpOpAST { " << lOrOp << " } ";
  lAndExp->Dump();
  std::cout << "} ";
}

std::pair<bool, int> LOrExpWithOpAST::Output() const {
  str += "\t%result_";
  str += std::to_string(if_cnt + 1);
  str += " = alloc i32";
  str += "\n";
  str += "\tstore 1, %result_";
  str += std::to_string(if_cnt + 1);
  str += "\n";

  if_cnt++;
  int cur_if = if_cnt;

  auto result_l = lOrExp->Output();
  int cnt_l = cnt - 1;

  if (result_l.first) {
    str += "\t%";
    str += std::to_string(cnt);
    cnt++;
    str += " = eq ";
    str += std::to_string(result_l.second);
    str += ", 0\n";
  } else {
    str += "\t%";
    str += std::to_string(cnt);
    cnt++;
    str += " = eq %";
    str += std::to_string(cnt_l);
    str += ", 0\n";
  }

  str += "\tbr %";
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
    str += "\t%";
    str += std::to_string(cnt);
    cnt++;
    str += " = ne ";
    str += std::to_string(result_r.second);
    str += ", 0\n";
  } else {
    str += "\t%";
    str += std::to_string(cnt);
    cnt++;
    str += " = ne %";
    str += std::to_string(cnt_r);
    str += ", 0\n";
  }

  str += "\tstore %";
  str += std::to_string(cnt - 1);
  str += ", %result_";
  str += std::to_string(cur_if);
  str += "\n";

  str += "\tjump %end";
  str += "_";
  str += std::to_string(cur_if);
  str += "\n";

  str += "%end";
  str += "_";
  str += std::to_string(cur_if);
  str += ":\n";

  str += "\t%";
  str += std::to_string(cnt);
  cnt++;
  str += " = load %result_";
  str += std::to_string(cur_if);
  str += "\n";

  return std::make_pair(false, 0);
}

void ConstExpAST::Dump() const {
  std::cout << "ConstExpAST { ";
  exp->Dump();
  std::cout << "} ";
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