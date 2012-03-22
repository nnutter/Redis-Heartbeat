CCFLAGS   = -Ivendor/hiredis `perl -MConfig -e 'print join(" ", @Config{qw(ccflags optimize cccdlflags)}, "-I$$Config{archlib}/CORE")'`
MAIN_LDDLFLAGS = -Wl,-rpath,vendor/hiredis,-rpath,. -L. -lpthread -Lvendor/hiredis -lhiredis
LDDLFLAGS = $(MAIN_LDDLFLAGS) `perl -MConfig -e 'print $$Config{lddlflags}'`

PRODUCT                    = heartbeat
SWIG_WRAP                  = $(PRODUCT)_wrap
DYLIB_NAME                 = $(PRODUCT).so
HIREDIS_DYLIB_NAME         = libhiredis.so
HIREDIS_DYLIB_INSTALL_NAME = libhiredis.so.0.10

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifeq ($(uname_S),Darwin)
	DYLIB_NAME                 = $(PRODUCT).dylib
	HIREDIS_DYLIB_NAME         = libhiredis.dylib
	HIREDIS_DYLIB_INSTALL_NAME = libhiredis.0.10.dylib
endif

all: $(DYLIB_NAME)

$(SWIG_WRAP).c $(PRODUCT).pm:
	swig -perl5 $(PRODUCT).i

$(PRODUCT).o $(SWIG_WRAP).o: $(SWIG_WRAP).c
	gcc -c $(CCFLAGS) $(PRODUCT).c $(SWIG_WRAP).c

$(DYLIB_NAME): $(PRODUCT).o $(SWIG_WRAP).o
	gcc $(PRODUCT).o $(SWIG_WRAP).o -o $(DYLIB_NAME) $(LDDLFLAGS)

run: $(DYLIB_NAME) $(PRODUCT).pm
	perl -I vendor/hiredis -e 'use $(PRODUCT); my $$thread = $(PRODUCT)::start_pacer("127.0.0.1", 6379, "bar", 1, 10); for (1..3) { print "MAIN: DO STUFF $_\n"; sleep(1); }; $(PRODUCT)::stop_pacer($$thread);'

$(HIREDIS_DYLIB_NAME):
	cd vendor/hiredis && make && ln -sf $(HIREDIS_DYLIB_NAME) $(HIREDIS_DYLIB_INSTALL_NAME)

$(PRODUCT): $(HIREDIS_DYLIB_NAME)
	gcc $(CCFLAGS) -o $(PRODUCT) $(PRODUCT).c $(MAIN_LDDLFLAGS)

clean:
	rm -f $(SWIG_WRAP).c $(PRODUCT).o $(DYLIB_NAME) $(PRODUCT).pm $(SWIG_WRAP).o $(PRODUCT)
