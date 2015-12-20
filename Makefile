all: ofxwrap.spark

ofxwrap.spark: ofxwrap.o Makefile
	g++ -arch x86_64 -dynamiclib -undefined dynamic_lookup ofxwrap.o -o ofxwrap.spark

ofxwrap.o: ofxwrap.cpp props.h dialogs.h ifxs.h half.h halfExport.h
	g++ -fPIC -DDL_LITTLE_ENDIAN -D_DARWIN_USE_64_BIT_INODE -arch x86_64 -c ofxwrap.cpp -o ofxwrap.o

clean:
	rm ofxwrap.spark ofxwrap.o
