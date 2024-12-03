#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "server.h"
#include "observe.h"


void observeCommand(client *c) {
    char *subcommand = NULL;
    if (c->argc < 2) {
        addReplyError(c, "no arguments provided");
        return;
    }
    subcommand = c->argv[1]->ptr;

    if (!strcasecmp(subcommand, "TAP-ATTACH")) {
        if (c->argc != 4 && c->argc != 5) {
            addReplyError(c, "invalid arguments");
            return;
        }
        robj *tap_name = c->argv[2];
        robj *attachment_name = c->argv[3];
        robj *lua_code = NULL;
        if (c->argc == 5) {
            lua_code = c->argv[4];
        }

        if (lua_code != NULL) {
            printf("tap-attach %s %s %s\n", (char*)tap_name->ptr, (char*)attachment_name->ptr, (char*)lua_code->ptr);
        } else {
            printf("tap-attach %s %s\n", (char*)tap_name->ptr, (char*)attachment_name->ptr);
        }

        server.observe->enabled = true;

        addReplyNull(c);
        return;
    }

    if (!strcasecmp(subcommand, "TAP-DETACH")) {
        if (c->argc != 4) {
            addReplyError(c, "invalid arguments");
            return;
        }
        robj *tap_name = c->argv[2];
        robj *attachment_name = c->argv[3];

        printf("tap-detach: %s %s\n", (char*)tap_name->ptr, (char*)attachment_name->ptr);

        server.observe->enabled = false;

        addReplyNull(c);
        return;
    }

    if (!strcasecmp(subcommand, "TAP-RETRIEVE")) {
        if (c->argc != 4) {
            addReplyError(c, "invalid arguments");
            return;
        }
        robj *tap_name = c->argv[2];
        robj *attachment_name = c->argv[3];

        printf("tap-retrieve: %s %s\n", (char*)tap_name->ptr, (char*)attachment_name->ptr);

        addReplyNull(c);
        return;
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

void observeProcessUnit(const observeUnit *unit) {
    // Print out the information about pipeline unit.
    observeProcessUnitPrint(unit);

    // Implement the unit pipeline processing.
    observePipelineData pd = {.unit = unit};

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
    // TODO: It should use the client->observe.enabled value to avoid concurrent rw.
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
