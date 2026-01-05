all: client_out server_out

client_out: game.c game.h shared.h
	gcc client/client.c game.c game.h shared.h -o client_out

server_out: game.c game.h shared.h
	gcc server/server.c game.c game.h shared.h -o server_out
	
clean:
	rm -f client_out server_out


