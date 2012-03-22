#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h> // sleep

/* #include "vendor/hiredis/hiredis.h" */

typedef struct {
    char *key;
    int refresh_interval;
} thread_info_t;

void *pace(void *arg) {
    thread_info_t *thread_info = (thread_info_t *) arg;
    int refresh_interval = thread_info->refresh_interval;
    char *key = thread_info->key;
    free((void *)thread_info);

    while (1) {
        printf("THREAD: key => %s, refresh_interval => %d\n", key, refresh_interval);

        /*  renew the expiration on the key */
        /*  EXPIRE $key $timeout */

        if (0) {
            exit(200);
        }

        sleep(thread_info->refresh_interval);
    }

    return NULL;
}

int stop_pacer(pthread_t thread) {
    return pthread_cancel(thread);
}

pthread_t start_pacer(char *key, int refresh_interval) {
    pthread_t thread;
    int rc;

    thread_info_t *thread_info = (thread_info_t *) malloc(sizeof(thread_info_t));
    thread_info->refresh_interval = refresh_interval;
    thread_info->key = key;
    printf("MAIN: created thread (refresh_interval = %d).\n", thread_info->refresh_interval);
    rc = pthread_create(&thread, NULL, &pace, (void *) thread_info);
    assert(0 == rc);

    return thread;
}

int main(void) {
    int rc;

    char *key = "foo";
    pthread_t thread = start_pacer(key, 2);

    printf("MAIN: sleep(3)\n");
    sleep(5);

    printf("MAIN: cancelling thread\n");
    // Need to implement a cancel method that also frees struct?
    rc = stop_pacer(thread);
    assert(0 == rc);

    exit(EXIT_SUCCESS);
}
