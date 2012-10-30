UNAME := $(shell uname)

all: prime_finder.cc
ifeq ($(UNAME), Linux)
	g++ prime_finder.cc -o things
endif
ifeq ($(UNAME), SunOS)
	g++ prime_finder.cc -o shiva -lnsl -lsocket -lresolv
endif

clean:
	rm -f things shiva
