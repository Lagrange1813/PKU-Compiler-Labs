[北大编译实践在线文档](https://pku-minic.github.io/online-doc/#/)

### Example

C

```c
int fib(int n) {
  if (n <= 2) {
    return 1;
  } else {
    return fib(n - 1) + fib(n - 2);
  }
}

int main() {
  int input = getint();
  putint(fib(input));
  putch(10);
  return 0;
}
```

Koopa IR 生成结果

```ir
decl @getint(): i32
decl @getch(): i32
decl @getarray(*i32): i32
decl @putint(i32)
decl @putch(i32)
decl @putarray(i32, *i32)
decl @starttime()
decl @stoptime()

fun @fib(@n: i32): i32 {
%entry:
	@n_1 = alloc i32
	store @n, @n_1
	%0 = load @n_1
	%1 = le %0, 2
	br %1, %then, %else
%then:
	ret 1
%else:
	%2 = load @n_1
	%3 = sub %2, 1
	%4 = call @fib(%3)
	%5 = load @n_1
	%6 = sub %5, 2
	%7 = call @fib(%6)
	%8 = add %4, %7
	ret %8
}

fun @main(): i32 {
%entry:
	%0 = call @getint()
	@input_6 = alloc i32
	store %0, @input_6
	%1 = load @input_6
	%2 = call @fib(%1)
	call @putint(%2)
	call @putch(10)
	ret 0
}
```

RISC-V 汇编生成结果

```assembly
	.text
	.globl fib
fib:
	addi sp, sp, -44
	sw ra, 40(sp)
	sw a0, 0(sp)
	lw t0, 0(sp)
	sw t0, 4(sp)
	lw t0, 4(sp)
	li t1, 2
	sgt t0, t0, t1
	seqz t0, t0
	sw t0, 8(sp)
	lw t0, 8(sp)
	bnez t0, then
	j else
then:
	li a0, 1
	lw ra, 40(sp)
	addi sp, sp, 44
	ret
else:
	lw t0, 0(sp)
	sw t0, 12(sp)
	lw t0, 12(sp)
	li t1, 1
	sub t0, t0, t1
	sw t0, 16(sp)
	lw t0, 16(sp)
	mv a0, t0
	call fib
	sw a0, 20(sp)
	lw t0, 0(sp)
	sw t0, 24(sp)
	lw t0, 24(sp)
	li t1, 2
	sub t0, t0, t1
	sw t0, 28(sp)
	lw t0, 28(sp)
	mv a0, t0
	call fib
	sw a0, 32(sp)
	lw t0, 20(sp)
	lw t1, 32(sp)
	add t0, t0, t1
	sw t0, 36(sp)
	lw a0, 36(sp)
	lw ra, 40(sp)
	addi sp, sp, 44
	ret

	.text
	.globl main
main:
	addi sp, sp, -20
	sw ra, 16(sp)
	call getint
	sw a0, 0(sp)
	lw t0, 0(sp)
	sw t0, 4(sp)
	lw t0, 4(sp)
	sw t0, 8(sp)
	lw t0, 8(sp)
	mv a0, t0
	call fib
	sw a0, 12(sp)
	lw t0, 12(sp)
	mv a0, t0
	call putint
	li a0, 10
	call putch
	li a0, 0
	lw ra, 16(sp)
	addi sp, sp, 20
	ret
```

AST 树

```
CompUnitAST {
  CompUnitSubWithFuncAST {
    CompUnitSubWithFuncAST {
      FuncDefAST {
        FuncTypeAST{ int } Ident{ fib } FuncFParamsAST {
          FuncFParamAST {
            BTypeAST{ int } Ident {
              n
            }
          }
        }
        BlockAST {
          BlockItemWithStmtAST {
            StmtWithIfAST {
              ExpAST {
                LOrExpAST {
                  LAndExpAST {
                    EqExpAST {
                      RelExpWithOpAST {
                        RelExpAST {
                          AddExpAST {
                            MulExpAST {
                              UnaryExpAST {
                                PrimaryExpWithLValAST {
                                  LValAST {
                                    n
                                  }
                                }
                              }
                            }
                          }
                        }
                        RelExpOpAST{ <= } AddExpAST {
                          MulExpAST {
                            UnaryExpAST {
                              PrimaryExpWithNumAST {
                                2
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
              StmtWithBlockAST {
                BlockAST {
                  BlockItemWithStmtAST {
                    StmtWithReturnAST {
                      ExpAST {
                        LOrExpAST {
                          LAndExpAST {
                            EqExpAST {
                              RelExpAST {
                                AddExpAST {
                                  MulExpAST {
                                    UnaryExpAST {
                                      PrimaryExpWithNumAST {
                                        1
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
              StmtWithBlockAST {
                BlockAST {
                  BlockItemWithStmtAST {
                    StmtWithReturnAST {
                      ExpAST {
                        LOrExpAST {
                          LAndExpAST {
                            EqExpAST {
                              RelExpAST {
                                AddExpWithOpAST {
                                  AddExpAST {
                                    MulExpAST {
                                      UnaryExpWithFuncAST {
                                        Ident{ fib } FuncRParamsAST {
                                          ExpAST {
                                            LOrExpAST {
                                              LAndExpAST {
                                                EqExpAST {
                                                  RelExpAST {
                                                    AddExpWithOpAST {
                                                      AddExpAST {
                                                        MulExpAST {
                                                          UnaryExpAST {
                                                            PrimaryExpWithLValAST {
                                                              LValAST {
                                                                n
                                                              }
                                                            }
                                                          }
                                                        }
                                                      }
                                                      AddExpOpAST{ - } MulExpAST {
                                                        UnaryExpAST {
                                                          PrimaryExpWithNumAST {
                                                            1
                                                          }
                                                        }
                                                      }
                                                    }
                                                  }
                                                }
                                              }
                                            }
                                          }
                                        }
                                      }
                                    }
                                  }
                                  AddExpOpAST{ + } MulExpAST {
                                    UnaryExpWithFuncAST {
                                      Ident{ fib } FuncRParamsAST {
                                        ExpAST {
                                          LOrExpAST {
                                            LAndExpAST {
                                              EqExpAST {
                                                RelExpAST {
                                                  AddExpWithOpAST {
                                                    AddExpAST {
                                                      MulExpAST {
                                                        UnaryExpAST {
                                                          PrimaryExpWithLValAST {
                                                            LValAST {
                                                              n
                                                            }
                                                          }
                                                        }
                                                      }
                                                    }
                                                    AddExpOpAST{ - } MulExpAST {
                                                      UnaryExpAST {
                                                        PrimaryExpWithNumAST {
                                                          2
                                                        }
                                                      }
                                                    }
                                                  }
                                                }
                                              }
                                            }
                                          }
                                        }
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    FuncDefAST {
      FuncTypeAST{ int } Ident{ main } BlockAST {
        BlockItemWithDeclAST {
          DeclWithVarAST {
            VarDeclAST {
              BTypeAST{ int } VarDefWithAssignAST {
                Ident{ input } InitValAST {
                  ExpAST {
                    LOrExpAST {
                      LAndExpAST {
                        EqExpAST {
                          RelExpAST {
                            AddExpAST {
                              MulExpAST {
                                UnaryExpWithFuncAST {
                                  Ident {
                                    getint
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
        BlockItemWithStmtAST {
          StmtWithExpAST {
            ExpAST {
              LOrExpAST {
                LAndExpAST {
                  EqExpAST {
                    RelExpAST {
                      AddExpAST {
                        MulExpAST {
                          UnaryExpWithFuncAST {
                            Ident{ putint } FuncRParamsAST {
                              ExpAST {
                                LOrExpAST {
                                  LAndExpAST {
                                    EqExpAST {
                                      RelExpAST {
                                        AddExpAST {
                                          MulExpAST {
                                            UnaryExpWithFuncAST {
                                              Ident{ fib } FuncRParamsAST {
                                                ExpAST {
                                                  LOrExpAST {
                                                    LAndExpAST {
                                                      EqExpAST {
                                                        RelExpAST {
                                                          AddExpAST {
                                                            MulExpAST {
                                                              UnaryExpAST {
                                                                PrimaryExpWithLValAST {
                                                                  LValAST {
                                                                    input
                                                                  }
                                                                }
                                                              }
                                                            }
                                                          }
                                                        }
                                                      }
                                                    }
                                                  }
                                                }
                                              }
                                            }
                                          }
                                        }
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
        BlockItemWithStmtAST {
          StmtWithExpAST {
            ExpAST {
              LOrExpAST {
                LAndExpAST {
                  EqExpAST {
                    RelExpAST {
                      AddExpAST {
                        MulExpAST {
                          UnaryExpWithFuncAST {
                            Ident{ putch } FuncRParamsAST {
                              ExpAST {
                                LOrExpAST {
                                  LAndExpAST {
                                    EqExpAST {
                                      RelExpAST {
                                        AddExpAST {
                                          MulExpAST {
                                            UnaryExpAST {
                                              PrimaryExpWithNumAST {
                                                10
                                              }
                                            }
                                          }
                                        }
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
        BlockItemWithStmtAST {
          StmtWithReturnAST {
            ExpAST {
              LOrExpAST {
                LAndExpAST {
                  EqExpAST {
                    RelExpAST {
                      AddExpAST {
                        MulExpAST {
                          UnaryExpAST {
                            PrimaryExpWithNumAST {
                              0
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
```