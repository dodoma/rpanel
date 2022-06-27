#ifndef __DB_H__
#define __DB_H__

#include "hiredis.h"

__BEGIN_DECLS

MERR* dbConnect(const char *ip, int port);
MERR* dbConnectUnix(const char *path);
void dbFree();
void dbExecNoReply(const char *fmt, ...) ATTRIBUTE_PRINTF(1, 2);
redisReply* dbExec(const char *fmt, ...) ATTRIBUTE_PRINTF(1, 2);
/* TODO add default int value parameter */
int64_t dbGetInt(const char *fmt, ...) ATTRIBUTE_PRINTF(1, 2);

__END_DECLS
#endif
