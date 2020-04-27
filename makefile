exe: utils.c utils.h srClient.c srServer.c srRelay.c packet.h config.h
	gcc srClient.c utils.c -o client
	gcc srServer.c utils.c -o server
	gcc srRelay.c utils.c -o relay

