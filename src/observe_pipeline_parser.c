#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "observe_pipeline_parser.h"
#include "observe_pipeline_ast.h"
#include "observe_pipeline_grammar.h"
#include "observe_pipeline_lexer.h"

ObservePipelineStageType observeParseStagePipelineType(const char *stage_type_raw) {
    if (strcmp(stage_type_raw, OBSERVE_PIPELINE_STAGE_FILTER_STR_REPR) == 0) {
        return OBSERVE_PIPELINE_STAGE_FILTER;
    }
    if (strcmp(stage_type_raw, OBSERVE_PIPELINE_STAGE_REDUCE_STR_REPR) == 0) {
        return OBSERVE_PIPELINE_STAGE_REDUCE;
    }
    if (strcmp(stage_type_raw, OBSERVE_PIPELINE_STAGE_SAMPLE_STR_REPR) == 0) {
        return OBSERVE_PIPELINE_STAGE_SAMPLE;
    }
    if (strcmp(stage_type_raw, OBSERVE_PIPELINE_STAGE_MAP_STR_REPR) == 0) {
        return OBSERVE_PIPELINE_STAGE_MAP;
    }
    if (strcmp(stage_type_raw, OBSERVE_PIPELINE_STAGE_WINDOW_STR_REPR) == 0) {
        return OBSERVE_PIPELINE_STAGE_WINDOW;
    }
    if (strcmp(stage_type_raw, OBSERVE_PIPELINE_STAGE_REDUCE_STR_REPR) == 0) {
        return OBSERVE_PIPELINE_STAGE_REDUCE;
    }
    if (strcmp(stage_type_raw, OBSERVE_PIPELINE_STAGE_OUTPUT_STR_REPR) == 0) {
        return OBSERVE_PIPELINE_STAGE_OUTPUT;
    }
    return OBSERVE_PIPELINE_STAGE_UNKNOWN;
}

const char* observeStagePipelineTypeToStr(ObservePipelineStageType stage_type) {
    if (stage_type == OBSERVE_PIPELINE_STAGE_FILTER) {
        return OBSERVE_PIPELINE_STAGE_FILTER_STR_REPR;
    }
    if (stage_type == OBSERVE_PIPELINE_STAGE_PARTITION) {
        return OBSERVE_PIPELINE_STAGE_PARTITION_STR_REPR;
    }
    if (stage_type == OBSERVE_PIPELINE_STAGE_SAMPLE) {
        return OBSERVE_PIPELINE_STAGE_SAMPLE_STR_REPR;
    }
    if (stage_type == OBSERVE_PIPELINE_STAGE_MAP) {
        return OBSERVE_PIPELINE_STAGE_MAP_STR_REPR;
    }
    if (stage_type == OBSERVE_PIPELINE_STAGE_WINDOW) {
        return OBSERVE_PIPELINE_STAGE_WINDOW_STR_REPR;
    }
    if (stage_type == OBSERVE_PIPELINE_STAGE_REDUCE) {
        return OBSERVE_PIPELINE_STAGE_REDUCE_STR_REPR;
    }
    if (stage_type == OBSERVE_PIPELINE_STAGE_OUTPUT) {
        return OBSERVE_PIPELINE_STAGE_OUTPUT_STR_REPR;
    }
    return "unknown";
}

ObservePipelineConfiguration *observeParsePipelineConfiguration(const char *str) {
    SExpression *ast;
    yyscan_t scanner;
    YY_BUFFER_STATE state;

    if (yylex_init(&scanner)) {
        return NULL;
    }

    state = yy_scan_string(str, scanner);

    if (yyparse(&ast, scanner)) {
        return NULL;
    }

    yy_delete_buffer(state, scanner);
    yylex_destroy(scanner);

    // === Copy the data to our final output data structure ===

    assert(ast->type == AST_NODE_STAGE_LIST);

    ObservePipelineConfiguration *configuration = (ObservePipelineConfiguration*)malloc(sizeof(ObservePipelineConfiguration));
    configuration->len = 0;
    configuration->array = NULL;
    if (ast->stage_list.len == 0) {
        return configuration;
    }

    configuration->len = ast->stage_list.len;
    configuration->array = (ObservePipelineStage*)malloc(sizeof(ObservePipelineStage) * configuration->len);
    for (size_t i = 0; i < configuration->len; i++) {
        configuration->array[i].stage_type = observeParseStagePipelineType(ast->stage_list.array[i]->name);
        Function *function = ast->stage_list.array[i]->function;
        configuration->array[i].function_name = strdup(function->name);

        if (function->args_list != NULL && function->args_list->args_array_len > 0) {
            size_t args_len = function->args_list->args_array_len;
            configuration->array[i].arguments_len = args_len;
            configuration->array[i].arguments = (ObservePipelineArgument*)malloc(sizeof(ObservePipelineArgument)*args_len);
            for (size_t j = 0; j < args_len; j++) {
                configuration->array[i].arguments[j].name = strdup(function->args_list->args_array[j].name);
                configuration->array[i].arguments[j].value = strdup(function->args_list->args_array[j].value);
            }
        } else {
            configuration->array[i].arguments_len = 0;
            configuration->array[i].arguments = NULL;
        }
    }

    observePipelineAST_free(ast);

    return configuration;
}

void observePrintPipelineConfiguration(const ObservePipelineConfiguration *c) {
    assert(c != NULL);
    printf("[observe][parser] printPipelineConfiguration\n");
    for (size_t i = 0; i < c->len; i++) {
        ObservePipelineStage stage = c->array[i];
        if (stage.arguments_len == 0) {
            printf("\t%s %s() |\n", observeStagePipelineTypeToStr(stage.stage_type), stage.function_name);
        } else {
            printf("\t%s %s (\n", observeStagePipelineTypeToStr(stage.stage_type), stage.function_name);
            for (size_t j = 0; j < stage.arguments_len; j++) {
                printf("\t\t%s=%s,\n", stage.arguments[j].name, stage.arguments[j].value);
            }
            printf("\t) |\n");
        }
    }
}

void observeFreePipelineConfiguration(ObservePipelineConfiguration *c) {
    if (c == NULL) {
        return;
    }
    if (c->array != NULL) {
        for (size_t i = 0; i < c->len; i++) {
            free(c->array[i].function_name);
            for (size_t j = 0; j < c->array[i].arguments_len; j++) {
                free(c->array[i].arguments[j].name);
                free(c->array[i].arguments[j].value);
            }
            if (c->array[i].arguments != NULL) {
                free(c->array[i].arguments);
            }
        }
        free(c->array);
    }
    free(c);
}
