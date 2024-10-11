#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "server.h"
#include "observe.h"
#include "observe_pipeline_parser.h"


int run_observe_test(const char *input) {
    ObservePipelineConfiguration *c = observeParsePipelineConfiguration(input);
    if (c == NULL) {
        printf("Error parsing expression\n");
        return 1;
    }
    observePrintPipelineConfiguration(c);
    observeFreePipelineConfiguration(c);
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

        if (run_observe_test(pipeline_str) != 0) {
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

void observeProcessUnit(const observeUnit* unit) {
    printf("process observe unit [command_id=%d]:", unit->command_id);
    for (size_t i = 0; i < unit->argv_len; i++) {
        printf(" '%s'", (char *)unit->argv[i]->ptr);
    }
    printf(" | response_bytes=%ld", unit->response_size_bytes);
    float duration_ms = (float)unit->duration_microseconds / 1000;
    printf(" | execution_time=%.3fms\n", duration_ms);
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

observeClient* initObserveClient(void) {
    observeClient *c = zmalloc(sizeof(observeClient));
    assert(c != NULL);
    return c;
}

void freeObserveClient(observeClient* client) {
    zfree(client);
}

observeServer* initObserveServer(void) {
    observeServer *c = zmalloc(sizeof(observeServer));
    assert(c != NULL);
    c->enabled = 0;
    return c;
}

void freeObserveServer(observeServer* client) {
    zfree(client);
}
