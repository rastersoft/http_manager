CC=gcc -c  -g
LN=gcc -g


http_manager: debug.o http.o http_manager.o process_list.o 
	$(LN) -o http_manager debug.o http.o http_manager.o process_list.o 

clean:
	rm -f *.o http_manager

install:
	cp http_manager /usr/bin
	chmod 755 /usr/bin/http_manager

copy:
	cp *.c *.h Makefile ../../gentoo/bg_apps/http_manager

debug.o: debug.c debug.h
	$(CC) -o debug.o debug.c

http.o: http.c http.h
	$(CC) -o http.o http.c

http_manager.o: http_manager.c process_list.h debug.h http.h http_manager.h
	$(CC) -o http_manager.o http_manager.c

process_list.o: process_list.c process_list.h
	$(CC) -o process_list.o process_list.c

