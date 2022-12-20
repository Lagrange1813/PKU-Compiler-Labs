#pragma once

#include <cassert>
#include <cstring>

#include "koopa.h"

void Visit(const koopa_raw_program_t& program);
void Visit(const koopa_raw_slice_t& slice);
void Visit(const koopa_raw_function_t& func);
void Visit(const koopa_raw_basic_block_t& bb);
void Visit(const koopa_raw_value_t& value);

void Visit(const koopa_raw_load_t& load, const koopa_raw_value_t& value);
void Visit(const koopa_raw_store_t& store);

void Visit(const koopa_raw_integer_t& integer);
void Visit(const koopa_raw_binary_t& binary, const koopa_raw_value_t& value);

void Visit(const koopa_raw_branch_t& branch);
void Visit(const koopa_raw_jump_t& jump);
void Visit(const koopa_raw_call_t& call);
void Visit(const koopa_raw_return_t& ret);