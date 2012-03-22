CCFLAGS           = `perl -MConfig -e 'print join(" ", @Config{qw(ccflags optimize cccdlflags)}, "-I$$Config{archlib}/CORE")'`
LDDLFLAGS         = `perl -MConfig -e 'print $$Config{lddlflags}'`

all: heartbeat.dylib

heartbeat_wrap.c heartbeat.pm:
	swig -perl5 heartbeat.i

heartbeat.o heartbeat_wrap.o: heartbeat_wrap.c
	gcc -c $(CCFLAGS) heartbeat.c heartbeat_wrap.c

heartbeat.dylib: heartbeat.o heartbeat_wrap.o
	gcc $(LDDLFLAGS) heartbeat.o heartbeat_wrap.o -o heartbeat.dylib

run: heartbeat.dylib heartbeat.pm
	perl -e 'use heartbeat; my $$thread = heartbeat::start_pacer("bar", 1); for (1..3) { print "MAIN: DO STUFF $_\n"; sleep(1); }; heartbeat::stop_pacer($$thread);'

main:
	gcc -o heartbeat heartbeat.c
	./heartbeat

clean:
	rm -f heartbeat_wrap.c heartbeat.o heartbeat.dylib heartbeat.pm heartbeat_wrap.o heartbeat
