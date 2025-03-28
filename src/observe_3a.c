#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "server.h"
#include "observe.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
#include <string.h>

/* === ./src/valkey-benchmark -h localhost -p 6379 -t get,set -n 1000000 -c 20 ===
*/

const char *observeLuaFnCode =
"function observe_process_unit(observe_unit)\n"
"    -- Default case: return '0'\n"
"    return true\n"
"end\n";


// Global Lua state
lua_State *observeL;
int observeFnRef;
int requests = 0;
long long acc_time_nsec = 0;

// Initialize Lua environment
int observeLuaInit(void) {
    observeL = luaL_newstate();
    if (!observeL) {
        fprintf(stderr, "Failed to create Lua state\n");
        return -1;
    }
    luaL_openlibs(observeL);

    // Load Lua code
    if (luaL_dostring(observeL, observeLuaFnCode) != 0) {
        fprintf(stderr, "Error loading Lua code: %s\n", lua_tostring(observeL, -1));
        lua_close(observeL);
        return -1;
    }

    lua_getglobal(observeL, "observe_process_unit");
    observeFnRef = luaL_ref(observeL, LUA_REGISTRYINDEX);

    return 0;
}


// Push robj array to Lua as a table
void observeLuaPushRobjArray(lua_State *L, robj **argv, size_t argv_len) {
    lua_newtable(L); // Create a new table
    for (size_t i = 0; i < argv_len; i++) {
        lua_pushstring(L, argv[i]->ptr); // Push string value
        lua_rawseti(L, -2, i + 1);       // Set table[i+1] = argv[i]->str (1-based indexing in Lua)
    }
}

// Push observeUnit to Lua as a table
void observeLuaPushObserveUnit(lua_State *L, const observeUnit *unit) {
    lua_newtable(L); // Create a new table

    // Add command_id
    lua_pushstring(L, "command_id");
    lua_pushinteger(L, unit->command_id);
    lua_settable(L, -3);

    lua_pushstring(L, "request_id");
    lua_pushinteger(L, requests);
    lua_settable(L, -3);

    // Add argv
    // lua_pushstring(L, "argv");
    // observeLuaPushRobjArray(L, unit->argv, unit->argv_len);
    // lua_settable(L, -3);

    // Add response_size_bytes
    lua_pushstring(L, "response_size_bytes");
    lua_pushinteger(L, unit->response_size_bytes);
    lua_settable(L, -3);

    // Add duration_microseconds
    lua_pushstring(L, "duration_microseconds");
    lua_pushinteger(L, unit->duration_microseconds);
    lua_settable(L, -3);
}

// Run Lua function
int observeRunProcessLuaFn(const observeUnit *unit) {
    // lua_getglobal(observeL, "observe_process_unit"); // Get the Lua function
    lua_rawgeti(observeL, LUA_REGISTRYINDEX, observeFnRef);
    observeLuaPushObserveUnit(observeL, unit);       // Push the observeUnit table

    // Call the Lua function with 1 argument and 1 result
    if (lua_pcall(observeL, 1, 1, 0) != 0) {
        fprintf(stderr, "Error calling Lua function: %s\n", lua_tostring(observeL, -1));
        return 0;
    }

    // Get the result
    int lua_result = lua_toboolean(observeL, -1);
    lua_pop(observeL, 1);

    return lua_result;
}

// Clean up Lua environment
void observeLuaCleanup(void) {
    if (observeL) {
        lua_close(observeL);
        observeL = NULL;
    }
}


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
    // printf("[%d] process observe unit [command_id=%d]:", requests, unit->command_id);
    for (size_t i = 0; i < unit->argv_len; i++) {
        // printf(" '%s'", (char *)unit->argv[i]->ptr);
    }
    // printf(" | response_bytes=%ld", unit->response_size_bytes);
    float duration_ms = (float)unit->duration_microseconds / 1000;
    // printf(" | execution_time=%.3fms\n", duration_ms);
}

void observeProcessUnit(const observeUnit *unit) {
    // Run the Lua code for each Unit.
    // It filters out the observe unit if 0 is returned.
    if (observeRunProcessLuaFn(unit) == 0) {
        return;
    }

    // Print out the information about pipeline unit.
    observeProcessUnitPrint(unit);

    // Implement the unit pipeline processing.
    // observePipelineData pd = {.unit = unit};

    // TODO: Implement the units procesing inside the pipeline.
    // I guess, I can start with something simpler, not necessarily customizable via Lua scripts.

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
    // unit->response_size_bytes = 0;
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
    int lua_result = 0;
    struct timespec start, end;

    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        fprintf(stderr, "clock_gettime start failed");
        return;
    }

    lua_getglobal(observeL, "observe_process_unit");

    lua_newtable(observeL);

    if (lua_pcall(observeL, 1, 1, 0) == 0) {
        lua_result = lua_toboolean(observeL, -1);
    } else {
        fprintf(stderr, "Error calling Lua function: %s\n", lua_tostring(observeL, -1));
    }

    lua_pop(observeL, 1);

    if (lua_result) {
        // printf("[%d] process observe unit [command_id=%d]:", requests, c->cmd->id);
        for (int i = 0; i < c->argv_len; i++) {
            // printf(" '%s'", (char *)c->argv[i]->ptr);
        }
        // printf(" | response_bytes=%llu", c->net_output_bytes_curr_cmd);
        float duration_ms = (float)duration / 1000;
        // printf(" | execution_time=%.3fms\n", duration_ms);
    }

    ++requests;

    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        fprintf(stderr, "clock_gettime end failed");
        return;
    }

    long long elapsed_nsec = (end.tv_sec - start.tv_sec) * 1000000000LL +
                                 (end.tv_nsec - start.tv_nsec);
    acc_time_nsec += elapsed_nsec;

    if ((requests % 2000000) == 1) {
        double avg_nsec = (double) acc_time_nsec / requests;
        printf("Requests: %d Average time: %f nanoseconds\n", requests - 1, avg_nsec);
    }
}

/* Constructors / Destructors for observe structs. */

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

    if (observeLuaInit() != 0) {
        printf("FAILED TO INIT LUA\n");
    }

    return c;
}

void freeObserveServer(observeServer *client) {
    observeLuaCleanup();

    zfree(client);
}
