UNAME := $(shell uname)

CC = gcc
ifeq ($(UNAME), Linux)
	CFLAGS = -lpthread -lnsl
else
	CFLAGS = -lpthread -lnsl -lsocket
endif

all: myproxy

myproxy:myproxy.c
	gcc -o $@ $< $(CFLAGS)

clean:
	rm myproxy
