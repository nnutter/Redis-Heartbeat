PRODUCT = heartbeat

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifeq ($(uname_S),Linux)
	DYLIB_NAME       = $(PRODUCT).so
	HIREDIS_LD_FLAGS = -L/usr/lib -lhiredis
endif
ifeq ($(uname_S),Darwin)
	DYLIB_NAME       = $(PRODUCT).dylib
	HIREDIS_LD_FLAGS = -L/usr/local/lib -lhiredis
endif

CCFLAGS            = -Ivendor/hiredis `perl -MConfig -e 'print join(" ", @Config{qw(ccflags optimize cccdlflags)}, "-I$$Config{archlib}/CORE")'`
HEARTBEAT_LD_FLAGS = -Wl,-rpath,. -L. -lpthread
SWIG_LD_FLAGS      = $(HEARTBEAT_LD_FLAGS) `perl -MConfig -e 'print $$Config{lddlflags}'`
SWIG_WRAP          = $(PRODUCT)_wrap

all: $(DYLIB_NAME)

$(SWIG_WRAP).c $(PRODUCT).pm:
	swig -perl5 $(PRODUCT).i

$(PRODUCT).o $(SWIG_WRAP).o: $(SWIG_WRAP).c
	gcc -c $(CCFLAGS) $(PRODUCT).c $(SWIG_WRAP).c

$(DYLIB_NAME): $(PRODUCT).o $(SWIG_WRAP).o
	gcc $(PRODUCT).o $(SWIG_WRAP).o -o $(DYLIB_NAME) $(SWIG_LD_FLAGS) $(HIREDIS_LD_FLAGS)

run: $(DYLIB_NAME) $(PRODUCT).pm
	perl -e 'use $(PRODUCT); my $$thread = $(PRODUCT)::start_pacer("127.0.0.1", 6379, "bar", 1, 10); for (1..3) { print "MAIN: DO STUFF $_\n"; sleep(1); }; $(PRODUCT)::stop_pacer($$thread);'

$(PRODUCT):
	gcc $(CCFLAGS) -o $(PRODUCT) $(PRODUCT).c $(HEARTBEAT_LD_FLAGS) $(HIREDIS_LD_FLAGS)

clean:
	rm -f $(SWIG_WRAP).c $(PRODUCT).o $(DYLIB_NAME) $(PRODUCT).pm $(SWIG_WRAP).o $(PRODUCT)
