fat32reader: main.o fat32.o bootsector.o directoryentry.o fsinfo.o
	clang -Wall -g -o fat32reader main.o fat32.o bootsector.o directoryentry.o fsinfo.o
	
main.o: main.c
	clang -Wall -g -c main.c
	
fat32.o: fat32.c
	clang -Wall -g -c fat32.c
	
bootsector.o: bootsector.c
	clang -Wall -g -c bootsector.c
	
directoryentry.o: directoryentry.c
	clang -Wall -g -c directoryentry.c
	
fsinfo.o: fsinfo.c
	clang -Wall -g -c fsinfo.c
	
clean:
	rm -rf *o fat32reader