all: server.c client.c
	gcc -Wall -o client client.c -L/usr/lib -lssl -lcrypto
	gcc -Wall -o server server.c -L/usr/lib -lssl -lcrypto