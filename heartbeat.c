
#define DEBUGPRINT(...) fprintf(stderr, __VA_ARGS__)

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h> // sleep
#include <errno.h>

#include "hiredis/hiredis.h"

typedef struct {
    char *ip;
    int port;
    char *key;
    int refresh_interval;
    int timeout;
} thread_info_t;

pthread_mutex_t pacing_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pacing_cond = PTHREAD_COND_INITIALIZER;
int pacing = 1;

void *pace(void *arg) {
    thread_info_t *thread_info = (thread_info_t *) arg;

    char *ip             = thread_info->ip;
    int port             = thread_info->port;
    char *key            = thread_info->key;
    int refresh_interval = thread_info->refresh_interval;
    int timeout          = thread_info->timeout;

    int rc;
    struct timeval now;
    struct timespec wait_until;

    free((void *)thread_info);
    DEBUGPRINT("THREAD: thread_info:\n\tip => %s\n\tport => %d\n\tkey => %s\n\trefresh_interval => %d\n\ttimeout => %d\n", ip, port, key, refresh_interval, timeout);

    redisContext *context = redisConnect(ip, port);
    if (context->err) {
        DEBUGPRINT("THREAD: Error: %s\n", context->errstr);
        redisFree(context);
        exit(1);
    }

    while (pacing) {
        DEBUGPRINT("THREAD: PULSE\n");

        //  renew the expiration on the key
        redisReply *reply = redisCommand(context, "EXPIRE %s %d", key, timeout);

        if (NULL == reply) {
            DEBUGPRINT("THREAD: Error: %s\n", context->errstr);
            freeReplyObject(reply);
            redisFree(context);
            exit(2);
        }
        switch(reply->type) {
            case REDIS_REPLY_STATUS:
                DEBUGPRINT("THREAD: REDIS_REPLY_STATUS\n");
                break;
            case REDIS_REPLY_ERROR:
                DEBUGPRINT("THREAD: REDIS_REPLY_ERROR\n");
                break;
            case REDIS_REPLY_INTEGER:
                DEBUGPRINT("THREAD: REDIS_REPLY_INTEGER = %lld\n", (long long) reply->integer);
                if (0 == reply->integer) {
                    DEBUGPRINT("THREAD: ERROR: Failed to set new expiration on '%s'.\n", key);
                    freeReplyObject(reply);
                    redisFree(context);
                    exit(3);
                }
                break;
            case REDIS_REPLY_NIL:
                DEBUGPRINT("THREAD: REDIS_REPLY_NIL\n");
                break;
            case REDIS_REPLY_STRING:
                DEBUGPRINT("THREAD: REDIS_REPLY_STRING\n");
                break;
            case REDIS_REPLY_ARRAY:
                DEBUGPRINT("THREAD: REDIS_REPLY_ARRAY\n");
                break;
            default:
                DEBUGPRINT("THREAD: DEFAULT\n");
                break;
        }

        gettimeofday(&now, NULL);
        wait_until.tv_sec  = now.tv_sec + refresh_interval;
        wait_until.tv_nsec = now.tv_usec * 1000;
        rc = pthread_cond_timedwait(&pacing_cond, &pacing_mutex, &wait_until);
        switch(rc) {
            case ETIMEDOUT:
                DEBUGPRINT("THREAD: pacing_cond timed out\n");
                break;
            case 0:
                DEBUGPRINT("THREAD: pacing_cond succeeded\n");
                break;
            default:
                DEBUGPRINT("THREAD: ERROR: %s!\n", strerror(rc));
                break;
        }
    }

    return NULL;
}

int stop_pacer(pthread_t thread) {
    pthread_mutex_lock(&pacing_mutex);
    pacing = 0;
    pthread_mutex_unlock(&pacing_mutex);
    pthread_cond_broadcast(&pacing_cond);
    return pthread_join(thread, NULL);
}

pthread_t start_pacer(char *ip, int port, char *key, int refresh_interval, int timeout) {
    pthread_t thread;
    int rc;

    thread_info_t *thread_info = (thread_info_t *) malloc(sizeof(thread_info_t));
    assert(NULL != thread_info);
    thread_info->ip               = ip;
    thread_info->port             = port;
    thread_info->key              = key;
    thread_info->refresh_interval = refresh_interval;
    thread_info->timeout          = timeout;

    DEBUGPRINT("MAIN: created thread (refresh_interval = %d).\n", thread_info->refresh_interval);
    rc = pthread_create(&thread, NULL, &pace, (void *) thread_info);
    assert(0 == rc);

    return thread;
}

int main(void) {
    int rc;

    char *key = "foo";
    char *ip = "127.0.0.1";
    pthread_t thread = start_pacer(ip, 6379, key, 2, 10);
    assert(NULL != thread);

    DEBUGPRINT("MAIN: sleep(5)\n");
    sleep(5);

    DEBUGPRINT("MAIN: cancelling thread\n");
    rc = stop_pacer(thread);
    assert(0 == rc);

    exit(EXIT_SUCCESS);
}
