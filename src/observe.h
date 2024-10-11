#ifndef __OBSERVE_H
#define __OBSERVE_H

#include "server.h"

typedef struct observeServer {
    int enabled;
} observeServer;

typedef struct observeUnit {
    int command_id;

    robj **argv;
    size_t argv_len;

    size_t response_size_bytes;

    long long duration_microseconds;
} observeUnit;

typedef struct observeClient {
    // TODO: We might need to store something inside the Client structure.
} observeClient;

observeServer* initObserveServer(void);
void freeObserveServer(observeServer* client);

observeClient* initObserveClient(void);
void freeObserveClient(observeClient* client);

// Command call wrappers

void observePostCommand(struct client *c, ustime_t duration);

// Observe Units processing

void observeProcessUnit(const observeUnit *unit); // evaluates FILTER, SAMPLE, PARTITION, WINDOW, MAP, REDUCE, OUTPUT stages

#endif /* __OBSERVE_H */
