%{

#include "observe_pipeline_ast.h"
#include "observe_pipeline_grammar.h"

#include <stdio.h>

%}

%option outfile="observe_pipeline_lexer.c" header-file="observe_pipeline_lexer.h"
%option warn nodefault

%option reentrant noyywrap never-interactive nounistd
%option bison-bridge

%%

[ \r\n\t]*   { continue; }

"|"          { return TOKEN_PIPE; }
"("          { return TOKEN_LPAREN; }
")"          { return TOKEN_RPAREN; }
","          { return TOKEN_COMMA; }
"="          { return TOKEN_EQUALS; }

[a-zA-Z0-9_]* {
                        yylval->identifier = strdup(yytext); 
                        return TOKEN_IDENTIFIER; 
                      }

.	printf("bad input character '%s' at line %d\n", yytext, yylineno);

%%

int yyerror(SExpression **expression, yyscan_t scanner, const char *msg) {
    // discard the unused warnings...
    scanner = scanner;
    expression = expression;

    fprintf(stderr, "Error: %s\n", msg);
    return 0;
}

