#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "server.h"
#include "observe.h"
#include "observe_pipeline_parser.h"


observePipeline *observeNewPipeline(const char *name, size_t stages_len) {
    observePipeline *p = (observePipeline *)zmalloc(sizeof(observePipeline));

    p->name = zstrdup(name);
    assert(p->name != NULL);

    p->configuration.stages = (observePipelineConfigurationStage *)zmalloc(sizeof(observePipelineConfigurationStage) * stages_len);
    assert(p->configuration.stages != NULL);
    p->configuration.stages_len = stages_len;

    return p;
}

void observeFreePipeline(observePipeline *p) {
    if (p == NULL) {
        return;
    }
    if (p->configuration.stages != NULL) {
        zfree(p->configuration.stages);
    }
    if (p->name != NULL) {
        zfree(p->name);
    }
    zfree(p);
}

int observeCommandConfigure(const char *name, const char *input) {
    observePipelineParserConfiguration *pc = observeParsePipelineConfiguration(input);
    if (pc == NULL) {
        printf("Error parsing expression\n");
        return 1;
    }

    // Move raw parsed pipeline configuration into our internal representation.
    observePipeline *p = observeNewPipeline(name, pc->len);
    for (size_t i = 0; i < pc->len; i++) {
        p->configuration.stages[i].stage_type = pc->array[i].stage_type;
        p->configuration.stages[i].function_name = zstrdup(pc->array[i].function_name);
        assert(p->configuration.stages[i].function_name != NULL);
    }

    if (server.observe->pipeline != NULL) {
        observeFreePipeline(server.observe->pipeline);
        server.observe->pipeline = NULL;
    }
    server.observe->pipeline = p;

    observeFreeParsedPipelineConfiguration(pc);
    return 0;
}

void observeCommand(client *c) {
    char *subcommand = NULL;
    if (c->argc < 2) {
        addReplyError(c, "no arguments provided");
        return;
    }
    subcommand = c->argv[1]->ptr;

    if (!strcasecmp(subcommand, "create")) {
        if (c->argc != 3) {
            addReplyError(c, "err");
            return;
        }
        char *name = c->argv[2]->ptr;
        serverLog(LL_VERBOSE, "[observe] %s %s", subcommand, name);
    }

    if (!strcasecmp(subcommand, "delete")) {
        if (c->argc != 3) {
            addReplyError(c, "err");
            return;
        }
        char *name = c->argv[2]->ptr;
        serverLog(LL_VERBOSE, "[observe] %s %s", subcommand, name);
    }

    if (!strcasecmp(subcommand, "start")) {
        if (c->argc != 3) {
            addReplyError(c, "err");
            return;
        }
        server.observe->enabled = 1;
        char *name = c->argv[2]->ptr;
        serverLog(LL_VERBOSE, "[observe] %s %s", subcommand, name);
    }

    if (!strcasecmp(subcommand, "stop")) {
        if (c->argc != 3) {
            addReplyError(c, "err");
            return;
        }
        server.observe->enabled = 0;
        char *name = c->argv[2]->ptr;
        serverLog(LL_VERBOSE, "[observe] %s %s", subcommand, name);
    }

    if (!strcasecmp(subcommand, "configure")) {
        if (c->argc != 4) {
            addReplyError(c, "err");
            return;
        }
        char *name = c->argv[2]->ptr;
        char *pipeline_str = c->argv[3]->ptr;
        // serverLog(LL_VERBOSE, "[observe] %s %s %s", subcommand, name, pipeline_str);

        if (observeCommandConfigure(name, pipeline_str) != 0) {
            serverLog(LL_VERBOSE, "[observe] %s %s '%s' [error]", subcommand, name, pipeline_str);
            addReplyError(c, "failed to parse the expression");
            return;
        } else {
            serverLog(LL_VERBOSE, "[observe] %s %s '%s' -> OK?", subcommand, name, pipeline_str);
            addReplyNull(c);
            return;
        }
    }

    addReplyNull(c);
    return;
}


/* Observe units and pipeline execution */

void observeProcessUnitPrint(const observeUnit *unit) {
    printf("process observe unit [command_id=%d]:", unit->command_id);
    for (size_t i = 0; i < unit->argv_len; i++) {
        printf(" '%s'", (char *)unit->argv[i]->ptr);
    }
    printf(" | response_bytes=%ld", unit->response_size_bytes);
    float duration_ms = (float)unit->duration_microseconds / 1000;
    printf(" | execution_time=%.3fms\n", duration_ms);
}

observePipelineStageResult observeProcessExecFilter(observePipelineConfigurationStage *cfg, observePipelineData *data) {
    return OBSERVE_PIPELINE_STAGE_RESULT_OK;
}

observePipelineStageResult observeProcessExecSample(observePipelineConfigurationStage *cfg, observePipelineData *data) {
    return OBSERVE_PIPELINE_STAGE_RESULT_OK;
}

observePipelineStageResult observeProcessExecPartition(observePipelineConfigurationStage *cfg, observePipelineData *data) {
    return OBSERVE_PIPELINE_STAGE_RESULT_OK;
}

observePipelineStageResult observeProcessExecWindow(observePipelineConfigurationStage *cfg, observePipelineData *data) {
    return OBSERVE_PIPELINE_STAGE_RESULT_OK;
}

observePipelineStageResult observeProcessExecReduce(observePipelineConfigurationStage *cfg, observePipelineData *data) {
    return OBSERVE_PIPELINE_STAGE_RESULT_OK;
}

observePipelineStageResult observeProcessExecOutput(observePipelineConfigurationStage *cfg, observePipelineData *data) {
    return OBSERVE_PIPELINE_STAGE_RESULT_OK;
}

observePipelineStageResult observeProcessPipelineStage(observePipeline *p, size_t stage_i, observePipelineData *d) {
    if (p->configuration.stages[stage_i].stage_type == OBSERVE_PIPELINE_STAGE_TYPE_FILTER) {
        return observeProcessExecFilter(&p->configuration.stages[stage_i], d);
    }
    if (p->configuration.stages[stage_i].stage_type == OBSERVE_PIPELINE_STAGE_TYPE_SAMPLE) {
        return observeProcessExecSample(&p->configuration.stages[stage_i], d);
    }
    if (p->configuration.stages[stage_i].stage_type == OBSERVE_PIPELINE_STAGE_TYPE_PARTITION) {
        return observeProcessExecPartition(&p->configuration.stages[stage_i], d);
    }
    if (p->configuration.stages[stage_i].stage_type == OBSERVE_PIPELINE_STAGE_TYPE_WINDOW) {
        return observeProcessExecWindow(&p->configuration.stages[stage_i], d);
    }
    if (p->configuration.stages[stage_i].stage_type == OBSERVE_PIPELINE_STAGE_TYPE_REDUCE) {
        return observeProcessExecReduce(&p->configuration.stages[stage_i], d);
    }
    if (p->configuration.stages[stage_i].stage_type == OBSERVE_PIPELINE_STAGE_TYPE_OUTPUT) {
        return observeProcessExecOutput(&p->configuration.stages[stage_i], d);
    }
    return OBSERVE_PIPELINE_STAGE_RESULT_UNKNOWN;
}

void observeProcessUnit(const observeUnit *unit) {
    // Print out the information about pipeline unit.
    observeProcessUnitPrint(unit);

    // Implement the unit pipeline processing.
    if (server.observe->pipeline == NULL) {
        // Nothing to do if there is no pipeline configuration.
        return;
    }

    observePipelineData pd = {.unit = unit};
    observePipeline *p = server.observe->pipeline;

    for (size_t i = 0; i < p->configuration.stages_len; i++) {
        observePipelineStageResult result = observeProcessPipelineStage(p, i, &pd);
        if (result == OBSERVE_PIPELINE_STAGE_RESULT_IGNORE) {
            printf("Stage: filter() => IGNORE\n");
            break;
        } else if (result == OBSERVE_PIPELINE_STAGE_RESULT_OK) {
            continue;
        } else if (result == OBSERVE_PIPELINE_STAGE_RESULT_UNKNOWN) {
            printf("Stage: unknown => IGNORE\n");
            break;
        }
    }

    // TODO: Implement the units procesing inside the pipeline.
    // I guess, I can start with something simpler, not necessarily customizable via Lua scripts.
    printf("\tobserve pipeline result => %s\n", "todo(): pipeline executor");

    return;
}

/* Pre-Post command execution Observe actions */

void initObserveUnit(client *c, observeUnit *unit, ustime_t duration) {
    unit->command_id = c->cmd->id;
    unit->argv_len = c->argv_len;
    unit->argv = (robj **)zmalloc(sizeof(robj *) * c->argv_len);
    if (unit->argv == NULL) {
        serverPanic("Cannot allocate memory for observe unit argv");
    }
    for (int i = 0; i < c->argv_len; i++) {
        unit->argv[i] = dupStringObject(c->argv[i]);
        if (unit->argv[i] == NULL) {
            serverPanic("Cannot copy the command argument");
        }
    }
    unit->response_size_bytes = 0;
    unit->response_size_bytes = c->net_output_bytes_curr_cmd;
    unit->duration_microseconds = duration;
}

void deallocObserveUnitFields(observeUnit *unit) {
    if (unit == NULL) {
        return;
    }
    if (unit->argv != NULL) {
        for (size_t i = 0; i < unit->argv_len; i++) {
            decrRefCount(unit->argv[i]);
        }
        zfree(unit->argv);
    }
    unit->argv = NULL;
    unit->response_size_bytes = 0;
}

void observePostCommand(client *c, ustime_t duration) {
    if (!server.observe->enabled) {
        return;
    }

    // Allocate memory for the observe unit internal fields.
    observeUnit unit;
    initObserveUnit(c, &unit, duration);

    // Process the observe unit through the pipeline.
    observeProcessUnit(&unit);

    // Deallocate memory for the observe unit internal fields.
    deallocObserveUnitFields(&unit);
}

/* Constructors for observe structs. */

observeClient *initObserveClient(void) {
    observeClient *c = zmalloc(sizeof(observeClient));
    assert(c != NULL);
    return c;
}

void freeObserveClient(observeClient *client) {
    zfree(client);
}

observeServer *initObserveServer(void) {
    observeServer *c = zmalloc(sizeof(observeServer));
    assert(c != NULL);
    c->enabled = 0;
    return c;
}

void freeObserveServer(observeServer *client) {
    zfree(client);
}
