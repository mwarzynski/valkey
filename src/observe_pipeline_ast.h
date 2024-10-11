#ifndef OBSERVE_PIPELINE_AST_H
#define OBSERVE_PIPELINE_AST_H

#include <stdlib.h>

typedef enum tagNodeType {
    AST_NODE_STAGE_LIST,
    AST_NODE_STAGE,
    AST_NODE_FUNCTION,
    AST_NODE_ARGUMENT_LIST,
    AST_NODE_ARGUMENT,
} NodeType;

typedef struct tagArg {
    char *name;
    char *value;
} Arg;

typedef struct tagArgsList {
    Arg *args_array;
    size_t args_array_len;
} ArgsList;

typedef struct tagFunction {
    char *name;
    ArgsList *args_list;
} Function;

typedef struct tagStage {
    char *name;
    Function *function;
} Stage;

typedef struct tagStageList {
    Stage **array;
    size_t len;
} StageList;

typedef struct SExpression
{
    NodeType type;
    union {
        StageList stage_list;
        Stage stage;
        Function function;
        ArgsList args_list;
        Arg arg;
    };
} SExpression;

SExpression *observePipelineAST_createStage(const char *stage_name, SExpression *function);
SExpression *observePipelineAST_addStage(SExpression *stages, SExpression *stage);
SExpression *observePipelineAST_createStageFunction(const char *function_name, SExpression *args_list);
SExpression *observePipelineAST_createStageFunctionNoArguments(const char *function_name);
SExpression *observePipelineAST_createStageFunctionArgsListNew(SExpression *arg);
SExpression *observePipelineAST_createStageFunctionArgsListAppend(SExpression *args_list, SExpression *arg);
SExpression *observePipelineAST_createStageFunctionArg(const char *arg_name, const char *arg_value);

void observePipelineAST_free(SExpression *node);
void observePipelineAST_print(const SExpression *node);

#endif
