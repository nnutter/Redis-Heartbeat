#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h> // sleep

#include "vendor/hiredis/hiredis.h"

typedef struct {
    char *ip;
    int port;
    char *key;
    int refresh_interval;
    int timeout;
} thread_info_t;

void *pace(void *arg) {
    thread_info_t *thread_info = (thread_info_t *) arg;

    char *ip             = thread_info->ip;
    int port             = thread_info->port;
    char *key            = thread_info->key;
    int refresh_interval = thread_info->refresh_interval;
    int timeout          = thread_info->timeout;

    free((void *)thread_info);
    printf("THREAD: thread_info:\n\tip => %s\n\tport => %d\n\tkey => %s\n\trefresh_interval => %d\n\ttimeout => %d\n", ip, port, key, refresh_interval, timeout);

    redisContext *context = redisConnect(ip, port);
    if (context->err) {
        printf("THREAD: Error: %s\n", context->errstr);
        redisFree(context);
        exit(1);
    }

    while (1) {
        printf("THREAD: PULSE\n");

        //  renew the expiration on the key
        void *reply = redisCommand(context, "EXPIRE %s %d", key, timeout);

        if (NULL == reply) {
            printf("THREAD: Error: %s\n", context->errstr);
            freeReplyObject(reply);
            redisFree(context);
            exit(200);
        }

        sleep(thread_info->refresh_interval);
    }

    return NULL;
}

int stop_pacer(pthread_t thread) {
    return pthread_cancel(thread);
}

pthread_t start_pacer(char *ip, int port, char *key, int refresh_interval, int timeout) {
    pthread_t thread;
    int rc;

    thread_info_t *thread_info = (thread_info_t *) malloc(sizeof(thread_info_t));
    thread_info->ip               = ip;
    thread_info->port             = port;
    thread_info->key              = key;
    thread_info->refresh_interval = refresh_interval;
    thread_info->timeout          = timeout;

    printf("MAIN: created thread (refresh_interval = %d).\n", thread_info->refresh_interval);
    rc = pthread_create(&thread, NULL, &pace, (void *) thread_info);
    assert(0 == rc);

    return thread;
}

int main(void) {
    int rc;

    char *key = "foo";
    char *ip = "127.0.0.1";
    pthread_t thread = start_pacer(ip, 9000, key, 2, 10);

    printf("MAIN: sleep(5)\n");
    sleep(5);

    printf("MAIN: cancelling thread\n");
    rc = stop_pacer(thread);
    assert(0 == rc);

    exit(EXIT_SUCCESS);
}
