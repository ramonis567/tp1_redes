all:
	gcc -Wall -c common.c -o common.o
	gcc -Wall client.c common.o -o client
	gcc -Wall server.c common.o -o server

clean:
	rm client server common.o
