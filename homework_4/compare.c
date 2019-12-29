#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>

int main(int argc, char *argv[])
{
	int realY = open("y_test", O_RDONLY);
	int predY = open("result.csv", O_RDONLY);
	FILE *predYfp = fdopen(predY, "r");

	int num = atoi(argv[1]);
	int correct = 0;
	int times[10] = {};

	fscanf(predYfp, "id,label");
	for(int i = 0; i < num; i++) {
		char real;
		int id, pred;
		read(realY, &real, sizeof(char));
		fscanf(predYfp, "%d,%d", &id, &pred);
		times[pred]++;
		if(pred == (int)real)
			correct++;
	}
	printf("correctness = %f\n", (double)correct / num);
	for(int i = 0; i < 10; i++)
		printf("%d: %d\n", i, times[i]);
	return 0;
}