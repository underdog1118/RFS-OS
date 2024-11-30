all: rfs rfserver

rfs: rfs_client.c rfs.h
	gcc -o rfs rfs_client.c -Wall -lpthread

rfserver: rfs_server.c rfs.h
	gcc -o rfserver rfs_server.c -Wall -lpthread

clean:
	rm -f rfs rfserver
	rm -rf server_root
