all: client_app server_app

client_app:
	gcc client/client.c -o client_app

server_app:
	gcc server/server.c -o server_app

clean:
	rm -f client_app server_app


