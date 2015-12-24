CFLAGS = -O0 -g -fPIC -DDL_LITTLE_ENDIAN
LDFLAGS = -fPIC

ifeq ($(shell uname), Darwin)
	CFLAGS += -D_DARWIN_USE_64_BIT_INODE
	LDFLAGS += -dynamiclib -undefined dynamic_lookup
	EXT = spark
endif
ifeq ($(shell uname), Linux)
	CFLAGS += -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
	LDFLAGS += -shared -Bsymbolic
	EXT = spark_x86_64
endif

all: ofxwrap.$(EXT)

ofxwrap.$(EXT): ofxwrap.o Makefile
	g++ $(LDFLAGS) ofxwrap.o -o ofxwrap.$(EXT)

ofxwrap.o: ofxwrap.cpp props.h dialogs.h ifxs.h parms.h half.h halfExport.h spark.h Makefile
	g++ $(CFLAGS) -c ofxwrap.cpp -o ofxwrap.o

spark.h: Makefile
	ln -sf `ls /usr/discreet/presets/*/sparks/spark.h | tail -n1` spark.h

clean:
	rm -f ofxwrap.spark ofxwrap.spark_x86_64 ofxwrap.o spark.h
