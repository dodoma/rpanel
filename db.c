#include "reef.h"
#include "hiredis.h"

#include "db.h"

static redisContext *conn;

/*
 * 所有的节点值，请以 string 方式设置，不然无法设置成功，例如
 *  _hmset(key, "{id: '%s', statu: '%d', errormsg: '%s'}", id, statu, msg); is ok
 *  _hmset(key, "{id: '%s', statu: %d, errormsg: '%s'}", id, statu, msg);   is nok
 */
static MERR* _hmset(char *key, const char *fmt, ...)
{
    char buf[1024], *val;
    int len;
    va_list ap, tmpap;
    MERR *err;

    va_start(ap, fmt);
    va_copy(tmpap, ap);
    len = vsnprintf(buf, 1024, fmt, tmpap);
    if (len >= 1024) {
        val = (char*)mos_malloc(len);
        vsprintf(val, fmt, ap);
    } else val = buf;
    va_end(ap);

    MDF *anode;
    mdf_init(&anode);

#define RETURN(ret)                             \
    do {                                        \
        if (len >= 1024) mos_free(val);         \
        mdf_destroy(&anode);                    \
        return (ret);                           \
    } while (0)

    err = mdf_json_import_string(anode, val);
    if (err != MERR_OK) RETURN(merr_pass(err));

    int argnum = mdf_child_count(anode, NULL);
    if (argnum <= 0) RETURN(merr_raise(MERR_ASSERT, "HMSET 参数错误"));

    int maxnum = argnum * 2 + 2;
    char *argv[maxnum];
    size_t argvlen[maxnum];

    memset(argv, 0x0, sizeof(argv));
    memset(argvlen, 0x0, sizeof(argvlen));

    argv[0] = (char*)"HMSET";
    argvlen[0] = 5;

    argv[1] = key;
    argvlen[1] = strlen(key);

    int anum = 2;
    MDF *cnode = mdf_get_child(anode, NULL);
    while (cnode && anum < maxnum)  {
        char *k = mdf_get_name(cnode, NULL);
        char *v = mdf_get_value(cnode, NULL, NULL);

        mtc_mm_noise("main", "k %s, v %s", k, v);

        if (!k || !v) goto nextnode;

        argv[anum] = k;
        argvlen[anum] = strlen(k);
        argv[anum + 1] = v;
        argvlen[anum + 1] = strlen(v);

        anum += 2;

    nextnode:
        cnode = mdf_node_next(cnode);
    }

    mtc_mm_noise("main", "argnum %d ==> %d ", argnum, anum);

    if (conn->err != REDIS_OK) {
        mtc_mm_err("main", "redis nok %s, refresh", conn->errstr);
        conn->err = REDIS_OK;
    }

    int rv = redisAppendCommandArgv(conn, anum, (const char**)argv, argvlen);
    if (rv != REDIS_OK) RETURN(merr_raise(MERR_ASSERT, "%s", conn->errstr));

    redisReply *reply;
    rv = redisGetReply(conn, (void**)&reply);
    if (rv != REDIS_OK || !reply) RETURN(merr_raise(MERR_ASSERT, "%s", conn->errstr));
    freeReplyObject(reply);

    RETURN(MERR_OK);

#undef RETURN
}

MERR* dbConnect(const char *ip, int port)
{
    MERR_NOT_NULLA(ip);

    if (!conn) {
        struct timeval tv = {0, 500000};
        conn = redisConnectWithTimeout(ip, port, tv);
        if (!conn || conn->err) {
            if (conn) return merr_raise(MERR_ASSERT, "connection error %s", conn->errstr);
            else return merr_raise(MERR_ASSERT, "connect to %s %d failure", ip, port);
        }
    }

    return MERR_OK;
}

MERR* dbConnectUnix(const char *path)
{
    MERR_NOT_NULLA(path);

    if (!conn) {
        struct timeval tv = {0, 500000};
        conn = redisConnectUnixWithTimeout(path, tv);
        if (!conn || conn->err) {
            if (conn) return merr_raise(MERR_ASSERT, "connection error %s", conn->errstr);
            else return merr_raise(MERR_ASSERT, "connect to %s failure", path);
        }
    }

    return MERR_OK;
}

void dbFree()
{
    if (conn) {
        redisFree(conn);
        conn = NULL;
    }
}

void dbExecNoReply(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    if (conn->err != REDIS_OK) {
        mtc_mm_err("main", "redis nok %s, refresh", conn->errstr);
        conn->err = REDIS_OK;
    }

    redisReply *reply = (redisReply*)redisvCommand(conn, fmt, ap);
    if (reply->type == REDIS_REPLY_ERROR) {
        mtc_mm_err("main", "%s %s", fmt, reply->str);
    }
    freeReplyObject(reply);

    va_end(ap);
}

redisReply* dbExec(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    if (conn->err != REDIS_OK) {
        mtc_mm_err("main", "redis nok %s, refresh", conn->errstr);
        conn->err = REDIS_OK;
    }

    redisReply *reply = (redisReply*)redisvCommand(conn, fmt, ap);

    va_end(ap);

    return reply;
}

int64_t dbGetInt(const char *fmt, ...)
{
    int64_t ret = 0;

    va_list ap;
    va_start(ap, fmt);

    if (conn->err != REDIS_OK) {
        mtc_mm_err("main", "redis nok %s, refresh", conn->errstr);
        conn->err = REDIS_OK;
    }

    redisReply *reply = (redisReply*)redisvCommand(conn, fmt, ap);
    if (reply->type == REDIS_REPLY_INTEGER) ret = (int64_t)reply->integer;
    else if (reply->type == REDIS_REPLY_STRING) ret = strtoll(reply->str, NULL, 10);
    freeReplyObject(reply);

    return ret;
}
