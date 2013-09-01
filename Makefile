balls: balls.c Makefile
	gcc -c hid.c
	g++ -c pscryptor/PBKeyDerive.cpp -o pscryptor/PBKeyDerive.o
	g++ -c pscryptor/PSCryptor.cpp -o pscryptor/PSCryptor.o
	g++ -c balls.c
	g++ balls.o hid.o pscryptor/PSCryptor.o pscryptor/PBKeyDerive.o -framework IOKit -framework CoreFoundation -o balls
