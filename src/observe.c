#include "server.h"
#include "observe.h"

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
        char *name = c->argv[2]->ptr;
        serverLog(LL_VERBOSE, "[observe] %s %s", subcommand, name);
    }

    if (!strcasecmp(subcommand, "stop")) {
        if (c->argc != 3) {
            addReplyError(c, "err");
            return;
        }
        char *name = c->argv[2]->ptr;
        serverLog(LL_VERBOSE, "[observe] %s %s", subcommand, name);
    }

    if (!strcasecmp(subcommand, "configure")) {
        if (c->argc != 4) {
            addReplyError(c, "err");
            return;
        }
        char *name = c->argv[2]->ptr;
        char *step = c->argv[3]->ptr;
        serverLog(LL_VERBOSE, "[observe] %s %s %s", subcommand, name, step);
    }

    addReplyNull(c);
    return;
}
