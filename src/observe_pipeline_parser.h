#ifndef OBSERVE_PIPELINE_PARSER_H
#define OBSERVE_PIPELINE_PARSER_H

#include <stddef.h>

#include "observe.h"

#define OBSERVE_PIPELINE_STAGE_FILTER_STR_REPR "filter"
#define OBSERVE_PIPELINE_STAGE_PARTITION_STR_REPR "partition"
#define OBSERVE_PIPELINE_STAGE_SAMPLE_STR_REPR "sample"
#define OBSERVE_PIPELINE_STAGE_WINDOW_STR_REPR "window"
#define OBSERVE_PIPELINE_STAGE_REDUCE_STR_REPR "reduce"
#define OBSERVE_PIPELINE_STAGE_OUTPUT_STR_REPR "output"

typedef struct observePipelineParserArgument {
    char *name;
    char *value;
} observePipelineParserArgument;

typedef struct observePipelineParserStage {
    observePipelineStageType stage_type;
    char *function_name;
    observePipelineParserArgument *arguments;
    size_t arguments_len;
} observePipelineParserStage;

typedef struct observePipelineParserConfiguration {
    observePipelineParserStage *array;
    size_t len;
} observePipelineParserConfiguration;

observePipelineParserConfiguration *observeParsePipelineConfiguration(const char *str);
void observePrintParsedPipelineConfiguration(const observePipelineParserConfiguration *c);
void observeFreeParsedPipelineConfiguration(observePipelineParserConfiguration *c);

#endif
