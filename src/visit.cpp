#include <iostream>

#include "visit.hpp"

using namespace std;

int asm_cnt = 0;

// 访问 raw program
void Visit(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有全局变量
  Visit(program.values);
  // 访问所有函数
  Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice) {
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
void Visit(const koopa_raw_function_t &func) {
  // 执行一些其他的必要操作
  cout << "  .text\n";
  cout << "  .globl " << func->name + 1 << "\n";
  cout << func->name + 1 << ":\n";
  // 访问所有基本块
  Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // cout << bb->name << "\n";
  // 访问所有指令
  Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
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
      Visit(kind.data.binary);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
      break;
  }
}

void Visit(const koopa_raw_return_t &ret) {
  // assert(ret.value->kind.tag == KOOPA_RVT_INTEGER);
  if (asm_cnt == 0) {
    cout << "  li a0, ";
    Visit(ret.value->kind.data.integer);
    cout << "\n";
  } else {
    cout << "  mv    a0, " << "t" << asm_cnt - 1 << "\n";
  }
  cout<< "  ret" << "\n";
}

void Visit(const koopa_raw_integer_t &integer) {
  cout<< integer.value;
}

void Visit(const koopa_raw_binary_t &binary) {
  switch (binary.op) {
  case 1:

    // 1
    cout << "  li    " << "t" << asm_cnt << ", ";
    Visit(binary.lhs);
    cout << "\n";
    // 2
    cout << "  xor   " << "t" << asm_cnt << ", t" << asm_cnt << ", x0\n";
    // 3
    cout << "  seqz  " << "t" << asm_cnt << ", t" << asm_cnt << "\n";

    asm_cnt++;

    break;

  case 7:

    cout << "  sub   t" << asm_cnt << ", x0, t" << asm_cnt - 1 << "\n";

    asm_cnt++;

  default:
    break;
  }
}
