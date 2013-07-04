# FILE:           Makefile
# AUTHOR:         Mark Birger, FIT VUTBR
# DATE:           10.4.2013
# DESCRIPTION:    Santa Claus Problem

CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic -lpthread 

all: santa


santa: santa.c
	$(CC) $(CFLAGS) santa.c -o santa

clean:
	rm -f *.o santa