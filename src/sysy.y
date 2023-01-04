%code requires {
  #include <memory>
  #include <string>
  #include "ast.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <vector>

#include "ast.hpp"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  std::vector<std::unique_ptr<BaseAST>> *vec_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT VOID RETURN CONST IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> CompUnitSub
%type <ast_val> Decl ConstDecl ConstDef ConstInitVal   
%type <ast_val> VarDecl VarDef InitVal
%type <ast_val> FuncDef FuncFParams FuncFParam FuncRParams
%type <ast_val> Block BlockItem Stmt 
%type <ast_val> Exp LVal PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp ConstExp

%type <vec_val> BlockItemList ConstDefList VarDefList FuncFParamList ExpList ConstInitValList ArrayConstExpList InitValList ArrayExpList

%type <int_val> Number

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : CompUnitSub {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->sub = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;

CompUnitSub
  : FuncDef {
    auto ast = new CompUnitSubWithFuncAST();
    ast->func_def = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Decl {
    auto ast = new CompUnitSubWithDeclAST();
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | CompUnitSub FuncDef {
    auto ast = new CompUnitSubWithFuncAST();
    ast->compUnit = make_optional(unique_ptr<BaseAST>($1));
    ast->func_def = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | CompUnitSub Decl {
    auto ast = new CompUnitSubWithDeclAST();
    ast->compUnit = make_optional(unique_ptr<BaseAST>($1));
    ast->decl = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclWithConstAST();
    ast->constDecl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclWithVarAST();
    ast->varDecl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST INT ConstDef ConstDefList ';' {
    auto ast = new ConstDeclAST();
    string * type = new string("int");
    ast->bType = *unique_ptr<string>(type);
    ast->constDefList.push_back(unique_ptr<BaseAST>($3));
    vector<unique_ptr<BaseAST> > *vec = ($4);
    for (auto it = vec->begin(); it != vec->end(); it++)
      ast->constDefList.push_back(move(*it));
    $$ = ast;
  }
  ;

ConstDefList
  : {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | ConstDefList ',' ConstDef {
    vector<unique_ptr<BaseAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

ConstDef
  : IDENT ArrayConstExpList '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    vector<unique_ptr<BaseAST> > *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
      ast->arrayConstExpList.push_back(move(*it));
    ast->constInitVal = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

ArrayConstExpList
  : {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | ArrayConstExpList '[' ConstExp ']' {
    vector<unique_ptr<BaseAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->constExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new ConstInitValWithListAST();
    $$ = ast;
  }
  | '{' ConstInitVal ConstInitValList '}' {
    auto ast = new ConstInitValWithListAST();
    ast->constInitValList.push_back(unique_ptr<BaseAST>($2));
    vector<unique_ptr<BaseAST> > *vec = ($3);
    for (auto it = vec->begin(); it != vec->end(); it++)
      ast->constInitValList.push_back(move(*it));
    $$ = ast;
  }
  ;

ConstInitValList
  : {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | ConstInitValList ',' ConstInitVal {
    vector<unique_ptr<BaseAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

VarDecl
  : INT VarDef VarDefList ';' {
    auto ast = new VarDeclAST();
    string * type = new string("int");
    ast->bType = *unique_ptr<string>(type);
    ast->varDefList.push_back(unique_ptr<BaseAST>($2));
    vector<unique_ptr<BaseAST> > *vec = ($3);
    for (auto it = vec->begin(); it != vec->end(); it++)
      ast->varDefList.push_back(move(*it));
    $$ = ast;
  }
  ;

VarDefList
  : {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | VarDefList ',' VarDef {
    vector<unique_ptr<BaseAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

VarDef
  : IDENT ArrayConstExpList {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    vector<unique_ptr<BaseAST> > *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
      ast->arrayConstExpList.push_back(move(*it));
    $$ = ast;
  }
  | IDENT ArrayConstExpList '=' InitVal {
    auto ast = new VarDefWithAssignAST();
    ast->ident = *unique_ptr<string>($1);
    vector<unique_ptr<BaseAST> > *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
      ast->arrayConstExpList.push_back(move(*it));
    ast->initVal = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new InitValWithListAST();
    $$ = ast;
  }
  | '{' InitVal InitValList '}' {
    auto ast = new InitValWithListAST();
    ast->initValList.push_back(unique_ptr<BaseAST>($2));
    vector<unique_ptr<BaseAST> > *vec = ($3);
    for (auto it = vec->begin(); it != vec->end(); it++)
      ast->initValList.push_back(move(*it));
    $$ = ast;
  }
  ;

InitValList
  : {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | InitValList ',' InitVal {
    vector<unique_ptr<BaseAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
FuncDef
  : INT IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    string * type = new string("int");
    ast->funcType = *unique_ptr<string>(type);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | INT IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    string * type = new string("int");
    ast->funcType = *unique_ptr<string>(type);
    ast->ident = *unique_ptr<string>($2);
    ast->params = make_optional(unique_ptr<BaseAST>($4));
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  | VOID IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    string * type = new string("void");
    ast->funcType = *unique_ptr<string>(type);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | VOID IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    string * type = new string("void");
    ast->funcType = *unique_ptr<string>(type);
    ast->ident = *unique_ptr<string>($2);
    ast->params = make_optional(unique_ptr<BaseAST>($4));
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;

FuncFParams
  : FuncFParam FuncFParamList {
    auto ast = new FuncFParamsAST();
    ast->paramList.push_back(unique_ptr<BaseAST>($1));
    vector<unique_ptr<BaseAST>> *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
      ast->paramList.push_back(move(*it));
    $$ = ast;
  }
  ;

FuncFParamList
  : {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | FuncFParamList ',' FuncFParam {
    vector<unique_ptr<BaseAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

FuncFParam
  : INT IDENT {
    auto ast = new FuncFParamAST();
    string * type = new string("int");
    ast->bType = *unique_ptr<string>(type);
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  | INT IDENT '[' ']' ArrayConstExpList {
    auto ast = new FuncFParamAST();
    string * type = new string("int");
    ast->bType = *unique_ptr<string>(type);
    ast->ident = *unique_ptr<string>($2);
    ast->isArray = true;
    vector<unique_ptr<BaseAST> > *vec = ($5);
    for (auto it = vec->begin(); it != vec->end(); it++)
      ast->arrayConstExpList.push_back(move(*it));
    $$ = ast;
  }
  ;

Block
  : '{' BlockItemList '}' {
    auto ast = new BlockAST();
    vector<unique_ptr<BaseAST> > *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
      ast->blockItemList.push_back(move(*it));
    $$ = ast;
  }
  ;

BlockItemList
  : {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | BlockItemList BlockItem {
    vector<unique_ptr<BaseAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }

BlockItem
  : Decl {
    auto ast = new BlockItemWithDeclAST();
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Stmt {  
    auto ast = new BlockItemWithStmtAST();
    ast->stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }

Stmt
  : LVal '=' Exp ';' {
    auto ast = new StmtWithAssignAST();
    ast->lVal = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtWithExpAST();
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtWithExpAST();
    ast->exp = make_optional(unique_ptr<BaseAST>($1)); 
    $$ = ast;
  }
  | Block {
    auto ast = new StmtWithBlockAST();
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | IF '(' Exp ')' Stmt {
    auto ast = new StmtWithIfAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->if_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' Stmt ELSE Stmt {
    auto ast = new StmtWithIfAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->if_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new StmtWithWhileAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtWithBreakAST();
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtWithContinueAST();
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtWithReturnAST();
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new StmtWithReturnAST();
    ast->exp = make_optional(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lOrExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT ArrayExpList {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    vector<unique_ptr<BaseAST> > *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
      ast->arrayExpList.push_back(move(*it));
    $$ = ast;
  }
  ;

ArrayExpList
  : {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | ArrayExpList '[' Exp ']' {
    vector<unique_ptr<BaseAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpWithBrAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | LVal { 
    auto ast = new PrimaryExpWithLValAST();
    ast->lVal = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpWithNumAST();
    ast->number = $1;
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->primaryExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | IDENT '(' ')'  {
    auto ast = new UnaryExpWithFuncAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')'  {
    auto ast = new UnaryExpWithFuncAST();
    ast->ident = *unique_ptr<string>($1);
    ast->params = make_optional(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  | '!' UnaryExp {
    auto ast = new UnaryExpWithOpAST();
    ast->unaryOp = '!';
    ast->unaryExp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | '+' UnaryExp {
    auto ast = new UnaryExpWithOpAST();
    ast->unaryOp = '+';
    ast->unaryExp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | '-' UnaryExp {
    auto ast = new UnaryExpWithOpAST();
    ast->unaryOp = '-';
    ast->unaryExp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

FuncRParams
  : Exp ExpList {
    auto ast = new FuncRParamsAST();
    ast->paramList.push_back(unique_ptr<BaseAST>($1));
    vector<unique_ptr<BaseAST>> *vec = ($2);
    for (auto it = vec->begin(); it != vec->end(); it++)
      ast->paramList.push_back(move(*it));
    $$ = ast;
  }
  ;

ExpList
  : {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | ExpList ',' Exp {
    vector<unique_ptr<BaseAST>> *vec = ($1);
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->unaryExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp '*' UnaryExp {
    auto ast = new MulExpWithOpAST();
    ast->mulExp = unique_ptr<BaseAST>($1);
    ast->mulOp = '*';
    ast->unaryExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | MulExp '/' UnaryExp {
    auto ast = new MulExpWithOpAST();
    ast->mulExp = unique_ptr<BaseAST>($1);
    ast->mulOp = '/';
    ast->unaryExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | MulExp '%' UnaryExp {
    auto ast = new MulExpWithOpAST();
    ast->mulExp = unique_ptr<BaseAST>($1);
    ast->mulOp = '%';
    ast->unaryExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->mulExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp '+' MulExp {
    auto ast = new AddExpWithOpAST();
    ast->addExp = unique_ptr<BaseAST>($1);
    ast->addOp = '+';
    ast->mulExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | AddExp '-' MulExp {
    auto ast = new AddExpWithOpAST();
    ast->addExp = unique_ptr<BaseAST>($1);
    ast->addOp = '-';
    ast->mulExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->addExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp '<' AddExp {
    auto ast = new RelExpWithOpAST();
    ast->relExp = unique_ptr<BaseAST>($1);
    string * op = new string("<");
    ast->relOp = *unique_ptr<string>(op);
    ast->addExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp '>' AddExp {
    auto ast = new RelExpWithOpAST();
    ast->relExp = unique_ptr<BaseAST>($1);
    string * op = new string(">");
    ast->relOp = *unique_ptr<string>(op);
    ast->addExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp '<' '=' AddExp {
    auto ast = new RelExpWithOpAST();
    ast->relExp = unique_ptr<BaseAST>($1);
    string * op = new string("<=");
    ast->relOp = *unique_ptr<string>(op);
    ast->addExp = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  | RelExp '>' '=' AddExp {
    auto ast = new RelExpWithOpAST();
    ast->relExp = unique_ptr<BaseAST>($1);
    string * op = new string(">=");
    ast->relOp = *unique_ptr<string>(op);
    ast->addExp = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->relExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp '=' '=' RelExp {
    auto ast = new EqExpWithOpAST();
    ast->eqExp = unique_ptr<BaseAST>($1);
    string * op = new string("==");
    ast->eqOp = *unique_ptr<string>(op);
    ast->relExp = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  | EqExp '!' '=' RelExp {
    auto ast = new EqExpWithOpAST();
    ast->eqExp = unique_ptr<BaseAST>($1);
    string * op = new string("!=");
    ast->eqOp = *unique_ptr<string>(op);
    ast->relExp = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->eqExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp '&' '&' EqExp {
    auto ast = new LAndExpWithOpAST();
    ast->lAndExp = unique_ptr<BaseAST>($1);
    string * op = new string("&&");
    ast->lAndOp = *unique_ptr<string>(op);
    ast->eqExp = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->lAndExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp '|' '|' LAndExp {
    auto ast = new LOrExpWithOpAST();
    ast->lOrExp = unique_ptr<BaseAST>($1);
    string * op = new string("||");
    ast->lOrOp = *unique_ptr<string>(op);
    ast->lAndExp = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数

// 打印错误信息
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  extern int yylineno;
  extern char *yytext;

  int len = strlen(yytext);
  int i;
  char buf[512] = {0};

  for (i=0;i<len;++i){
    sprintf(buf,"%s%d ",buf,yytext[i]);
  }

  fprintf(stderr, "ERROR: %s at symbol '%s' on line %d\n", s, buf, yylineno);
}