#include <iostream>
#include <unordered_map>

#include "visit.hpp"

using namespace std;

int asm_cnt = 0;
unordered_map<koopa_raw_value_t, string> dic;
int stack_space = 0;
int stack_cnt = 0;
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
  // 执行一些其他的必要操作
  cout << "  .text\n";
  cout << "  .globl " << func->name + 1 << "\n";
  cout << func->name + 1 << ":\n";

  const koopa_raw_slice_t& insts = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[0])->insts;
  int cnt = insts.len;
  for (size_t i = 0; i < insts.len; ++i) {
    auto inst = reinterpret_cast<koopa_raw_value_t>(insts.buffer[i]);
    if (inst->ty->tag == KOOPA_RTT_UNIT) {
      cnt--;
    }
  }
  stack_space = cnt * 4;
  cout << "  addi sp, sp, " << -(cnt * 4) << "\n";

  // 访问所有基本块
  Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t& bb) {
  // 执行一些其他的必要操作
  // cout << bb->name << "\n";
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
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret);
      break;
    default:
      // 其他类型暂时遇不到
      // cout << kind.tag << "\n";
      assert(false);
      break;
  }
}

bool Search(const koopa_raw_value_t value) {
  // cout << "  TAG: " << value->kind.tag << "\n";

  if (value->kind.tag == KOOPA_RVT_INTEGER && value->kind.data.integer.value != 0) {
    cout << "  li t" << reg_cnt << ", ";
    Visit(value->kind.data.integer);
    cout << "\n";

    dic[value] = "t" + to_string(reg_cnt);
    reg_cnt++;

    return false;
  } else if (value->kind.tag == KOOPA_RVT_INTEGER && value->kind.data.integer.value == 0) {
    dic[value] = "x0";
    return true;
  } else if (value->kind.tag == KOOPA_RVT_ALLOC || value->kind.tag == KOOPA_RVT_LOAD || value->kind.tag == KOOPA_RVT_BINARY) {
    cout << "  lw t" << reg_cnt << ", " << dic[value] << "\n";
    dic[value] = "t" + to_string(reg_cnt);
    reg_cnt++;
    return false;
  }

  return false;
}

bool isNum(const koopa_raw_value_t value) {
  if (value->kind.tag == KOOPA_RVT_INTEGER) {
    return true;
  }
  return false;
}

void Visit(const koopa_raw_load_t& load, const koopa_raw_value_t& value) {
  cout << "  lw t0, " << dic[load.src] << "\n";
  cout << "  sw t0, " << stack_cnt * 4 << "(sp)"
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
  cout << "  sw " << dic[store.value] << ", " << dic[store.dest] << "\n";
  
}

void Visit(const koopa_raw_return_t& ret) {
  if (ret.value->kind.tag == KOOPA_RVT_INTEGER) {
    cout << "  li a0, ";
    Visit(ret.value->kind.data.integer);
    cout << "\n";
  } else if (ret.value->kind.tag == KOOPA_RVT_BINARY || ret.value->kind.tag == KOOPA_RVT_LOAD) {
    cout << "  lw a0, " << dic[ret.value] << "\n";
  } else {
    cout << "  ERROR: Undefined Tag: " << ret.value->kind.tag << "\n";
  }

  cout << "  addi sp, sp, " << stack_space << "\n";
  cout << "  ret"
       << "\n";
}

void Visit(const koopa_raw_integer_t& integer) {
  cout << integer.value;
}

void Visit(const koopa_raw_binary_t& binary, const koopa_raw_value_t& value) {
  switch (binary.op) {
    /// Not equal to.
    case 0: {
      bool zero_l = Search(binary.lhs);
      bool zero_r = Search(binary.rhs);

      int cur = asm_cnt - 1;

      if (zero_l && zero_r) {
        cur = asm_cnt;
        asm_cnt++;
      }

      cout << "  xor t" << cur << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      cout << "  snez t" << cur << ", t" << cur << "\n";

      dic[value] = "t" + to_string(cur);

      break;
    }

    /// Equal to.
    case 1: {
      bool zero_l = Search(binary.lhs);
      bool zero_r = Search(binary.rhs);

      int cur = asm_cnt - 1;

      if (zero_l && zero_r) {
        cur = asm_cnt;
        asm_cnt++;
      }

      cout << "  xor t" << cur << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      cout << "  seqz t" << cur << ", t" << cur << "\n";

      dic[value] = "t" + to_string(cur);

      break;
    }

    /// Greater than.
    case 2: {
      bool zero_l = Search(binary.lhs);
      bool zero_r = Search(binary.rhs);

      int cur = asm_cnt - 1;

      if (zero_l && zero_r) {
        cur = asm_cnt;
        asm_cnt++;
      }

      cout << "  sgt t" << cur << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";

      dic[value] = "t" + to_string(cur);

      break;
    }

    /// Less than.
    case 3: {
      bool zero_l = Search(binary.lhs);
      bool zero_r = Search(binary.rhs);

      int cur = asm_cnt - 1;

      if (zero_l && zero_r) {
        cur = asm_cnt;
        asm_cnt++;
      }

      cout << "  slt t" << cur << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";

      dic[value] = "t" + to_string(cur);

      break;
    }

    /// Greater than or equal to.
    case 4: {
      bool zero_l = Search(binary.lhs);
      bool zero_r = Search(binary.rhs);

      int cur = asm_cnt - 1;

      if (zero_l && zero_r) {
        cur = asm_cnt;
        asm_cnt++;
      }

      cout << "  slt t" << cur << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      cout << "  seqz t" << cur << ", t" << cur << "\n";

      dic[value] = "t" + to_string(cur);

      break;
    }

    /// Less than or equal to.
    case 5: {
      bool zero_l = Search(binary.lhs);
      bool zero_r = Search(binary.rhs);

      int cur = asm_cnt - 1;

      if (zero_l && zero_r) {
        cur = asm_cnt;
        asm_cnt++;
      }

      cout << "  sgt t" << cur << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      cout << "  seqz t" << cur << ", t" << cur << "\n";

      dic[value] = "t" + to_string(cur);

      break;
    }

    /// Addition.
    case 6: {
      reg_cnt = 0;
      
      Search(binary.lhs);
      Search(binary.rhs);

      cout << "  add t0" << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      cout << "  sw t0, " << stack_cnt * 4 << "(sp)" << "\n";

      dic[value] =  to_string(stack_cnt * 4) + "(sp)";
      stack_cnt++;

      break;
    }

    /// Subtraction.
    case 7: {
      bool zero_l = Search(binary.lhs);
      bool zero_r = Search(binary.rhs);

      int cur = asm_cnt - 1;

      if (zero_l && zero_r) {
        cur = asm_cnt;
        asm_cnt++;
      }

      cout << "  sub t" << cur << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";

      dic[value] = "t" + to_string(cur);

      break;
    }

    /// Multiplication.
    case 8: {
      bool zero_l = Search(binary.lhs);
      bool zero_r = Search(binary.rhs);

      int cur = asm_cnt - 1;

      if (zero_l && zero_r) {
        cur = asm_cnt;
        asm_cnt++;
      }

      cout << "  mul t" << cur << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";

      dic[value] = "t" + to_string(cur);

      break;
    }

    /// Division.
    case 9: {
      bool zero_l = Search(binary.lhs);
      bool zero_r = Search(binary.rhs);

      int cur = asm_cnt - 1;

      if (zero_l && zero_r) {
        cur = asm_cnt;
        asm_cnt++;
      }

      cout << "  div t" << cur << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";

      dic[value] = "t" + to_string(cur);

      break;
    }

    /// Modulo.
    case 10: {
      bool zero_l = Search(binary.lhs);
      bool zero_r = Search(binary.rhs);

      int cur = asm_cnt - 1;

      if (zero_l && zero_r) {
        cur = asm_cnt;
        asm_cnt++;
      }

      cout << "  rem t" << cur << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";

      dic[value] = "t" + to_string(cur);

      break;
    }

    /// Bitwise AND.
    case 11: {
      bool int_l = isNum(binary.lhs);
      bool int_r = isNum(binary.rhs);

      if (int_l && int_r) {
        cout << "  li t" << asm_cnt << ", ";
        Visit(binary.lhs->kind.data.integer);
        cout << "\n";

        dic[binary.lhs] = "t" + to_string(asm_cnt);
        asm_cnt++;

        cout << "  andi t" << asm_cnt - 1 << ", " << dic[binary.lhs] << ", ";
        Visit(binary.lhs->kind.data.integer);
        cout << "\n";
      } else if (int_l && !int_r) {
        cout << "  andi t" << asm_cnt - 1 << ", " << dic[binary.rhs] << ", ";
        Visit(binary.lhs->kind.data.integer);
        cout << "\n";
      } else if (!int_l && int_r) {
        cout << "  andi t" << asm_cnt - 1 << ", " << dic[binary.lhs] << ", ";
        Visit(binary.rhs->kind.data.integer);
        cout << "\n";
      } else {
        cout << "  and t" << asm_cnt - 1 << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      }

      dic[value] = "t" + to_string(asm_cnt - 1);

      break;
    }

    /// Bitwise OR.
    case 12: {
      bool int_l = isNum(binary.lhs);
      bool int_r = isNum(binary.rhs);

      if (int_l && int_r) {
        cout << "  li t" << asm_cnt << ", ";
        Visit(binary.lhs->kind.data.integer);
        cout << "\n";

        dic[binary.lhs] = "t" + to_string(asm_cnt);
        asm_cnt++;

        cout << "  ori t" << asm_cnt - 1 << ", " << dic[binary.lhs] << ", ";
        Visit(binary.lhs->kind.data.integer);
        cout << "\n";
      } else if (int_l && !int_r) {
        cout << "  ori t" << asm_cnt - 1 << ", " << dic[binary.rhs] << ", ";
        Visit(binary.lhs->kind.data.integer);
        cout << "\n";
      } else if (!int_l && int_r) {
        cout << "  ori t" << asm_cnt - 1 << ", " << dic[binary.lhs] << ", ";
        Visit(binary.rhs->kind.data.integer);
        cout << "\n";
      } else {
        cout << "  or t" << asm_cnt - 1 << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";
      }

      dic[value] = "t" + to_string(asm_cnt - 1);

      break;
    }

    default:
      break;
  }
}