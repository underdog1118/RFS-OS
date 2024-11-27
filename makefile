all: rfs rfserver

rfs: rfs_client.c rfs.h
	gcc -o rfs rfs_client.c -Wall

rfserver: rfs_server.c rfs.h
	gcc -o rfserver rfs_server.c -Wall

clean:
	rm -f rfs rfserver
	rm -rf server_root