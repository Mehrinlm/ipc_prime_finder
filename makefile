shiva: transport.cc
	g++ transport.cc -o shiva -lnsl -lsocket -lresolv

things: transport.cc
	g++ transport.cc -o things

clean:
	rm -f things shiva
