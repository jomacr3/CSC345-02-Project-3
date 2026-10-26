all:
	gcc -o main1 main1.c
	gcc -o main2 main2.c
clean:
	rm -f main1 main2 main3 out1.txt out2.txt out3.txt
