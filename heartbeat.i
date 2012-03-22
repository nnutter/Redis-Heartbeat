%module heartbeat
%{
    extern pthread_t start_pacer(char *, int);
    extern int stop_pacer(pthread_t *);
%}

extern pthread_t start_pacer(char *, int);
extern int stop_pacer(pthread_t *);
