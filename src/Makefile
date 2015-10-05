CC=gcc -c  -g
LN=gcc -g


http_manager: debug.o http.o http_manager.o 
	$(LN) -o http_manager debug.o http.o http_manager.o 

clean:
	rm -f *.o http_manager


debug.o: debug.c debug.h
	$(CC) -o debug.o debug.c

http.o: http.c http.h
	$(CC) -o http.o http.c

http_manager.o: http_manager.c debug.h http.h http_manager.h
	$(CC) -o http_manager.o http_manager.c

