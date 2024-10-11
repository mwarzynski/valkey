%{

#include "observe_pipeline_ast.h"
#include "observe_pipeline_grammar.h"
#include "observe_pipeline_lexer.h"

int yyerror(SExpression **expression, yyscan_t scanner, const char *msg);

%}

%code requires {
  typedef void* yyscan_t;
}


%output  "observe_pipeline_grammar.c"
%defines "observe_pipeline_grammar.h"

%define api.pure
%lex-param   { yyscan_t scanner }
%parse-param { SExpression **expression }
%parse-param { yyscan_t scanner }

%union {
    SExpression *expression;
    char *identifier;
}

%token TOKEN_PIPE                    "|"
%token TOKEN_LPAREN                  "("
%token TOKEN_RPAREN                  ")"
%token TOKEN_COMMA                   ","
%token TOKEN_EQUALS                  "="
%token <identifier> TOKEN_IDENTIFIER "identifier"

%type <expression> input expr expr_list stage function args arg

%%

input
    : expr_list { *expression = $1; }
    ;

expr_list
    : expr                      { $$ = $1; }
    | expr_list TOKEN_PIPE expr { $$ = observePipelineAST_addStage($1, $3); }
    ;

expr
    : stage { $$ = $1; }
    ;

stage
    : TOKEN_IDENTIFIER TOKEN_LPAREN function TOKEN_RPAREN
        { $$ = observePipelineAST_createStage($1, $3); }
    ;

function
    : TOKEN_IDENTIFIER TOKEN_LPAREN args TOKEN_RPAREN
        { $$ = observePipelineAST_createStageFunction($1, $3); }
    | TOKEN_IDENTIFIER TOKEN_LPAREN TOKEN_RPAREN
        { $$ = observePipelineAST_createStageFunctionNoArguments($1); }
    ;

args
    : arg
        { $$ = observePipelineAST_createStageFunctionArgsListNew($1); }
    | args TOKEN_COMMA arg
        { $$ = observePipelineAST_createStageFunctionArgsListAppend($1, $3); }
    ;

arg
    : TOKEN_IDENTIFIER TOKEN_EQUALS TOKEN_IDENTIFIER
        { $$ = observePipelineAST_createStageFunctionArg($1, $3); }
    ;

%%
