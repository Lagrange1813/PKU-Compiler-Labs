#include "ast.hpp"

bool filter(int input) {
  if (input == 0 || input == 1) {
    return true;
  }

  str += "  %";
  str += std::to_string(cnt);
  str += " = ";
  str += "eq";
  str += " ";
  str += std::to_string(input);
  str += ", 0\n";
  cnt++;

  return false;
}