balls: balls.c hid.c hidapi.h Makefile
	gcc -c balls.c hid.c
	gcc balls.o hid.o -framework IOKit -framework CoreFoundation -o balls
