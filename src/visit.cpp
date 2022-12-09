#include <iostream>
#include <unordered_map>

#include "visit.hpp"

using namespace std;

int asm_cnt = 0;
unordered_map<koopa_raw_value_t, string> dic;

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
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer);
      break;
    case KOOPA_RVT_BINARY:
      // 访问 binary 指令
      Visit(kind.data.binary, value);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
      break;
  }
}

void Visit(const koopa_raw_return_t& ret) {
  if (ret.value->kind.tag == KOOPA_RVT_INTEGER) {
    cout << "  li a0, ";
    Visit(ret.value->kind.data.integer);
    cout << "\n";
  } else {
    cout << "  mv a0, "
         << "t" << asm_cnt - 1 << "\n";
  }

  cout << "  ret"
       << "\n";
}

void Visit(const koopa_raw_integer_t& integer) {
  cout << integer.value;
}

bool Search(const koopa_raw_value_t value) {
  if (asm_cnt == 7) asm_cnt = 0;

  if (value->kind.tag == KOOPA_RVT_INTEGER && value->kind.data.integer.value != 0) {
    cout << "  li t" << asm_cnt << ", ";
    Visit(value->kind.data.integer);
    cout << "\n";

    dic[value] = "t" + to_string(asm_cnt);
    asm_cnt++;

    return false;
  } else if (value->kind.tag == KOOPA_RVT_INTEGER && value->kind.data.integer.value == 0) {
    dic[value] = "x0";
    return true;
  }

  return false;
}

bool isNum(const koopa_raw_value_t value) {
  if (value->kind.tag == KOOPA_RVT_INTEGER) {
    return true;
  }
  return false;
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
      bool zero_l = Search(binary.lhs);
      bool zero_r = Search(binary.rhs);

      int cur = asm_cnt - 1;

      if (zero_l && zero_r) {
        cur = asm_cnt;
        asm_cnt++;
      }

      cout << "  add t" << cur << ", " << dic[binary.lhs] << ", " << dic[binary.rhs] << "\n";

      dic[value] = "t" + to_string(cur);

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