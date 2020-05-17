all: main.c
	gcc -Wall -o main main.c -L/usr/lib -lssl -lcrypto
	gcc -o insert.cgi insert.c
	gcc -o web.cgi web.c