#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "observe_pipeline_ast.h"


SExpression *observePipelineAST_createStage(const char *stage_name, SExpression *function) {
    // printf("[observe][ast] createStage (%s, func_ptr=%p)\n", stage_name, (void*)function);

    SExpression *s = (SExpression *)malloc(sizeof(SExpression));
    s->type = AST_NODE_STAGE;
    s->stage.function = &function->function;
    s->stage.name = strdup(stage_name);

    // printf("\t=> %p\n", (void*)s);
    return s;
}

SExpression *observePipelineAST_addStage(SExpression *a, SExpression *b) {
    // printf("[observe][ast] addStage (%p, %p)\n", (void*)a, (void*)b);
    assert(a != NULL);
    assert(b != NULL);
    // printf("\t(t1=%d, t2=%d)\n", a->type, b->type);


    if (a->type == AST_NODE_STAGE_LIST && b->type == AST_NODE_STAGE_LIST) {
        assert(0);
        // TODO: Implement merging the stage list... 
        //          but I think the grammar will never call the addStage with two stage_list items.
    }
    SExpression *stage_list;
    if (a->type == AST_NODE_STAGE && b->type == AST_NODE_STAGE) {
        stage_list = (SExpression *)malloc(sizeof(SExpression));
        stage_list->type = AST_NODE_STAGE_LIST;
        stage_list->stage_list.len = 2;
        stage_list->stage_list.array = (Stage**)malloc(sizeof(Stage*)*2);
        stage_list->stage_list.array[0] = &a->stage;
        stage_list->stage_list.array[1] = &b->stage;
        // printf("\t=> %p\n", (void*)stage_list);
        return stage_list;
    }

    SExpression *stage = NULL;
    if (a->type == AST_NODE_STAGE_LIST && b->type == AST_NODE_STAGE) {
        stage_list = a;
        stage = b;
    }
    if (a->type == AST_NODE_STAGE && b->type == AST_NODE_STAGE_LIST) {
        stage_list = b;
        stage = a;
    }

    size_t new_length = stage_list->stage_list.len + 1;
    stage_list->stage_list.array = (Stage**)realloc(stage_list->stage_list.array, sizeof(Stage*)*new_length);
    stage_list->stage_list.array[new_length-1] = &stage->stage;
    stage_list->stage_list.len = new_length;

    return stage_list;
}

SExpression *observePipelineAST_createStageFunction(const char *function_name, SExpression *args_list) {
    assert(args_list != NULL);
    assert(args_list->type == AST_NODE_ARGUMENT_LIST);
    // printf("[observe][ast] createStageFunction (%s, %p)\n", function_name, (void *)args_list);

    SExpression *expr = (SExpression *)malloc(sizeof(SExpression));
    expr->type = AST_NODE_FUNCTION;
    expr->function.name = strdup(function_name);
    expr->function.args_list = &args_list->args_list;

    return expr;
}

SExpression *observePipelineAST_createStageFunctionNoArguments(const char *function_name) {
    // printf("[observe][ast] createStageFunctionNoArguments (%s)\n", function_name);

    SExpression *expr = (SExpression *)malloc(sizeof(SExpression));
    expr->type = AST_NODE_FUNCTION;
    expr->function.name = strdup(function_name);
    expr->function.args_list = NULL;

    return expr;
}

SExpression *observePipelineAST_createStageFunctionArgsListNew(SExpression *arg) {
    // printf("[observe][ast] createStageFunctionArgsListNew (%p)\n", (void*)arg);

    SExpression *expr = (SExpression *)malloc(sizeof(SExpression));
    expr->type = AST_NODE_ARGUMENT_LIST;
    expr->args_list.args_array = (Arg*)malloc(sizeof(Arg)*1);
    expr->args_list.args_array[0] = arg->arg;
    expr->args_list.args_array_len = 1;

    return expr;
}

SExpression *observePipelineAST_createStageFunctionArgsListAppend(SExpression *args_list, SExpression *arg) {
    // printf("[observe][ast] createStageFunctionArgsListAppend (args_list=%p, arg=%p)\n", (void*)args_list, (void*)arg);

    size_t new_length = args_list->args_list.args_array_len + 1;
    args_list->args_list.args_array = realloc(args_list->args_list.args_array, sizeof(Arg)*new_length);
    args_list->args_list.args_array[new_length-1] = arg->arg;
    args_list->args_list.args_array_len = new_length;

    return args_list;
}

SExpression *observePipelineAST_createStageFunctionArg(const char *arg_name, const char *arg_value) {
    // printf("[observe][ast] createStageFunctionArg (arg_name=%s, arg_value=%s)\n", arg_name, arg_value);

    SExpression *expr = (SExpression *)malloc(sizeof(SExpression));
    expr->type = AST_NODE_ARGUMENT;
    expr->arg.name = strdup(arg_name);
    expr->arg.value = strdup(arg_value);

    return expr;
}

void observePipelineAST_free(SExpression *node) {
    // printf("[observe][ast] freeAST (%p)\n", (void*)node);
    if (node == NULL) { return; }

    if (node->type == AST_NODE_STAGE_LIST) {
        if (node->stage_list.array != NULL) {
            free(node->stage_list.array);
        }
    }
    if (node->type == AST_NODE_STAGE) {
        free(node->stage.function);
        free(node->stage.name);
    }

    free(node);
}

void observePipelineAST_print(const SExpression *node) {
    assert(node != NULL);
    if (node->type == AST_NODE_STAGE_LIST) {
        printf("[ast] printAST Node(StageList) ptr=%p\n", (void*)node);
        for (size_t i = 0; i < node->stage_list.len; i++) {
            printf("\tname=%s\n", node->stage_list.array[i]->function->name);
        }
    }
    if (node->type == AST_NODE_STAGE) {
        printf("[ast] printAST Node(Stage) ptr=%p name=%s\n", (void*)node, node->stage.name);
    }
}
