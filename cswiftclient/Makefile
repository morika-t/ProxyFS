CC=gcc

## TODO - review all Makefile entries

CFLAGS=-I. -I/opt/ss/include -fPIC -g
# The -lrt flag is needed to avoid a link error related to clock_* methods if glibc < 2.17
LDFLAGS += -ljson-c -lpthread -L/opt/ss/lib64 -lrt -lm
DEPS = proxyfs.h
LIBINSTALL?=/usr/lib
LIBINSTALL_CENTOS?=/usr/lib64
INCLUDEDIR?=/usr/include

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

all: libcswift.so.1.0.0 test

libcswift.so.1.0.0: cswift.o socket.o
	$(CC) -shared -fPIC -Wl,-soname,libcswift.so.1 -o libcswift.so.1.0.0 cswift.o socket.o $(LDFLAGS) -lc
	ln -f -s ./libcswift.so.1.0.0 ./libcswift.so.1
	ln -f -s ./libcswift.so.1.0.0 ./libcswift.so


test: cswift.o socket.o test.o
	$(CC) -o test cswift.o socket.o test.o $(CFLAGS) $(LDFLAGS)

install:
	cp -f libcswift.so.1.0.0 $(LIBINSTALL)/libcswift.so.1.0.0
	ln -f -s $(LIBINSTALL)/libcswift.so.1.0.0 $(LIBINSTALL)/libcswift.so.1
	ln -f -s $(LIBINSTALL)/libcswift.so.1.0.0 $(LIBINSTALL)/libcswift.so

installcentos:
	cp -f libcswift.so.1.0.0 $(LIBINSTALL_CENTOS)/libcswift.so.1.0.0
	ln -f -s $(LIBINSTALL_CENTOS)/libcswift.so.1.0.0 $(LIBINSTALL_CENTOS)/libcswift.so.1
	ln -f -s $(LIBINSTALL_CENTOS)/libcswift.so.1.0.0 $(LIBINSTALL_CENTOS)/libcswift.so

clean:
	rm -f *.o libcswift.so.1.0.0 ./libcswift.so.1 ./libcswift.so test
