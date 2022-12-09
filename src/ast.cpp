#include "ast.hpp"

void CompUnitAST::Dump() const {
  std::cout << "CompUnitAST { ";
  func_def->Dump();
  std::cout << " }";
}

std::pair<bool, int> CompUnitAST::Output() const {
  func_def->Output();
  return std::pair<bool, int>(false, 0);
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
  block->Output();

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
  stmt->Dump();
  std::cout << " }";
}

std::pair<bool, int> BlockAST::Output() const {
  str += "{\n";
  str += "\%entry:\n";
  stmt->Output();
  str += "}";

  return std::pair<bool, int>(false, 0);
}

void StmtAST::Dump() const {
  std::cout << "StmtAST { ";
  exp->Dump();
  std::cout << " }";
}

std::pair<bool, int> StmtAST::Output() const {
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

void ExpAST::Dump() const {
  std::cout << "ExpAST { ";
  lOrExp->Dump();
  std::cout << " }";
}

std::pair<bool, int> ExpAST::Output() const {
  return lOrExp->Output();
}

void PrimaryExpWithBrAST::Dump() const {
  std::cout << "PrimaryExpWithBrAST { ";
  exp->Dump();
  std::cout << " }";
}

std::pair<bool, int> PrimaryExpWithBrAST::Output() const {
  return exp->Output();
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
  std::pair<bool, int> result_l = lAndExp->Output();
  int cnt_l = cnt - 1;

  std::pair<bool, int> result_r = eqExp->Output();
  int cnt_r = cnt - 1;

  if (result_l.first && result_r.first) {
    bool sign_l = filter(result_l.second);
    bool sign_r = filter(result_r.second);

    str += "  %";
    str += std::to_string(cnt);
    str += " = ";
    str += "and";
    str += " ";

    if (sign_l) {
      str += std::to_string(result_l.second);
    } else if (!sign_l && sign_r) {
      str += "%";
      str += std::to_string(cnt - 1);
    } else {
      str += "%";
      str += std::to_string(cnt - 2);
    }

    str += ", ";

    if (sign_r) {
      str += std::to_string(result_r.second);
    } else {
      str += "%";
      str += std::to_string(cnt - 1);
    }

    str += "\n";
    cnt++;

  } else if (result_l.first) {
    bool sign_l = filter(result_l.second);

    str += "  %";
    str += std::to_string(cnt);
    str += " = ";
    str += "and";
    str += " ";

    if (sign_l) {
      str += std::to_string(result_l.second);
    } else {
      str += "%";
      str += std::to_string(cnt - 1);
    }

    str += ", %";
    str += std::to_string(cnt_r);
    str += "\n";
    cnt++;

  } else if (result_r.first) {
    bool sign_r = filter(result_r.second);

    str += "  %";
    str += std::to_string(cnt);
    str += " = ";
    str += "or";
    str += " %";
    str += std::to_string(cnt_l);
    str += ", ";

    if (sign_r) {
      str += std::to_string(result_r.second);
    } else {
      str += "%";
      str += std::to_string(cnt - 1);
    }

    str += "\n";
    cnt++;

  } else {
    str += "  %";
    str += std::to_string(cnt);
    str += " = ";
    str += "and";
    str += " %";
    str += std::to_string(cnt_l);
    str += ", %";
    str += std::to_string(cnt_r);
    str += "\n";
    cnt++;
  }

  return std::pair<bool, int>(false, 0);
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
  std::pair<bool, int> result_l = lOrExp->Output();
  int cnt_l = cnt - 1;

  std::pair<bool, int> result_r = lAndExp->Output();
  int cnt_r = cnt - 1;

  if (result_l.first && result_r.first) {
    bool sign_l = filter(result_l.second);
    bool sign_r = filter(result_r.second);

    str += "  %";
    str += std::to_string(cnt);
    str += " = ";
    str += "or";
    str += " ";

    if (sign_l) {
      str += std::to_string(result_l.second);
    } else if (!sign_l && sign_r) {
      str += "%";
      str += std::to_string(cnt - 1);
    } else {
      str += "%";
      str += std::to_string(cnt - 2);
    }

    str += ", ";

    if (sign_r) {
      str += std::to_string(result_r.second);
    } else {
      str += "%";
      str += std::to_string(cnt - 1);
    }

    str += "\n";
    cnt++;

  } else if (result_l.first) {
    bool sign_l = filter(result_l.second);

    str += "  %";
    str += std::to_string(cnt);
    str += " = ";
    str += "or";
    str += " ";

    if (sign_l) {
      str += std::to_string(result_l.second);
    } else {
      str += "%";
      str += std::to_string(cnt - 1);
    }

    str += ", %";
    str += std::to_string(cnt_r);
    str += "\n";
    cnt++;

  } else if (result_r.first) {
    bool sign_r = filter(result_r.second);

    str += "  %";
    str += std::to_string(cnt);
    str += " = ";
    str += "or";
    str += " %";
    str += std::to_string(cnt_l);
    str += ", ";

    if (sign_r) {
      str += std::to_string(result_r.second);
    } else {
      str += "%";
      str += std::to_string(cnt - 1);
    }

    str += "\n";
    cnt++;

  } else {
    str += "  %";
    str += std::to_string(cnt);
    str += " = ";
    str += "or";
    str += " %";
    str += std::to_string(cnt_l);
    str += ", %";
    str += std::to_string(cnt_r);
    str += "\n";
    cnt++;
  }

  return std::pair<bool, int>(false, 0);
}