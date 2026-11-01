.PHONY: all clean

all: main1 main2 main3

main1: main1.c
	gcc -o main1 main1.c

main2: main2.c
	gcc -o main2 main2.c

main3: main3.c
	gcc -o main3 main3.c

clean:
	rm -f main1 main2 main3 out1.txt out2.txt out3.txt
