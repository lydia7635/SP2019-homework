#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<signal.h>

#define SIGUSR3 SIGWINCH

void err_sys(const char *x)
{
	perror(x);
	exit(1);
}

void createChild(pid_t *cpid, int pipeFd[2], int P, int Q)
{
	if((*cpid = fork()) < 0)
		err_sys("fork error");
	else if(*cpid == 0) {
		close(pipeFd[0]);

		if(dup2(pipeFd[1], STDOUT_FILENO) != STDOUT_FILENO)
			err_sys("dup2 error to stdout");

		close(pipeFd[1]);

		char Pstr[4], Qstr[4];
		sprintf(Pstr, "%d\0", P);
		sprintf(Qstr, "%d\0", Q);
		execl("./hw3", "./hw3", Pstr, Qstr, "3\0", "0\0", (char *)0);
	}

	close(pipeFd[1]);
	return;
}

int main()
{
	int P, Q, R;
	scanf("%d%d%d", &P, &Q, &R);
	int sig[R];
	for(int i = 0; i < R; i++)
		scanf("%d", &sig[i]);

	//create pipe
	int pipeFd[2];
	if(pipe(pipeFd) < 0)
		err_sys("pipe error");

	//fork and exec
	pid_t cpid;

	createChild(&cpid, pipeFd, P, Q);
	FILE *pipeStm = fdopen(pipeFd[0], "r");

	//send signal
	for(int i = 0; i < R; i++) {
		sleep(5);

		char ACK[16];
		switch(sig[i]) {
		case 1:
			kill(cpid, SIGUSR1);
			fscanf(pipeStm, "%s", ACK);
#ifdef DEBUG
			fprintf(stderr, "%s\n", ACK);
#endif
			break;
		case 2:
			kill(cpid, SIGUSR2);
			fscanf(pipeStm, "%s", ACK);
#ifdef DEBUG
			fprintf(stderr, "%s\n", ACK);
#endif
			break;
		case 3:
			kill(cpid, SIGUSR3);
			fscanf(pipeStm, "%s", ACK);
			int len = strlen(ACK);
			for(int i = 0; i < len; i++)
				printf("%c%c", ACK[i], (i == len - 1)? '\n' : ' ');
			break;
		default:
			break;
		}
	}
	char arr[256];
	fscanf(pipeStm, "%s", arr);
	printf("%s\n", arr);
	wait(NULL);
	fclose(pipeStm);
	return 0;
}