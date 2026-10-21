# CSC345 Virtual Memory Manager starter build
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2

.PHONY: all clean
all: main1 main2

main1: main1.c
	$(CC) $(CFLAGS) -o $@ $<

main2: main2.c
	$(CC) $(CFLAGS) -o $@ $<

# When main3.c is added, uncomment these lines and add main3 to all.
# main3: main3.c
# 	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f main1 main2 main3 out1.txt out2.txt out3.txt
