#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/types.h>
#include<sys/times.h>

#define trainDataNum 60000
#define testDataNum 10000
#define size 784 //28 * 28

#define iteration 25
#define lr 0.0004

double trainX[trainDataNum][size];
double trainY[trainDataNum][10];
double trainY_hat[trainDataNum][10];

double testX[testDataNum][size];
double testY_hat[testDataNum][size];

double W[10][size];
double W_grad[10][size];

double trainZ[trainDataNum][10];
double testZ[testDataNum][10];

int place_type[10000][3];

unsigned char val_trainX[trainDataNum][size];
unsigned char val_testX[testDataNum][size];
unsigned char val_trainY[trainDataNum];


void err_sys(const char *x)
{
	perror(x);
	exit(1);
}

void readX(int fdX, int dataNum, int type)
{
	if(type == 0) {
		read(fdX, &val_trainX, sizeof(unsigned char) * size * dataNum);
		for(int i = 0; i < dataNum; ++i) {
			for(int j = 0; j < size; ++j)
				trainX[i][j] = ((double)val_trainX[i][j]) / (double)255;
		}
	}
	else {
		read(fdX, &val_testX, sizeof(unsigned char) * size * dataNum);
		for(int i = 0; i < dataNum; ++i) {
			for(int j = 0; j < size; ++j) {
				testX[i][j] = ((double)val_testX[i][j]) / (double)255;
			}
		}
	}
	
	return;
}

void readY(int fdY, int dataNum, int type)	//only used by training data
{
	unsigned char val;
	read(fdY, &val_trainY, sizeof(unsigned char) * dataNum);
	for(int i = 0; i < dataNum; ++i) {
		for(int j = 0; j < 10; ++j) {
			if(j == val_trainY[i])
				trainY[i][j] = 1;
			else
				trainY[i][j] = 0;
		}
	}
	return;
}

void *multiXW(void *vptr)
{
	int *arr = (int *)vptr;
	int head = arr[0];
	int tail = arr[1];
	int type = arr[2];
	if(type == 0) {
		for(int i = head; i <= tail; ++i) {
			for(int j = 0; j < 10; ++j) {
				trainZ[i][j] = 0;
				for(int k = 0; k < size; ++k)
					trainZ[i][j] += trainX[i][k] * W[j][k];
			}
		}
	}
	else {
		for(int i = head; i <= tail; ++i) {
			for(int j = 0; j < 10; ++j) {
				testZ[i][j] = 0;
				for(int k = 0; k < size; ++k)
					testZ[i][j] += testX[i][k] * W[j][k];
			}
		}
	}
	pthread_exit(NULL);
}

void softmax(int dataNum, int type)
{
	if(type == 0) {
		for(int i = 0; i < dataNum; ++i) {
			double e[10];
			double totale = 0;

			for(int j = 0; j < 10; ++j) {
				e[j] = exp(trainZ[i][j]);
				totale += e[j];
			}
			for(int j = 0; j < 10; ++j)
					trainY_hat[i][j] = e[j] / totale;
		}
	}
	else {
		for(int i = 0; i < dataNum; ++i) {
			double e[10];
			double totale = 0;

			for(int j = 0; j < 10; ++j) {
				e[j] = exp(testZ[i][j]);
				totale += e[j];
			}
			for(int j = 0; j < 10; ++j)
				testY_hat[i][j] = e[j] / totale;
		}
	}
	
	return;
}

void multiXY(int dataNum)
{
	for(int i = 0; i < dataNum; ++i) {
		for(int j = 0; j < 10; ++j) {
				trainY_hat[i][j] = trainY_hat[i][j] - trainY[i][j];
		}
	}

	for(int i = 0; i < 10; ++i)
		for(int j = 0; j < size; ++j)
			W_grad[i][j] = 0;

	for(int k = 0; k < dataNum; ++k) {
		for(int i = 0; i < 10; ++i) {
			for(int j = 0; j < size; ++j)
				W_grad[i][j] += trainX[k][j] * trainY_hat[k][i];
		}
	}

	return;
}

void refreshW()
{
	for(int i = 0; i < 10; ++i)
		for(int j = 0; j < size; ++j)
			W[i][j] = W[i][j] - lr * W_grad[i][j];
	return;
}

#ifdef DEBUG
void pr_times(clock_t real, struct tms *tmsStart, struct tms *tmsEnd)
{
	static long clktck = 0;
	if(clktck == 0)
		if((clktck = sysconf(_SC_CLK_TCK)) < 0)
			err_sys("sysconf error");
	fprintf(stderr, "real: %5.2f ", real / (double)clktck);
	fprintf(stderr, "user: %5.2f ", (tmsEnd->tms_utime - tmsStart->tms_utime) / (double)clktck);
	fprintf(stderr, "sys:  %5.2f\n", (tmsEnd->tms_stime - tmsStart->tms_stime) / (double)clktck);

}
#endif



int main(int argc, char *argv[])
{
#ifdef DEBUG
	struct tms tmsStart;
	struct tms tmsEnd;
	clock_t start, end;
	if((start = times(&tmsStart)) == -1)
		err_sys("time error");
#endif
	int threadNum = atoi(argv[5]);
	int trainX = open(argv[1], O_RDONLY);
	int trainY = open(argv[2], O_RDONLY);

	lseek(trainX, 0, SEEK_SET);
	lseek(trainY, 0, SEEK_SET);

	readX(trainX, trainDataNum, 0);
	readY(trainY, trainDataNum, 0);

	pthread_t tid[threadNum];
	int perTask = trainDataNum / threadNum;

#ifdef DEBUG
	if((end = times(&tmsEnd)) == -1)
		err_sys("times error");
	pr_times(end - start, &tmsStart, &tmsEnd);
#endif

	for(int i = 0; i < iteration; ++i) {
		for(int j = 0; j < threadNum; ++j) {
			place_type[j][0] = perTask * j;
			place_type[j][1] = perTask * (j + 1) - 1;
			place_type[j][2] = 0;
			if(pthread_create(&tid[j], NULL, multiXW, (void *)place_type[j]) != 0)
				err_sys("create thread error");
		}
		for(int j = 0; j < threadNum; ++j) {
			pthread_join(tid[j], NULL);
		}
		softmax(trainDataNum, 0);
		multiXY(trainDataNum);
		refreshW();
#ifdef DEBUG
		fprintf(stderr, "iteration %d\n", i);
		if((end = times(&tmsEnd)) == -1)
			err_sys("times error");
		pr_times(end - start, &tmsStart, &tmsEnd);
#endif
	}
	close(trainX);
	close(trainY);

	//test data
	int testX = open(argv[3], O_RDONLY);
	lseek(testX, 0, SEEK_SET);

	int predY = open("result.csv", O_WRONLY | O_TRUNC | O_CREAT, 0644);
	FILE *predYfp = fdopen(predY, "w");
	fseek(predYfp, 0, SEEK_SET);

	readX(testX, testDataNum, 1);
	
	perTask = testDataNum / threadNum;
	for(int j = 0; j < threadNum; ++j) {
		place_type[j][0] = perTask * j;
		place_type[j][1] = perTask * (j + 1) - 1;
		place_type[j][2] = 1;
		pthread_create(&tid[j], NULL, multiXW, (void *)place_type[j]);
	}
	for(int j = 0; j < threadNum; ++j)
		pthread_join(tid[j], NULL);
	softmax(testDataNum, 1);

	fprintf(predYfp, "id,label\n");
	for(int i = 0; i < testDataNum; ++i) {
		int ans = 0;
		for(int j = 1; j < 10; ++j) {
			if(testY_hat[i][j] > testY_hat[i][ans])
				ans = j;
		}
		fprintf(predYfp, "%d,%d\n", i, ans);
	}
#ifdef DEBUG
	if((end = times(&tmsEnd)) == -1)
		err_sys("times error");
	pr_times(end - start, &tmsStart, &tmsEnd);
#endif
	close(testX);
	fclose(predYfp);
	return 0;
}