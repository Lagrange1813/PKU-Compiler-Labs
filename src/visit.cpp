#include <iostream>
#include <unordered_map>

#include "visit.hpp"

using namespace std;

unordered_map<koopa_raw_value_t, string> dic;
// 记录函数所用栈空间
int stack_space = 0;
// 栈局部变量计数器
int stack_cnt = 0;
// 记录函数内有无调用
int has_call = 0;
int global_cnt = -1;

// Just use for single instructions.
int reg_cnt = 0;

// 访问 raw program
void Visit(const koopa_raw_program_t& program) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有全局变量
  Visit(program.values);
  // 访问所有函数
  Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t& slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        // 访问函数
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
}

// 访问函数
void Visit(const koopa_raw_function_t& func) {
  if (func->bbs.len == 0) return;

  // 清零函数相关变量
  stack_cnt = 0;
  has_call = 0;

  // 执行一些其他的必要操作
  cout << "\t.text\n";
  cout << "\t.globl " << func->name + 1 << "\n";
  cout << func->name + 1 << ":\n";

  // 记录函数内部局部变量数量
  int var_cnt = 0;
  int call_cnt = 0;

  for (int i = 0; i < func->bbs.len; i++) {
    const koopa_raw_slice_t& insts = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i])->insts;
    int cnt = insts.len;
    for (size_t i = 0; i < insts.len; ++i) {
      auto inst = reinterpret_cast<koopa_raw_value_t>(insts.buffer[i]);
      // 无返回值语句
      if (inst->ty->tag == KOOPA_RTT_UNIT) {
        cnt--;
      }
      // 记录函数内有无调用
      if (inst->kind.tag == KOOPA_RVT_CALL) {
        has_call = 1;
        int cur = max(int(inst->kind.data.call.args.len) - 8, 0);
        call_cnt = max(call_cnt, cur);
      }
    }
    var_cnt += cnt;
  }

  stack_cnt = call_cnt;

  // 更新栈所需空间
  stack_space = (var_cnt + has_call + call_cnt) * 4;
  if (stack_space != 0)
    cout << "\taddi sp, sp, " << -stack_space << "\n";

  if (has_call)
    cout << "\tsw ra, " << stack_space - 4 << "(sp)\n";

  // 访问所有基本块
  Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t& bb) {
  // 执行一些其他的必要操作
  if (strcmp(bb->name + 1, "entry"))
    cout << bb->name + 1 << ":\n";
  // 访问所有指令
  Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t& value) {
  // 根据指令类型判断后续需要如何访问
  const auto& kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer);
      break;
    case KOOPA_RVT_ALLOC:
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      Visit(kind.data.global_alloc, value);
      break;
    case KOOPA_RVT_LOAD:
      // 访问 load 指令
      Visit(kind.data.load, value);
      break;
    case KOOPA_RVT_STORE:
      // 访问 store 指令
      Visit(kind.data.store);
      break;
    case KOOPA_RVT_BINARY:
      // 访问 binary 指令
      Visit(kind.data.binary, value);
      break;
    case KOOPA_RVT_BRANCH:
      // 访问 branch 指令
      Visit(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      // 访问 jump 指令
      Visit(kind.data.jump);
      break;
    case KOOPA_RVT_CALL:
      // 访问 call 指令
      Visit(kind.data.call, value);
      break;
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret);
      break;
    default:
      // 其他类型暂时遇不到
      cout << "\t" << kind.tag << "\n";
      // assert(false);
      break;
  }
}

void Search(const koopa_raw_value_t value) {
  // cout << "\tTAG: " << value->kind.tag << "\n";

  if (value->kind.tag == KOOPA_RVT_INTEGER && value->kind.data.integer.value != 0) {
    cout << "\tli t" << reg_cnt << ", ";
    Visit(value->kind.data.integer);
    cout << "\n";

    dic[value] = "t" + to_string(reg_cnt);
    reg_cnt++;
  } else if (value->kind.tag == KOOPA_RVT_INTEGER && value->kind.data.integer.value == 0) {
    dic[value] = "x0";
  } else if (value->kind.tag == KOOPA_RVT_ALLOC || value->kind.tag == KOOPA_RVT_LOAD || value->kind.tag == KOOPA_RVT_BINARY || value->kind.tag == KOOPA_RVT_CALL) {
    cout << "\tlw t" << reg_cnt << ", " << dic[value] << "\n";
    dic[value] = "t" + to_string(reg_cnt);
    reg_cnt++;
  } else if (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
    size_t index = value->kind.data.func_arg_ref.index;
    if (index < 8) {
      dic[value] = "a" + to_string(index);
    } else {
      cout << "\tlw t0, " << stack_space + (index - 8) * 4 << "(sp)\n";
      dic[value] = "t0";
    }
  }
}

void Visit(const koopa_raw_global_alloc_t& global, const koopa_raw_value_t& value) {
  global_cnt++;
  cout << "\t.data\n";
  string label = "var_" + to_string(global_cnt);
  cout << "\t.globl " << label << "\n";
  cout << label << ":\n";
  switch (global.init->kind.tag) {
  case KOOPA_RVT_ZERO_INIT:
    cout << "\t.zero 4\n\n";
    break;
  case KOOPA_RVT_INTEGER:
    cout << "\t.word " << global.init->kind.data.integer.value << "\n\n";
    break;
  default:
    cout << "\tTAG: " << global.init->kind.tag << "\n";
    assert(false);
    break;
  }
  dic[value] = label;
}

void Visit(const koopa_raw_load_t& load, const koopa_raw_value_t& value) {
  if (load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    cout << "\tla t0, " << dic[load.src] << "\n";
    cout << "\tlw t0, 0(t0)" << "\n";
  } else {
    cout << "\tlw t0, " << dic[load.src] << "\n";
  }
  
  cout << "\tsw t0, " << stack_cnt * 4 << "(sp)"
       << "\n";
  dic[value] = to_string(stack_cnt * 4) + "(sp)";
  stack_cnt++;
}

void Visit(const koopa_raw_store_t& store) {
  reg_cnt = 0;
  Search(store.value);
  if (dic.find(store.dest) == dic.end()) {
    dic[store.dest] = to_string(stack_cnt * 4) + "(sp)";
    stack_cnt++;
  }

  if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    cout << "\tla t" << reg_cnt << ", " << dic[store.dest] << "\n";
    reg_cnt++;
    cout << "\tsw " << dic[store.value] << ", " << "0(t" << reg_cnt-1 << ")\n";
  } else {
    cout << "\tsw " << dic[store.value] << ", " << dic[store.dest] << "\n";
  }
}

void Visit(const koopa_raw_integer_t& integer) {
  cout << integer.value;
}

bool isNum(const koopa_raw_value_t value);

void Visit(const koopa_raw_binary_t& binary, const koopa_raw_value_t& value) {
  switch (binary.op) {
    /// Not equal to.
    case 0: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\txor t0"
           << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      cout << "\tsnez t0"
           << ", t0"
           << "\n";
      cout << "\tsw t0, " << stack_cnt * 4 << "(sp)"
           << "\n";

      dic[value] = to_string(stack_cnt * 4) + "(sp)";
      stack_cnt++;

      break;
    }

    /// Equal to.
    case 1: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\txor t0"
           << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      cout << "\tseqz t0"
           << ", t0"
           << "\n";
      cout << "\tsw t0, " << stack_cnt * 4 << "(sp)"
           << "\n";

      dic[value] = to_string(stack_cnt * 4) + "(sp)";
      stack_cnt++;

      break;
    }

    /// Greater than.
    case 2: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\tsgt t0"
           << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      cout << "\tsw t0, " << stack_cnt * 4 << "(sp)"
           << "\n";

      dic[value] = to_string(stack_cnt * 4) + "(sp)";
      stack_cnt++;

      break;
    }

    /// Less than.
    case 3: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\tslt t0"
           << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      cout << "\tsw t0, " << stack_cnt * 4 << "(sp)"
           << "\n";

      dic[value] = to_string(stack_cnt * 4) + "(sp)";
      stack_cnt++;

      break;
    }

    /// Greater than or equal to.
    case 4: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\tslt t0"
           << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      cout << "\tseqz t0"
           << ", t0"
           << "\n";
      cout << "\tsw t0, " << stack_cnt * 4 << "(sp)"
           << "\n";

      dic[value] = to_string(stack_cnt * 4) + "(sp)";
      stack_cnt++;

      break;
    }

    /// Less than or equal to.
    case 5: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\tsgt t0"
           << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      cout << "\tseqz t0, t0"
           << "\n";
      cout << "\tsw t0, " << stack_cnt * 4 << "(sp)"
           << "\n";

      dic[value] = to_string(stack_cnt * 4) + "(sp)";
      stack_cnt++;

      break;
    }

    /// Addition.
    case 6: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\tadd t0"
           << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      cout << "\tsw t0, " << stack_cnt * 4 << "(sp)"
           << "\n";

      dic[value] = to_string(stack_cnt * 4) + "(sp)";
      stack_cnt++;

      break;
    }

    /// Subtraction.
    case 7: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\tsub t0"
           << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      cout << "\tsw t0, " << stack_cnt * 4 << "(sp)"
           << "\n";

      dic[value] = to_string(stack_cnt * 4) + "(sp)";
      stack_cnt++;

      break;
    }

    /// Multiplication.
    case 8: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\tmul t0"
           << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      cout << "\tsw t0, " << stack_cnt * 4 << "(sp)"
           << "\n";

      dic[value] = to_string(stack_cnt * 4) + "(sp)";
      stack_cnt++;

      break;
    }

    /// Division.
    case 9: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\tdiv t0"
           << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      cout << "\tsw t0, " << stack_cnt * 4 << "(sp)"
           << "\n";

      dic[value] = to_string(stack_cnt * 4) + "(sp)";
      stack_cnt++;

      break;
    }

    /// Modulo.
    case 10: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\trem t0"
           << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      cout << "\tsw t0, " << stack_cnt * 4 << "(sp)"
           << "\n";

      dic[value] = to_string(stack_cnt * 4) + "(sp)";
      stack_cnt++;

      break;
    }

    /// Bitwise AND.
    case 11: {
      reg_cnt = 0;

      bool int_l = isNum(binary.lhs);
      bool int_r = isNum(binary.rhs);

      if (int_l && int_r) {
        cout << "\tli t" << reg_cnt << ", ";
        Visit(binary.lhs->kind.data.integer);
        cout << "\n";

        dic[binary.lhs] = "t" + to_string(reg_cnt);
        reg_cnt++;

        cout << "\tandi t0"
             << ", " << dic[binary.lhs] << ", ";
        Visit(binary.lhs->kind.data.integer);
        cout << "\n";
      } else if (int_l && !int_r) {
        cout << "\tandi t0"
             << ", " << dic[binary.rhs] << ", ";
        Visit(binary.lhs->kind.data.integer);
        cout << "\n";
      } else if (!int_l && int_r) {
        cout << "\tandi t0"
             << ", " << dic[binary.lhs] << ", ";
        Visit(binary.rhs->kind.data.integer);
        cout << "\n";
      } else {
        cout << "\tand t0"
             << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      }

      cout << "\tsw t0, " << stack_cnt * 4 << "(sp)"
           << "\n";

      dic[value] = to_string(stack_cnt * 4) + "(sp)";
      stack_cnt++;

      break;
    }

    /// Bitwise OR.
    case 12: {
      reg_cnt = 0;

      bool int_l = isNum(binary.lhs);
      bool int_r = isNum(binary.rhs);

      if (int_l && int_r) {
        cout << "\tli t" << reg_cnt << ", ";
        Visit(binary.lhs->kind.data.integer);
        cout << "\n";

        dic[binary.lhs] = "t" + to_string(reg_cnt);
        reg_cnt++;

        cout << "\tori t0"
             << ", " << dic[binary.lhs] << ", ";
        Visit(binary.lhs->kind.data.integer);
        cout << "\n";
      } else if (int_l && !int_r) {
        cout << "\tori t0"
             << ", " << dic[binary.rhs] << ", ";
        Visit(binary.lhs->kind.data.integer);
        cout << "\n";
      } else if (!int_l && int_r) {
        cout << "\tori t0"
             << ", " << dic[binary.lhs] << ", ";
        Visit(binary.rhs->kind.data.integer);
        cout << "\n";
      } else {
        cout << "\tor t0"
             << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      }

      cout << "\tsw t0, " << stack_cnt * 4 << "(sp)"
           << "\n";

      dic[value] = to_string(stack_cnt * 4) + "(sp)";
      stack_cnt++;

      break;
    }

    default:
      break;
  }
}

bool isNum(const koopa_raw_value_t value) {
  if (value->kind.tag == KOOPA_RVT_INTEGER) {
    return true;
  }
  cout << "\tlw t" << reg_cnt << ", " << dic[value] << "\n";
  dic[value] = "t" + to_string(reg_cnt);
  reg_cnt++;
  return false;
}

void Visit(const koopa_raw_branch_t& branch) {
  reg_cnt = 0;
  Search(branch.cond);
  cout << "\tbnez " << dic[branch.cond] << ", " << branch.true_bb->name + 1 << "\n";
  cout << "\tj " << branch.false_bb->name + 1 << "\n";
}

void Visit(const koopa_raw_jump_t& jump) {
  cout << "\tj " << jump.target->name + 1 << "\n";
}

void Visit(const koopa_raw_call_t& call, const koopa_raw_value_t& value) {
  for (int i = 0; i < min(int(call.args.len), 8); i++) {
    if (reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i])->kind.tag == KOOPA_RVT_INTEGER)
      cout << "\tli a" << i << ", " << reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i])->kind.data.integer.value << "\n";
    else {
      cout << "\tlw t0, " << dic[reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i])] << "\n";
      cout << "\tmv a" << i << ", t0\n";
    }
  }

  for (int i = 8; i < call.args.len; i++) {
    if (reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i])->kind.tag == KOOPA_RVT_INTEGER) {
      cout << "\tli t0"
           << ", " << reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i])->kind.data.integer.value << "\n";
      cout << "\tsw t0"
           << ", " << (i - 8) * 4 << "(sp)\n";
    } else {
      cout << "\tmv t0"
           << ", " << dic[reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i])] << "\n";
      cout << "\tsw t0"
           << ", " << (i - 8) * 4 << "(sp)\n";
    }
  }

  cout << "\tcall " << call.callee->name + 1 << "\n";

  // 根据ir是否保存返回值决定是否保存a0
  if (value->ty->tag != KOOPA_RTT_UNIT) {
    cout << "\tsw a0, " << stack_cnt * 4 << "(sp)\n";
    dic[value] = std::to_string(stack_cnt*4) + "(sp)";
    stack_cnt++;
  }
}

void Visit(const koopa_raw_return_t& ret) {
  if (ret.value != nullptr) {
    if (ret.value->kind.tag == KOOPA_RVT_INTEGER) {
      cout << "\tli a0, ";
      Visit(ret.value->kind.data.integer);
      cout << "\n";
    } else if (ret.value->kind.tag == KOOPA_RVT_BINARY || ret.value->kind.tag == KOOPA_RVT_LOAD || ret.value->kind.tag == KOOPA_RVT_CALL) {
      cout << "\tlw a0, " << dic[ret.value] << "\n";
    } else {
      cout << "\tERROR: Undefined Tag: " << ret.value->kind.tag << "\n";
    }
  }

  if (has_call)
    cout << "\tlw ra, " << stack_space - 4 << "(sp)\n";

  if (stack_space != 0)
    cout << "\taddi sp, sp, " << stack_space << "\n";
  cout << "\tret\n\n";
}