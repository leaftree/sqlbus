
all:client

client: *.c *.h
	gcc -g -O3 -Wall -Werror *.c -L ../lib -lhiredis -lcjson -o client -D HI_SQL_BUS_DEBUG_MAIN

clean:
	rm -f client
