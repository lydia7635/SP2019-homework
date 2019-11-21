all: server.c
	gcc server.c -o write_server
	gcc server.c -D READ_SERVER -o read_server

account: init_account.c check_account.c
	gcc -std=c99 init_account.c -o init_account
	gcc -std=c99 check_account.c -o check_account

clean:
	rm -f read_server write_server
