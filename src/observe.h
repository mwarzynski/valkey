#ifndef __OBSERVE_H
#define __OBSERVE_H

#include "server.h"


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
} observeServer;

observeServer* initObserveServer(void);
void freeObserveServer(observeServer* client);

#endif /* __OBSERVE_H */
