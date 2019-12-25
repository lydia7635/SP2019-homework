#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<unistd.h>
#include<setjmp.h>
#include<sys/types.h>
#include<signal.h>
#include"scheduler.h"

#define SIGUSR3 SIGWINCH

jmp_buf SCHEDULER;
jmp_buf MAIN;

FCB_ptr Current, Head;
int mutex;
int idx = 0;
char arr[10000];

int P, Q, task, runTime;
sigset_t newmask, oldmask, pendmask;
sigset_t unmask1, unmask2, unmask3;

bool queue[5];

void err_sys(const char *x)
{
	perror(x);
	exit(1);
}

static void sigHandler(int signo)
{
	if(sigprocmask(SIG_BLOCK, &newmask, NULL) < 0)
		err_sys("SIG_BLOCK error");

	FCB_ptr realCur = Current;

	if(signo == SIGUSR1) {
		printf("receive1\n");
		fflush(stdout);
	}
	else if(signo == SIGUSR2) {
		printf("receive2\n");
		fflush(stdout);
	}
	else if(signo == SIGUSR3) {

		for(int i = 1; i <= 4; i++) {
			if(queue[i] == 1) {
				printf("%d", i);
			}
		}
		printf("\n");
		fflush(stdout);
		Current = Current->Previous;
	}
	else
		err_sys("received wrong signal");

	if(setjmp(realCur->Environment) == 0)
		longjmp(SCHEDULER, 1);

	return;
}

void make_circular_linked_list(int name)
{
	if(Current == NULL) {
		Current = (FCB_ptr)malloc(sizeof(FCB));
		Current->Previous = Current;
		Head = Current;
	}
	else {
		Current->Next = (FCB_ptr)malloc(sizeof(FCB));
		Current->Next->Previous = Current;
		Current = Current->Next;
	}
	Current->Name = name;
	Current->Next = Current;
	return;
}

bool getLock(int name)
{
	if(mutex == 0 || mutex == name) {
		mutex = name;
		return true;
	}
	else
		return false;	
}

void releaseLock()
{
	mutex = 0;
	return;
}

void funct_x(int name)
{
	int i, j;

	make_circular_linked_list(name);
	if(setjmp(Current->Environment) == 0) {
		if(name == 4)
			longjmp(MAIN, 1);
		else
			funct_5(name + 1);
	}

	for(j = 1; j <= P + 1; j++) {
		if(sigprocmask(SIG_BLOCK, &newmask, NULL) < 0)
			err_sys("SIG_BLOCK error");

		while(task != 1 && !getLock(name)) {
			queue[name] = true;
			if(setjmp(Current->Environment) == 0)
				longjmp(SCHEDULER, 1);
		}
		queue[name] = false;

		//we just want to get the lock to continue...
		if(j == P + 1)
			break;

		for(i = 1; i <= Q; i++) {
			sleep(1);
			arr[idx++] = '0' + name;
#ifdef DEBUG
			fprintf(stderr, "j = %d, i = %d, arr = %s\n", j, i, arr);
#endif
		}

		if(task == 2 && j % runTime == 0) {
			releaseLock();
			if(setjmp(Current->Environment) == 0)
				longjmp(SCHEDULER, 1);
			else
				getLock(name);
		}
		if(task == 3) {
			sigemptyset(&pendmask);
			if(sigpending(&pendmask) < 0)
				err_sys("sigpending error");
			if(sigismember(&pendmask, SIGUSR1)) {
				sigprocmask(SIG_UNBLOCK, &unmask1, NULL);
			}
			if(sigismember(&pendmask, SIGUSR2)) {
				releaseLock();
				sigprocmask(SIG_UNBLOCK, &unmask2, NULL);
			}
			if(sigismember(&pendmask, SIGUSR3)) {
				sigprocmask(SIG_UNBLOCK, &unmask3, NULL);
			}
		}

	}
	releaseLock();

	longjmp(SCHEDULER, -2);
}

void funct_1(int name)
{
	funct_x(1);
	return;
}

void funct_2(int name)
{
	funct_x(2);
	return;
}

void funct_3(int name)
{
	funct_x(3);
	return;
}

void funct_4(int name)
{
	funct_x(4);
	return;
}

void funct_5(int name)
{
	int a[10000]; // This line must not be changed.
	// call other functions
	switch(name) {
	case 1:
		funct_1(1);
		break;
	case 2:
		funct_2(2);
		break;
	case 3:
		funct_3(3);
		break;
	case 4:
		funct_4(4);
		break;
	default:
		break;
	}
}

int main(int argc, char *argv[])
{
	P = atoi(argv[1]);
	Q = atoi(argv[2]);
	task = atoi(argv[3]);
	runTime = atoi(argv[4]);
	Current = NULL;
	mutex = 0;

	if(signal(SIGUSR1, sigHandler) == SIG_ERR)
		err_sys("catch SIGUSR1 error");
	if(signal(SIGUSR2, sigHandler) == SIG_ERR)
		err_sys("catch SIGUSR2 error");
	if(signal(SIGUSR3, sigHandler) == SIG_ERR)
		err_sys("catch SIGUSR3 error");

	sigemptyset(&newmask);
	sigaddset(&newmask, SIGUSR1);
	sigaddset(&newmask, SIGUSR2);
	sigaddset(&newmask, SIGUSR3);

	sigemptyset(&unmask1);
	sigemptyset(&unmask2);
	sigemptyset(&unmask3);
	sigaddset(&unmask1, SIGUSR1);
	sigaddset(&unmask2, SIGUSR2);
	sigaddset(&unmask3, SIGUSR3);

	sigemptyset(&pendmask);

	if(sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0)
		err_sys("SIG_BLOCK error");

	if(setjmp(MAIN) == 0)
		funct_5(1);

	Current->Next = Head;
	Head->Previous = Current;
	for(int i = 1; i <= 4; i++)
		queue[i] = false;

	Scheduler();
}