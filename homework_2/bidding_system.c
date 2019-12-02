#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<unistd.h>
#include<fcntl.h>
#include<limits.h>


#define maxC 4000
int combination[maxC][8];

void err_sys(const char *x)
{
	perror(x);
	exit(1);
}

void flush_sync(FILE *pipe, int pipeFd)
{
	fflush(pipe);
	fsync(pipeFd);
	return;
}

void find_comb(int player_num, int index, int element, int one_comb[8], int *comb_num)
{
	if (index == 8) {
		for(int i = 0; i < 8; i++)
			combination[*comb_num][i] = one_comb[i] + 1;
		(*comb_num)++;
		return;
	}
	if (element >= player_num)
		return;

	one_comb[index] = element;
	find_comb(player_num, index + 1, element + 1, one_comb, comb_num);
	find_comb(player_num, index, element + 1, one_comb, comb_num);
	return;
}

int create_combination(int player_num)
{
	int comb_num = 0;
	int one_comb[8];
	find_comb(player_num, 0, 0, one_comb, &comb_num);
	return comb_num;
}

void calcuRank(int rank[], int score[], int player_num)
{
	for(int i = 0; i < player_num; i++)
		rank[i] = 0;

	int fill = 0;
	int cur_rank = 1;
	while(fill < player_num) {
		int max = -1;
		for(int i = 0; i < player_num; i++) {
			if(rank[i] == 0 && score[i] > max)
				max = score[i];
		}
		for(int i = 0; i < player_num; i++) {
			if(score[i] == max) {
				rank[i] = cur_rank;
				fill++;
			}
		}
		cur_rank++;
	}
	return;
}

int main(int argc, char *argv[])
{
	if(argc < 3)
		err_sys("too few argument");

	int host_num = atoi(argv[1]);
	int player_num = atoi(argv[2]);
	int FIFOfd[host_num + 1];
	FILE *FIFO[host_num + 1];
	char buf[PIPE_BUF];

	//==================== task: make FIFO and fork ====================//

	if(mkfifo("./Host.FIFO\0", 0600) < 0)
		err_sys("mkfifo Host.FIFO error");
	if((FIFOfd[0] = open("./Host.FIFO\0", O_RDONLY | O_NONBLOCK)) < 0)
		err_sys("open Host.FIFO error");
	if((FIFO[0] = fdopen(FIFOfd[0], "r")) == NULL)
		err_sys("fdopen Host.FIFO error");


	int child_pid[host_num];
	for(int i = 1; i <= host_num; i++) {
		sprintf(buf, "./Host%d.FIFO\0", i);
		if(mkfifo(buf, 0600) < 0) {
			sprintf(buf, "mkfifo Host%d.FIFO error\0", i);
			err_sys(buf);
		}

		if((child_pid[i - 1] = fork()) < 0)
			err_sys("fork error");
		else if(child_pid[i - 1] == 0) {
			sprintf(buf, "%d\0", i);
			execl("./host", "./host", buf, buf, "0\0", (char *)0);
			//random_key starts from 1
		}

		sprintf(buf, "./Host%d.FIFO\0", i);
		if((FIFOfd[i] = open(buf, O_WRONLY)) < 0) {
			sprintf(buf, "open Host%d.FIFO error\0", i);
			err_sys(buf);
		}
		if((FIFO[i] = fdopen(FIFOfd[i], "w")) == NULL) {
			sprintf(buf, "fdopen Host%d.FIFO error\0", i);
			err_sys(buf);
		}
	}

	//==================== task: unlink FIFOs ====================//
	
	unlink("./Host.FIFO\0");
	for(int i = 1; i <= host_num; i++) {
		sprintf(buf, "./Host%d.FIFO\0", i);
		unlink(buf);
	}

	//==================== task: make combination ====================//

	int comb_num = create_combination(player_num);

	//==================== task: prepare data ====================//

	int score[player_num];
	for(int i = 0; i < player_num; i++)
		score[i] = 0;

	int cur_comb = 0;
	int finish = 0;

	//set FIFOfd[0] to be blocking mode, or we read nothing from it
	int flags = fcntl(FIFOfd[0], F_GETFL, 0);
	flags &= ~O_NONBLOCK;
	fcntl(FIFOfd[0], F_SETFL, flags);

	for(int i = 1; i <= host_num; i++) {
		if(cur_comb < comb_num) {
			fprintf(FIFO[i], "%d %d %d %d %d %d %d %d\n", combination[cur_comb][0],
				combination[cur_comb][1], combination[cur_comb][2], combination[cur_comb][3],
				combination[cur_comb][4], combination[cur_comb][5], combination[cur_comb][6],
				combination[cur_comb][7]);
			flush_sync(FIFO[i], FIFOfd[i]);
			cur_comb++;
		}
		else {
			fprintf(FIFO[i], "-1 -1 -1 -1 -1 -1 -1 -1\n");
			flush_sync(FIFO[i], FIFOfd[i]);
			wait(NULL);
			fclose(FIFO[i]);
		}
	}
	while(finish < comb_num) {
		int random_key;
		fscanf(FIFO[0], "%d", &random_key);

		for(int i = 0; i < 8; i++) {
			int player_id, player_rank;	//note that player_id starts from 1
			fscanf(FIFO[0], "%d %d", &player_id, &player_rank);
			score[player_id - 1] += (8 - player_rank);
		}
		finish++;

		if(cur_comb < comb_num) {
			fprintf(FIFO[random_key], "%d %d %d %d %d %d %d %d\n", combination[cur_comb][0],
				combination[cur_comb][1], combination[cur_comb][2], combination[cur_comb][3],
				combination[cur_comb][4], combination[cur_comb][5], combination[cur_comb][6],
				combination[cur_comb][7]);
			flush_sync(FIFO[random_key], FIFOfd[random_key]);
			cur_comb++;
		}
		else {
			fprintf(FIFO[random_key], "-1 -1 -1 -1 -1 -1 -1 -1\n");
			flush_sync(FIFO[random_key], FIFOfd[random_key]);
			wait(NULL);
			fclose(FIFO[random_key]);
		}
	}

	//==================== task: calulate rank and print ====================//

	int rank[player_num];
	calcuRank(rank, score, player_num);
	for(int i = 0; i < player_num; i++) {
		printf("%d %d\n", i + 1, rank[i]);
		flush_sync(stdout, STDOUT_FILENO);
	}

	fclose(FIFO[0]);

	exit(0);
}