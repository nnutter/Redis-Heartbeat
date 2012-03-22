%module heartbeat
%{
    extern pthread_t start_pacer(char *, int, char *, int, int);
    extern int stop_pacer(pthread_t *);
%}

extern pthread_t start_pacer(char *, int, char *, int, int);
extern int stop_pacer(pthread_t *);
