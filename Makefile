CCFLAGS   = -Ivendor/hiredis `perl -MConfig -e 'print join(" ", @Config{qw(ccflags optimize cccdlflags)}, "-I$$Config{archlib}/CORE")'`
LDDLFLAGS = -lpthread -Lvendor/hiredis -lhiredis `perl -MConfig -e 'print $$Config{lddlflags}'`
MAIN_LDDLFLAGS = -lpthread -Lvendor/hiredis -lhiredis

all: heartbeat.dylib

heartbeat_wrap.c heartbeat.pm:
	swig -perl5 heartbeat.i

heartbeat.o heartbeat_wrap.o: heartbeat_wrap.c
	gcc -c $(CCFLAGS) heartbeat.c heartbeat_wrap.c

heartbeat.dylib: heartbeat.o heartbeat_wrap.o
	gcc $(LDDLFLAGS) heartbeat.o heartbeat_wrap.o -o heartbeat.dylib

run: heartbeat.dylib heartbeat.pm
	perl -I vendor/hiredis -e 'use heartbeat; my $$thread = heartbeat::start_pacer('127.0.0.1', 6379, "bar", 1, 10); for (1..3) { print "MAIN: DO STUFF $_\n"; sleep(1); }; heartbeat::stop_pacer($$thread);'

hiredis:
	cd vendor/hiredis && make

main:
	gcc $(CCFLAGS) $(MAIN_LDDLFLAGS) -o heartbeat heartbeat.c
	install_name_tool -add_rpath vendor/hiredis heartbeat
	cd vendor/hiredis && ln -sf libhiredis.dylib libhiredis.0.10.dylib

clean:
	rm -f heartbeat_wrap.c heartbeat.o heartbeat.dylib heartbeat.pm heartbeat_wrap.o heartbeat
