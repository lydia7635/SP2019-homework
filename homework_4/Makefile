all: hw4.c
	gcc hw4.c -lm -lpthread -O3 -o hw4

debug: hw4.c
	gcc hw4.c -DDEBUG -lm -lpthread -O3 -o hw4

create: createImage.c
	gcc createImage.c -o createImage

comparing: compare.c
	gcc compare.c -o compare

clean:
	rm -f hw4 createImage compare