fat32reader: main.o
	clang -Wall -g -o fat32reader main.o
	
main.o: main.c
	clang -Wall -g -c main.c
	
clean:
	rm -rf *o fat32reader