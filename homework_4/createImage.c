#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/stat.h>
#include<fcntl.h>
#define threshold 128
#define word 784

int main(int argc, char *argv[])
{
	int target = (argc == 2)? atoi(argv[1]) - 1 : 0;
	unsigned char bright;

	int trainY = open("y_train", O_RDONLY);
	lseek(trainY, target, SEEK_SET);
	read(trainY, &bright, sizeof(unsigned char));
	printf("label: %d\n", (int)bright);
	close(trainY);

	int trainX = open("X_train", O_RDONLY); 
	lseek(trainX, word * target, SEEK_SET);
	for(int i = 0; i < 28; i++) {
		for(int j = 0; j < 28; j++) {
			read(trainX, &bright, sizeof(unsigned char));
			if(bright < threshold)
				printf(" ");
			else
				printf("#");
		}
		printf("\n");
	}
	
	close(trainX);
}