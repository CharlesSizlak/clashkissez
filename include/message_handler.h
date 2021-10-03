#ifndef _MESSAGE_HANDLER_H
#define _MESSAGE_HANDLER_H

#include "loop.h"
#include "contexts.h"

void message_handler(loop_t *loop, int fd, connection_ctx_t *connection_ctx);

#endif
