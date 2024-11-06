#ifndef __OBSERVE_H
#define __OBSERVE_H

#include "server.h"

typedef enum observePipelineStageType {
    OBSERVE_PIPELINE_STAGE_TYPE_UNKNOWN,
    OBSERVE_PIPELINE_STAGE_TYPE_FILTER,
    OBSERVE_PIPELINE_STAGE_TYPE_PARTITION,
    OBSERVE_PIPELINE_STAGE_TYPE_SAMPLE,
    OBSERVE_PIPELINE_STAGE_TYPE_WINDOW,
    OBSERVE_PIPELINE_STAGE_TYPE_REDUCE,
    OBSERVE_PIPELINE_STAGE_TYPE_OUTPUT,
} observePipelineStageType;

typedef enum observePipelineStageResult {
    OBSERVE_PIPELINE_STAGE_RESULT_UNKNOWN,
    OBSERVE_PIPELINE_STAGE_RESULT_OK,
    OBSERVE_PIPELINE_STAGE_RESULT_IGNORE,
} observePipelineStageResult;

typedef struct observePipelineStageFilter {
} observePipelineStageFilter;

typedef struct observePipelineStagePartition {
} observePipelineStagePartition;

typedef struct observePipelineStageSample {
} observePipelineStageSample;

typedef struct observePipelineStageWindow {
} observePipelineStageWindow;

typedef struct observePipelineStageReduce {
} observePipelineStageReduce;

typedef struct observePipelineStageOutput {
} observePipelineStageOutput;

typedef struct observePipelineConfigurationStage {
    observePipelineStageType stage_type;
    char *function_name;
    union {
        observePipelineStageFilter filter;
        observePipelineStageSample sample;
        observePipelineStagePartition partition;
        observePipelineStageWindow window;
        observePipelineStageReduce reduce;
        observePipelineStageOutput output;
    };
} observePipelineConfigurationStage;

typedef struct observePipelineConfiguration {
    observePipelineConfigurationStage *stages;
    size_t stages_len;
} observePipelineConfiguration;

typedef struct observePipeline {
    sds name;
    observePipelineConfiguration configuration;
} observePipeline;

observePipeline *observeNewPipeline(const char* name, size_t stages_len);
void observeFreePipeline(observePipeline *p);

typedef struct observeUnit {
    int command_id;

    robj **argv;
    size_t argv_len;

    size_t response_size_bytes;

    long long duration_microseconds;
} observeUnit;

typedef struct observePipelineData {
    const observeUnit *unit;

    // observeUnitReduced *unit_reduced;
} observePipelineData;

typedef struct observeClient {
    // TODO: We might need to store something inside the Client structure.
} observeClient;

observeClient* initObserveClient(void);
void freeObserveClient(observeClient* client);

// Command call wrappers

void observePostCommand(struct client *c, ustime_t duration);

// Observe Units processing

void observeProcessUnit(const observeUnit *unit);

typedef void (*observePipelineStage)(observePipelineData* data);

observePipelineStageResult observeProcessExecFilter(observePipelineConfigurationStage *cfg, observePipelineData *data);
observePipelineStageResult observeProcessExecSample(observePipelineConfigurationStage *cfg, observePipelineData *data);
observePipelineStageResult observeProcessExecPartition(observePipelineConfigurationStage *cfg, observePipelineData *data);
observePipelineStageResult observeProcessExecWindow(observePipelineConfigurationStage *cfg, observePipelineData *data);
observePipelineStageResult observeProcessExecReduce(observePipelineConfigurationStage *cfg, observePipelineData *data);
observePipelineStageResult observeProcessExecOutput(observePipelineConfigurationStage *cfg, observePipelineData *data);

typedef struct observePipelineResult {
    unsigned char *result;
    size_t result_len;
} observePipelineResult;

typedef struct observePipelineProcessor {
    observePipelineData *to_reduce;
    size_t to_reduce_len;

    observePipelineResult *results;
    size_t results_len;
} observePipelineProcessor;

// Observe Server

typedef struct observeServer {
    int enabled;
    observePipeline *pipeline;
} observeServer;

observeServer* initObserveServer(void);
void freeObserveServer(observeServer* client);

#endif /* __OBSERVE_H */
