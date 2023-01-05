#include <iostream>
#include <string>
#include <unordered_map>

#include "visit.hpp"

using namespace std;

typedef enum {
  REGISTER,
  STACK
} loca_type;

typedef struct {
  loca_type type;
  string reg_name;
  int stack;
} loca_object;

unordered_map<koopa_raw_value_t, loca_object*> lookup;

void saveLocation(koopa_raw_value_t value, loca_type type, string reg_name, int stack) {
  loca_object* target = new loca_object();
  if (type == REGISTER) {
    target->type = REGISTER;
    target->reg_name = reg_name;
  } else if (type == STACK) {
    target->type = STACK;
    target->stack = stack;
  } else {
    assert(false);
  }
  lookup[value] = target;
}

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

int Calculate(const koopa_raw_type_t target) {
  if (target->tag == KOOPA_RTT_ARRAY)
    return target->data.array.len * Calculate(target->data.array.base);
  else
    return 1;
  return 0;
}

// 访问函数
void Visit(const koopa_raw_function_t& func) {
  if (func->bbs.len == 0)
    return;

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
      if (inst->kind.tag == KOOPA_RVT_ALLOC) {
        if (inst->ty->tag == KOOPA_RTT_POINTER)
          cnt += Calculate(inst->ty->data.pointer.base);
        else
          assert(false);
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
  if (stack_space != 0) {
    if (stack_space <= 2048 && stack_space > -2048) {
      cout << "\taddi sp, sp, " << -stack_space << "\n";
    } else {
      cout << "\tli t0, " << -stack_space << "\n";
      cout << "\tadd sp, sp, t0"
           << "\n";
    }
  }

  if (has_call) {
    if (stack_space < 2052 && stack_space >= -2044) {
      cout << "\tsw ra, " << stack_space - 4 << "(sp)\n";
    } else {
      cout << "\tli t0, " << stack_space - 4 << "\n";
      cout << "\tadd t0, t0, sp\n";
      cout << "\tsw ra, 0(t0)\n";
    }
  }
  // cout << "\tsw ra, " << stack_space - 4 << "(sp)\n";

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
    case KOOPA_RVT_ALLOC: {
      // dic[value] = std::to_string(stack_cnt * 4) + "(sp)";
      saveLocation(value, STACK, "", stack_cnt * 4);
      stack_cnt++;
      if (value->ty->tag == KOOPA_RTT_POINTER)
        stack_cnt += Calculate(value->ty->data.pointer.base);
      else
        assert(false);
      break;
    }
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
    case KOOPA_RVT_GET_ELEM_PTR:
      // 访问 get_elem 指令
      Visit(kind.data.get_elem_ptr, value);
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

    // dic[value] = "t" + to_string(reg_cnt);
    saveLocation(value, REGISTER, "t" + to_string(reg_cnt), 0);

    reg_cnt++;
  } else if (value->kind.tag == KOOPA_RVT_INTEGER && value->kind.data.integer.value == 0) {
    // dic[value] = "x0";
    saveLocation(value, REGISTER, "x0", 0);
  } else if (value->kind.tag == KOOPA_RVT_ALLOC || value->kind.tag == KOOPA_RVT_LOAD || value->kind.tag == KOOPA_RVT_BINARY || value->kind.tag == KOOPA_RVT_CALL) {
    int loca = lookup[value]->stack;
    if (loca < 2048 && loca >= -2048) {
      cout << "\tlw t" << reg_cnt << ", " << loca << "(sp)\n";
    } else {
      cout << "\tli t1, " << loca << "\n";
      cout << "\tadd t1, t1, sp\n";
      cout << "\tlw t0, 0(t1)\n";
    }
    saveLocation(value, REGISTER, "t" + to_string(reg_cnt), 0);
    reg_cnt++;
  } else if (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
    // assert(false);
    size_t index = value->kind.data.func_arg_ref.index;
    if (index < 8) {
      saveLocation(value, REGISTER, "a" + to_string(index), 0);
    } else {
      cout << "\tlw t0, " << stack_space + (index - 8) * 4 << "(sp)\n";
      saveLocation(value, REGISTER, "t0", 0);
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
    case KOOPA_RVT_ZERO_INIT: {
      if (global.init->ty->tag == KOOPA_RTT_ARRAY)
        cout << "\t.zero " << 4 * global.init->ty->data.array.len << "\n\n";
      else if (global.init->ty->tag == KOOPA_RTT_INT32)
        cout << "\t.zero 4\n\n";
      break;
    }
    case KOOPA_RVT_INTEGER:
      cout << "\t.word " << global.init->kind.data.integer.value << "\n\n";
      break;
    case KOOPA_RVT_AGGREGATE: {
      auto size = global.init->kind.data.aggregate.elems.len;
      auto buffer = global.init->kind.data.aggregate.elems.buffer;
      for (auto i = 0; i < size; i++) {
        auto value = reinterpret_cast<koopa_raw_value_t>(buffer[i])->kind.data.integer.value;
        cout << "\t.word " << value << "\n";
      }
      cout << "\n";
      break;
    }
    default:
      cout << "\tTAG: " << global.init->kind.tag << "\n";
      assert(false);
      break;
  }
  // dic[value] = label;
  saveLocation(value, REGISTER, label, 0);
}

void Visit(const koopa_raw_load_t& load, const koopa_raw_value_t& value) {
  // if (load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
  //   cout << "\tla t0, " << dic[load.src] << "\n";
  //   cout << "\tlw t0, 0(t0)"
  //        << "\n";
  // } else {
  //   cout << "\tlw t0, " << dic[load.src] << "\n";
  // }

  switch (load.src->kind.tag) {
    case KOOPA_RVT_GLOBAL_ALLOC: {
      // cout << "\tla t0, " << dic[load.src] << "\n";
      cout << "\tla t0, " << lookup[load.src]->reg_name << "\n";
      cout << "\tlw t0, 0(t0)\n";
      break;
    }
    case KOOPA_RVT_GET_ELEM_PTR: {
      auto target = lookup[load.src];
      if (target->type == STACK) {
        if (target->stack < 2048 && target->stack >= -2048) {
          cout << "\tlw t0, " << lookup[load.src]->stack << "(sp)\n";
        } else {
          cout << "\tli t0, " << lookup[load.src]->stack << "\n";
          cout << "\tadd t0, t0, sp\n";
          cout << "\tlw t0, 0(t0)\n";
        }
      } else {
        assert(false);
        cout << "\tlw t0, " << lookup[load.src]->reg_name << "\n";
      }

      cout << "\tlw t0, 0(t0)\n";
      break;
    }
    default: {
      // cout << "\tlw t0, " << dic[load.src] << "\n";
      auto target = lookup[load.src];
      if (target->type == STACK) {
        if (target->stack < 2048 && target->stack >= -2048) {
          cout << "\tlw t0, " << target->stack << "(sp)\n";
        } else {
          cout << "\tli t0, " << target->stack << "\n";
          cout << "\tadd t0, t0, sp\n";
          cout << "\tlw t0, 0(t0)\n";
        }
      } else {
        assert(false);
        cout << "\tlw t0, " << lookup[load.src]->reg_name << "\n";
      }
      break;
    }
  }

  int target = stack_cnt * 4;
  if (target < 2048 && target >= -2048) {
    cout << "\tsw t0, " << target << "(sp)"
         << "\n";
  } else {
    cout << "\tli t1, " << target << "\n";
    cout << "\tadd t1, t1, sp\n";
    cout << "\tsw t0, 0(t1)\n";
  }

  // dic[value] = to_string(stack_cnt * 4) + "(sp)";
  saveLocation(value, STACK, "", stack_cnt * 4);
  stack_cnt++;
}

void Visit(const koopa_raw_store_t& store) {
  reg_cnt = 0;
  Search(store.value);
  // if (dic.find(store.dest) == dic.end()) {
  //   dic[store.dest] = to_string(stack_cnt * 4) + "(sp)";
  //   stack_cnt++;
  // }

  // if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
  //   cout << "\tla t" << reg_cnt << ", " << dic[store.dest] << "\n";
  //   reg_cnt++;
  //   cout << "\tsw " << dic[store.value] << ", "
  //        << "0(t" << reg_cnt - 1 << ")\n";
  // } else {
  //   cout << "\tsw " << dic[store.value] << ", " << dic[store.dest] << "\n";
  // }

  switch (store.dest->kind.tag) {
    case KOOPA_RVT_GLOBAL_ALLOC: {
      cout << "\tla t" << reg_cnt << ", " << lookup[store.dest]->reg_name << "\n";
      reg_cnt++;
      cout << "\tsw " << lookup[store.value]->reg_name << ", "
           << "0(t" << reg_cnt - 1 << ")\n";
      break;
    }
    case KOOPA_RVT_GET_ELEM_PTR: {
      auto target = lookup[store.dest];
      if (target->type == STACK) {
        if (target->stack < 2048 && target->stack >= -2048) {
          cout << "\tlw t" << reg_cnt << ", " << target->stack << "(sp)\n";
          reg_cnt++;
        } else {
          cout << "\tli t" << reg_cnt << ", " << target->stack << "\n";
          reg_cnt++;
          cout << "\tadd t" << reg_cnt << ", t" << reg_cnt << ", sp\n";
          cout << "\tlw t" << reg_cnt << ", 0(t" << reg_cnt - 1 << ")\n";
          reg_cnt++;
        }
      } else {
        assert(false);
        cout << "\tlw t0, " << target->reg_name << "\n";
      }

      cout << "\tsw " << lookup[store.value]->reg_name << ", "
           << "0(t" << reg_cnt - 1 << ")\n";

      break;
    }
    default: {
      auto target = lookup[store.dest];
      if (target->type == STACK) {
        if (target->stack < 2048 && target->stack >= -2048) {
          cout << "\tsw " << lookup[store.value]->reg_name << ", " << target->stack << "(sp)\n";
        } else {
          cout << "\tli t" << reg_cnt << ", " << target->stack << "\n";
          cout << "\tadd t" << reg_cnt << ", t" << reg_cnt << ", sp\n";
          reg_cnt++;
          cout << "\tsw " << lookup[store.value]->reg_name << ", 0(t" << reg_cnt - 1 << ")\n";
        }
      } else {
        assert(false);
      }
      break;
    }
  }
}

void Visit(const koopa_raw_integer_t& integer) {
  cout << integer.value;
}

void Visit(const koopa_raw_get_ptr_t& target, const koopa_raw_value_t& value) {
  reg_cnt = 0;
  if (target.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    cout << "\tla t0, " << lookup[target.src]->reg_name << "\n";
    cout << "\tli t1, " << target.index->kind.data.integer.value << "\n";
    cout << "\tli t2, 4"
         << "\n";
    cout << "\tmul t1, t1, t2"
         << "\n";
    cout << "\tadd t0, t0, t1"
         << "\n";

    int size = stack_cnt * 4;
    stack_cnt++;
    if (size < 2048 && size >= -2048) {
      cout << "\tsw t0, " << size << "(sp)\n";
    } else {
      cout << "\tli t1, " << size << "\n";
      cout << "\tadd t1, t1, sp\n";
      cout << "\tsw t0, 0(t1)\n";
    }
    saveLocation(value, STACK, "", size);
  } else {
    int size = lookup[target.src]->stack;
    if (size < 2048 && size >= -2048) {
      cout << "\taddi t0, sp, " << size << "\n";
    } else {
      cout << "\tli t1, " << size << "\n";
      cout << "\taddi t0, sp, t1\n";
    }
    cout << "\tli t1, " << target.index->kind.data.integer.value << "\n";
    cout << "\tli t2, 4"
         << "\n";
    cout << "\tmul t1, t1, t2"
         << "\n";
    cout << "\tadd t0, t0, t1"
         << "\n";
    int loca = stack_cnt * 4;
    stack_cnt++;
    if (loca < 2048 && loca >= -2048) {
      cout << "\tsw t0, " << loca << "(sp)\n";
    } else {
      cout << "\tli t1, " << loca << "\n";
      cout << "\tadd t1, t1, sp\n";
      cout << "\tsw t0, 0(t1)\n";
    }
    saveLocation(value, STACK, "", loca);
  }
}

void Visit(const koopa_raw_get_elem_ptr_t& target, const koopa_raw_value_t& value) {
  reg_cnt = 0;
  if (target.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    cout << "\tla t0, " << lookup[target.src]->reg_name << "\n";
    cout << "\tli t1, " << target.index->kind.data.integer.value << "\n";
    cout << "\tli t2, 4"
         << "\n";
    cout << "\tmul t1, t1, t2"
         << "\n";
    cout << "\tadd t0, t0, t1"
         << "\n";

    int size = stack_cnt * 4;
    stack_cnt++;
    if (size < 2048 && size >= -2048) {
      cout << "\tsw t0, " << size << "(sp)\n";
    } else {
      cout << "\tli t1, " << size << "\n";
      cout << "\tadd t1, t1, sp\n";
      cout << "\tsw t0, 0(t1)\n";
    }
    saveLocation(value, STACK, "", size);
  } else {
    int size = lookup[target.src]->stack;
    if (size < 2048 && size >= -2048) {
      cout << "\taddi t0, sp, " << size << "\n";
    } else {
      cout << "\tli t1, " << size << "\n";
      cout << "\taddi t0, sp, t1\n";
    }
    cout << "\tli t1, " << target.index->kind.data.integer.value << "\n";
    cout << "\tli t2, 4"
         << "\n";
    cout << "\tmul t1, t1, t2"
         << "\n";
    cout << "\tadd t0, t0, t1"
         << "\n";
    int loca = stack_cnt * 4;
    stack_cnt++;
    if (loca < 2048 && loca >= -2048) {
      cout << "\tsw t0, " << loca << "(sp)\n";
    } else {
      cout << "\tli t1, " << loca << "\n";
      cout << "\tadd t1, t1, sp\n";
      cout << "\tsw t0, 0(t1)\n";
    }
    saveLocation(value, STACK, "", loca);
  }
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
           << ", " << lookup[binary.lhs]->reg_name << ", " << lookup[binary.rhs]->reg_name << "\n";
      cout << "\tsnez t0"
           << ", t0"
           << "\n";

      int loca = stack_cnt * 4;
      if (loca < 2048 && loca >= -2048) {
        cout << "\tsw t0, " << loca << "(sp)\n";
      } else {
        cout << "\tli t1, " << loca << "\n";
        cout << "\tadd t1, t1, sp\n";
        cout << "\tsw t0, 0(t1)\n";
      }

      saveLocation(value, STACK, "", loca);
      stack_cnt++;

      break;
    }

    /// Equal to.
    case 1: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\txor t0"
           << ", " << lookup[binary.lhs]->reg_name << ", " << lookup[binary.rhs]->reg_name << "\n";
      cout << "\tseqz t0"
           << ", t0"
           << "\n";

      int loca = stack_cnt * 4;
      if (loca < 2048 && loca >= -2048) {
        cout << "\tsw t0, " << loca << "(sp)\n";
      } else {
        cout << "\tli t1, " << loca << "\n";
        cout << "\tadd t1, t1, sp\n";
        cout << "\tsw t0, 0(t1)\n";
      }

      saveLocation(value, STACK, "", loca);
      stack_cnt++;

      break;
    }

    /// Greater than.
    case 2: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\tsgt t0"
           << ", " << lookup[binary.lhs]->reg_name << ", " << lookup[binary.rhs]->reg_name << "\n";

      int loca = stack_cnt * 4;
      if (loca < 2048 && loca >= -2048) {
        cout << "\tsw t0, " << loca << "(sp)\n";
      } else {
        cout << "\tli t1, " << loca << "\n";
        cout << "\tadd t1, t1, sp\n";
        cout << "\tsw t0, 0(t1)\n";
      }

      saveLocation(value, STACK, "", loca);
      stack_cnt++;

      break;
    }

    /// Less than.
    case 3: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\tslt t0"
           << ", " << lookup[binary.lhs]->reg_name << ", " << lookup[binary.rhs]->reg_name << "\n";

      int loca = stack_cnt * 4;
      if (loca < 2048 && loca >= -2048) {
        cout << "\tsw t0, " << loca << "(sp)\n";
      } else {
        cout << "\tli t1, " << loca << "\n";
        cout << "\tadd t1, t1, sp\n";
        cout << "\tsw t0, 0(t1)\n";
      }

      saveLocation(value, STACK, "", loca);
      stack_cnt++;

      break;
    }

    /// Greater than or equal to.
    case 4: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\tslt t0"
           << ", " << lookup[binary.lhs]->reg_name << ", " << lookup[binary.rhs]->reg_name << "\n";
      cout << "\tseqz t0"
           << ", t0"
           << "\n";

      int loca = stack_cnt * 4;
      if (loca < 2048 && loca >= -2048) {
        cout << "\tsw t0, " << loca << "(sp)\n";
      } else {
        cout << "\tli t1, " << loca << "\n";
        cout << "\tadd t1, t1, sp\n";
        cout << "\tsw t0, 0(t1)\n";
      }

      saveLocation(value, STACK, "", loca);
      stack_cnt++;

      break;
    }

    /// Less than or equal to.
    case 5: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\tsgt t0"
           << ", " << lookup[binary.lhs]->reg_name << ", " << lookup[binary.rhs]->reg_name << "\n";
      cout << "\tseqz t0, t0"
           << "\n";

      int loca = stack_cnt * 4;
      if (loca < 2048 && loca >= -2048) {
        cout << "\tsw t0, " << loca << "(sp)\n";
      } else {
        cout << "\tli t1, " << loca << "\n";
        cout << "\tadd t1, t1, sp\n";
        cout << "\tsw t0, 0(t1)\n";
      }

      saveLocation(value, STACK, "", loca);
      stack_cnt++;

      break;
    }

    /// Addition.
    case 6: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\tadd t0"
           << ", " << lookup[binary.lhs]->reg_name << ", " << lookup[binary.rhs]->reg_name << "\n";

      int loca = stack_cnt * 4;
      if (loca < 2048 && loca >= -2048) {
        cout << "\tsw t0, " << loca << "(sp)\n";
      } else {
        cout << "\tli t1, " << loca << "\n";
        cout << "\tadd t1, t1, sp\n";
        cout << "\tsw t0, 0(t1)\n";
      }

      saveLocation(value, STACK, "", loca);
      stack_cnt++;

      break;
    }

    /// Subtraction.
    case 7: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\tsub t0"
           << ", " << lookup[binary.lhs]->reg_name << ", " << lookup[binary.rhs]->reg_name << "\n";

      int loca = stack_cnt * 4;
      if (loca < 2048 && loca >= -2048) {
        cout << "\tsw t0, " << loca << "(sp)\n";
      } else {
        cout << "\tli t1, " << loca << "\n";
        cout << "\tadd t1, t1, sp\n";
        cout << "\tsw t0, 0(t1)\n";
      }

      saveLocation(value, STACK, "", loca);
      stack_cnt++;

      break;
    }

    /// Multiplication.
    case 8: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\tmul t0"
           << ", " << lookup[binary.lhs]->reg_name << ", " << lookup[binary.rhs]->reg_name << "\n";

      int loca = stack_cnt * 4;
      if (loca < 2048 && loca >= -2048) {
        cout << "\tsw t0, " << loca << "(sp)\n";
      } else {
        cout << "\tli t1, " << loca << "\n";
        cout << "\tadd t1, t1, sp\n";
        cout << "\tsw t0, 0(t1)\n";
      }

      saveLocation(value, STACK, "", loca);
      stack_cnt++;

      break;
    }

    /// Division.
    case 9: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\tdiv t0"
           << ", " << lookup[binary.lhs]->reg_name << ", " << lookup[binary.rhs]->reg_name << "\n";

      int loca = stack_cnt * 4;
      if (loca < 2048 && loca >= -2048) {
        cout << "\tsw t0, " << loca << "(sp)\n";
      } else {
        cout << "\tli t1, " << loca << "\n";
        cout << "\tadd t1, t1, sp\n";
        cout << "\tsw t0, 0(t1)\n";
      }

      saveLocation(value, STACK, "", loca);
      stack_cnt++;

      break;
    }

    /// Modulo.
    case 10: {
      reg_cnt = 0;

      Search(binary.lhs);
      Search(binary.rhs);

      cout << "\trem t0"
           << ", " << lookup[binary.lhs]->reg_name << ", " << lookup[binary.rhs]->reg_name << "\n";

      int loca = stack_cnt * 4;
      if (loca < 2048 && loca >= -2048) {
        cout << "\tsw t0, " << loca << "(sp)\n";
      } else {
        cout << "\tli t1, " << loca << "\n";
        cout << "\tadd t1, t1, sp\n";
        cout << "\tsw t0, 0(t1)\n";
      }

      saveLocation(value, STACK, "", loca);
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

        saveLocation(binary.lhs, REGISTER, "t" + to_string(reg_cnt), 0);
        reg_cnt++;

        cout << "\tandi t0"
             << ", " << lookup[binary.lhs]->reg_name << ", ";
        Visit(binary.lhs->kind.data.integer);
        cout << "\n";
      } else if (int_l && !int_r) {
        cout << "\tandi t0"
             << ", " << lookup[binary.rhs]->reg_name << ", ";
        Visit(binary.lhs->kind.data.integer);
        cout << "\n";
      } else if (!int_l && int_r) {
        cout << "\tandi t0"
             << ", " << lookup[binary.lhs]->reg_name << ", ";
        Visit(binary.rhs->kind.data.integer);
        cout << "\n";
      } else {
        cout << "\tand t0"
             << ", " << lookup[binary.lhs]->reg_name << ", " << lookup[binary.rhs]->reg_name << "\n";
      }

      int loca = stack_cnt * 4;
      if (loca < 2048 && loca >= -2048) {
        cout << "\tsw t0, " << loca << "(sp)\n";
      } else {
        cout << "\tli t1, " << loca << "\n";
        cout << "\tadd t1, t1, sp\n";
        cout << "\tsw t0, 0(t1)\n";
      }

      saveLocation(value, STACK, "", loca);
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

        saveLocation(binary.lhs, REGISTER, "t" + to_string(reg_cnt), 0);
        reg_cnt++;

        cout << "\tori t0"
             << ", " << lookup[binary.lhs]->reg_name << ", ";
        Visit(binary.lhs->kind.data.integer);
        cout << "\n";
      } else if (int_l && !int_r) {
        cout << "\tori t0"
             << ", " << lookup[binary.rhs]->reg_name << ", ";
        Visit(binary.lhs->kind.data.integer);
        cout << "\n";
      } else if (!int_l && int_r) {
        cout << "\tori t0"
             << ", " << lookup[binary.lhs]->reg_name << ", ";
        Visit(binary.rhs->kind.data.integer);
        cout << "\n";
      } else {
        cout << "\tor t0"
             << ", " << lookup[binary.lhs]->reg_name << ", " << lookup[binary.rhs]->reg_name << "\n";
      }

      int loca = stack_cnt * 4;
      if (loca < 2048 && loca >= -2048) {
        cout << "\tsw t0, " << loca << "(sp)\n";
      } else {
        cout << "\tli t1, " << loca << "\n";
        cout << "\tadd t1, t1, sp\n";
        cout << "\tsw t0, 0(t1)\n";
      }

      saveLocation(value, STACK, "", loca);
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

  int loca = lookup[value]->stack;
  if (loca < 2048 && loca >= -2048) {
    cout << "\tlw t" << reg_cnt << ", " << loca << "(sp)\n";
  } else {
    cout << "\tli t" << reg_cnt << ", " << loca << "\n";
    cout << "\tadd t" << reg_cnt << ", t" << reg_cnt << ", sp\n";
    reg_cnt++;
    cout << "\tlw t" << reg_cnt << ", 0(t" << reg_cnt - 1 << ")\n";
  }

  saveLocation(value, REGISTER, "t" + to_string(reg_cnt), 0);
  reg_cnt++;
  return false;
}

void Visit(const koopa_raw_branch_t& branch) {
  reg_cnt = 0;
  Search(branch.cond);
  cout << "\tbnez " << lookup[branch.cond]->reg_name << ", " << branch.true_bb->name + 1 << "\n";
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
      int loca = lookup[reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i])]->stack;
      if (loca < 2048 && loca >= -2048) {
        cout << "\tlw t0, " << loca << "(sp)\n";
      } else {
        cout << "\tli t1, " << loca << "\n";
        cout << "\tadd t1, t1, sp\n";
        cout << "\tlw t0, 0(t1)\n";
      }
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
      cout << "\tlw t0"
           << ", " << lookup[reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i])]->stack << "(sp)\n";
      cout << "\tsw t0"
           << ", " << (i - 8) * 4 << "(sp)\n";
    }
  }

  cout << "\tcall " << call.callee->name + 1 << "\n";

  // 根据ir是否保存返回值决定是否保存a0
  if (value->ty->tag != KOOPA_RTT_UNIT) {
    int loca = stack_cnt * 4;
    if (loca < 2048 && loca >= -2048) {
      cout << "\tsw a0, " << loca << "(sp)\n";
    } else {
      cout << "\tli t0, " << loca << "\n";
      cout << "\tadd t0, t0, sp\n";
      cout << "\tsw a0, 0(t0)\n";
    }
    saveLocation(value, STACK, "", loca);
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
      int loca = lookup[ret.value]->stack;
      if (loca < 2048 && loca >= -2048) {
        cout << "\tlw a0, " << loca << "(sp)\n";
      } else {
        cout << "\tli t0, " << loca << "\n";
        cout << "\tadd t0, t0, sp\n";
        cout << "\tlw a0, 0(t0)\n";
      }
    } else {
      cout << "\tERROR: Undefined Tag: " << ret.value->kind.tag << "\n";
    }
  }

  // if (has_call)
  //   cout << "\tlw ra, " << stack_space - 4 << "(sp)\n";

  if (has_call) {
    if (stack_space < 2052 && stack_space >= -2044) {
      cout << "\tlw ra, " << stack_space - 4 << "(sp)\n";
    } else {
      cout << "\tli t0, " << stack_space - 4 << "\n";
      cout << "\tadd t0, t0, sp\n";
      cout << "\tlw ra, 0(t0)\n";
    }
  }

  if (stack_space != 0) {
    if (stack_space < 2048 && stack_space >= -2048) {
      cout << "\taddi sp, sp, " << stack_space << "\n";
    } else {
      cout << "\tli t0, " << stack_space << "\n";
      cout << "\tadd sp, sp, t0"
           << "\n";
    }
  }
  cout << "\tret\n\n";
}