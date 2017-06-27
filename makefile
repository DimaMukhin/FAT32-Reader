fat32reader: main.o fat32.o bootsector.o directoryentry.o
	clang -Wall -g -o fat32reader main.o fat32.o bootsector.o directoryentry.o
	
main.o: main.c
	clang -Wall -g -c main.c
	
fat32.o: fat32.c
	clang -Wall -g -c fat32.c
	
bootsector.o: bootsector.c
	clang -Wall -g -c bootsector.c
	
directoryentry.o: directoryentry.c
	clang -Wall -g -c directoryentry.c
	
clean:
	rm -rf *o fat32reader