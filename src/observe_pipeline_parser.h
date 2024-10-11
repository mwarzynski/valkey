#ifndef OBSERVE_PIPELINE_PARSER_H
#define OBSERVE_PIPELINE_PARSER_H

#include <stddef.h>

#define OBSERVE_PIPELINE_STAGE_FILTER_STR_REPR "filter"
#define OBSERVE_PIPELINE_STAGE_PARTITION_STR_REPR "partition"
#define OBSERVE_PIPELINE_STAGE_SAMPLE_STR_REPR "sample"
#define OBSERVE_PIPELINE_STAGE_MAP_STR_REPR "map"
#define OBSERVE_PIPELINE_STAGE_WINDOW_STR_REPR "window"
#define OBSERVE_PIPELINE_STAGE_REDUCE_STR_REPR "reduce"
#define OBSERVE_PIPELINE_STAGE_OUTPUT_STR_REPR "output"

typedef enum tagObservePipelineStageType {
    OBSERVE_PIPELINE_STAGE_UNKNOWN,
    OBSERVE_PIPELINE_STAGE_FILTER,
    OBSERVE_PIPELINE_STAGE_PARTITION,
    OBSERVE_PIPELINE_STAGE_SAMPLE,
    OBSERVE_PIPELINE_STAGE_MAP,
    OBSERVE_PIPELINE_STAGE_WINDOW,
    OBSERVE_PIPELINE_STAGE_REDUCE,
    OBSERVE_PIPELINE_STAGE_OUTPUT,
} ObservePipelineStageType;

typedef struct tagObservePipelineArgument {
    char *name;
    char *value;
} ObservePipelineArgument;

typedef struct tagObservePipelineStage {
    ObservePipelineStageType stage_type;
    char *function_name;
    ObservePipelineArgument *arguments;
    size_t arguments_len;
} ObservePipelineStage;

typedef struct tagObservePipelineConfiguration {
    ObservePipelineStage *array;
    size_t len;
} ObservePipelineConfiguration;

ObservePipelineConfiguration *observeParsePipelineConfiguration(const char *str);
void observePrintPipelineConfiguration(const ObservePipelineConfiguration *c);
void observeFreePipelineConfiguration(ObservePipelineConfiguration *c);

#endif
