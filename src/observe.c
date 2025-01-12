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

const char *observeLuaFnCode =
"function observe_process_unit(observe_unit)\n"
"    -- Process all SET commands.\n"
"    if observe_unit.argv[1] == 'SET' then\n"
"        return '1'\n"
"    end\n"
"\n"
"    -- Process 5% of GET commands.\n"
"    if observe_unit.argv[1] == 'GET' then\n"
"        if math.random(1, 100) <= 5 then\n"
"            return '1'\n"
"        end\n"
"    end\n"
"\n"
"    -- Default case: return '0'\n"
"    return '0'\n"
"end\n";


// Global Lua state
lua_State *observeL;

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

    // Add argv
    lua_pushstring(L, "argv");
    observeLuaPushRobjArray(L, unit->argv, unit->argv_len);
    lua_settable(L, -3);

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
char* observeRunProcessLuaFn(const observeUnit *unit) {
    lua_getglobal(observeL, "observe_process_unit"); // Get the Lua function
    observeLuaPushObserveUnit(observeL, unit);       // Push the observeUnit table

    // Call the Lua function with 1 argument and 1 result
    if (lua_pcall(observeL, 1, 1, 0) != 0) {
        fprintf(stderr, "Error calling Lua function: %s\n", lua_tostring(observeL, -1));
        return NULL;
    }

    // Get the result
    const char *lua_result = lua_tostring(observeL, -1);
    if (!lua_result) {
        lua_pop(observeL, 1);  // Remove nil or non-string result
        return NULL;
    }

    // Dynamically allocate memory for the result
    char *result = zmalloc(strlen(lua_result) + 1);
    if (!result) {
        fprintf(stderr, "Memory allocation failed\n");
        lua_pop(observeL, 1);  // Clean up Lua stack
        return NULL;
    }

    valkey_strlcpy(result, lua_result, strlen(lua_result)+1); // Copy Lua result to allocated memory
    lua_pop(observeL, 1);                                     // Clean up Lua stack

    return result;
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
    printf("process observe unit [command_id=%d]:", unit->command_id);
    for (size_t i = 0; i < unit->argv_len; i++) {
        printf(" '%s'", (char *)unit->argv[i]->ptr);
    }
    printf(" | response_bytes=%ld", unit->response_size_bytes);
    float duration_ms = (float)unit->duration_microseconds / 1000;
    printf(" | execution_time=%.3fms\n", duration_ms);
}

int observeProcessUnitLua(const observeUnit *unit) {
    int should_process_unit = 0;
    char *result = observeRunProcessLuaFn(unit);
    if (result) {
        if (strcmp(result, "1") == 0) {
            should_process_unit = 1;
        }
        zfree(result);
    }
    return should_process_unit;
}

void observeProcessUnit(const observeUnit *unit) {
    // Run the Lua code for each Unit.
    // It filters out the observe unit if 0 is returned.
    if (observeProcessUnitLua(unit) == 0) {
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
