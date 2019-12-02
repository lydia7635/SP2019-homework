#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef struct {
	int id;
	int winning_times;
	int rank;
} Data;

void err_sys(const char *x)
{
	perror(x);
	exit(1);
}

void init_data(Data *data[], int player[], int num)
{
	for(int i = 0; i < num; i++) {
		data[i] = (Data *)malloc(sizeof(Data));
		data[i]->id = player[i];
		data[i]->winning_times = 0;
		data[i]->rank = 0;
	}
	return;
}

void freeData(Data *data[], int num)
{
	for(int i = 0; i < num; i++)
		free(data[i]);
	return;
}

void calcuRank(Data *data[], int num)
{
	int fill = 0;
	int cur_rank = 1;
	while(fill < num) {
		int max = -1;
		for(int i = 0; i < num; i++) {
			if(data[i]->rank == 0 && data[i]->winning_times > max)
				max = data[i]->winning_times;
		}
		for(int i = 0; i < num; i++) {
			if(data[i]->winning_times == max) {
				data[i]->rank = cur_rank;
				fill++;
			}
		}
		cur_rank++;
	}
	return;
}

void flush_sync(FILE *pipe, int pipeFd)
{
	fflush(pipe);
	fsync(pipeFd);
	return;
}

void create2children(pid_t *pid1, pid_t *pid2, int pipe1_down[2], int pipe1_up[2], int pipe2_down[2], int pipe2_up[2],
	char *argv1, char *argv2, int depth, int player1_id, int player2_id)
{
	//create pipe for child 1 (left)
	if(pipe(pipe1_down) < 0)
		err_sys("pipe1_down error");
	if(pipe(pipe1_up) < 0)
		err_sys("pipe1_up error");

	//fork child 1
	if((*pid1 = fork()) < 0)
		err_sys("fork1 error");
	else if(*pid1 == 0) {	//child 1
		close(pipe1_down[1]);
		close(pipe1_up[0]);

		if(dup2(pipe1_down[0], STDIN_FILENO) != STDIN_FILENO)
			err_sys("pipe1_down: dup2 error to stdin");
		if(dup2(pipe1_up[1], STDOUT_FILENO) != STDOUT_FILENO)
			err_sys("pipe1_up: dup2 error to stdout");

		close(pipe1_down[0]);
		close(pipe1_up[1]);

		//exec host or player
		char num_str[8];
		if(depth <= 1) {
			sprintf(num_str, "%d\0", (depth + 1));
			execl("./host", "./host", argv1, argv2, num_str, (char *)0);
		}
		else {
			sprintf(num_str, "%d\0", player1_id);
			execl("./player", "./player", num_str, (char *)0);
		}
	}

	close(pipe1_down[0]);
	close(pipe1_up[1]);
	
	//create pipe for child 2 (right)
	if(pipe(pipe2_down) < 0)
		err_sys("pipe2_down error");
	if(pipe(pipe2_up) < 0)
		err_sys("pipe2_up error");

	//fork child 2
	if((*pid2 = fork()) < 0)
		err_sys("fork2 error");
	else if(*pid2 == 0) {	//child 2
		close(pipe2_down[1]);
		close(pipe2_up[0]);

		if(dup2(pipe2_down[0], STDIN_FILENO) != STDIN_FILENO)
			err_sys("pipe2_down: dup2 error to stdin");
		if(dup2(pipe2_up[1], STDOUT_FILENO) != STDOUT_FILENO)
			err_sys("pipe2_up: dup2 error to stdout");

		close(pipe2_down[0]);
		close(pipe2_up[1]);

		//remember to close pipe1
		close(pipe1_down[1]);
		close(pipe1_up[0]);

		//exec host or player
		char num_str[8];
		if(depth <= 1) {
			sprintf(num_str, "%d\0", (depth + 1));
			execl("./host", "./host", argv1, argv2, num_str, (char *)0);
		}
		else {
			sprintf(num_str, "%d\0", player2_id);
			execl("./player", "./player", num_str, (char *)0);
		}
	}

	close(pipe2_down[0]);
	close(pipe2_up[1]);
	return;
}

int main(int argc, char *argv[])
{
	if (argc != 4)
		err_sys("argc error!");

	int host_id = atoi(argv[1]);
	int random_key = atoi(argv[2]);
	int depth = atoi(argv[3]);

	pid_t pid1, pid2;
	int pipe1_down[2], pipe1_up[2];
	int pipe2_down[2], pipe2_up[2];

	//==================== task: fork() ====================//

	if(depth <= 1)	//only root & child
		create2children(&pid1, &pid2, pipe1_down, pipe1_up, pipe2_down, pipe2_up, argv[1], argv[2], depth, 0, 0);

	//==================== task: prepare data ====================//

	if(depth == 0) {	//root_host
#ifdef DEBUG
		fprintf(stderr, "depth = 0\n");
#endif
		char readFIFO_name[16];

		sprintf(readFIFO_name, "Host%d.FIFO\0", host_id);
		FILE *readFIFO = fopen(readFIFO_name, "r");
		if(readFIFO == NULL)
			err_sys("readFIFO error!");

		FILE *writeFIFO = fopen("Host.FIFO", "w");
		if(writeFIFO == NULL)
			err_sys("writeFIFO error!");

		FILE *write_pipe1 = fdopen(pipe1_down[1], "w");
		FILE *read_pipe1 = fdopen(pipe1_up[0], "r");
		FILE *write_pipe2 = fdopen(pipe2_down[1], "w");
		FILE *read_pipe2 = fdopen(pipe2_up[0], "r");
		if(!(write_pipe1 && write_pipe2 && read_pipe1 && read_pipe2))
			err_sys("depth = 0, fdopen(pipe) error");

		int player[8];
		fscanf(readFIFO, "%d %d %d %d %d %d %d %d", &player[0], &player[1], &player[2], &player[3],
			&player[4],	&player[5], &player[6], &player[7]);

		while(player[0] != -1) {
			Data *data[8];
			init_data(data, player, 8);

			fprintf(write_pipe1, "%d %d %d %d\n", player[0], player[1], player[2], player[3]);
			fprintf(write_pipe2, "%d %d %d %d\n", player[4], player[5], player[6], player[7]);
			flush_sync(write_pipe1, pipe1_down[1]);
			flush_sync(write_pipe2, pipe2_down[1]);

			int player1_id, player2_id, player1_money, player2_money;
			for(int i = 1; i <= 10; i++) {
				fscanf(read_pipe1, "%d %d", &player1_id, &player1_money);
				fscanf(read_pipe2, "%d %d", &player2_id, &player2_money);
				
				int winner_id = (player1_money > player2_money)? player1_id : player2_id;
				for(int i = 0; i < 8; i++) {
					if(data[i]->id == winner_id) {
						(data[i]->winning_times)++;
						break;
					}
				}

				if(i != 10) {
					fprintf(write_pipe1, "%d\n", winner_id);
					fprintf(write_pipe2, "%d\n", winner_id);
					flush_sync(write_pipe1, pipe1_down[1]);
					flush_sync(write_pipe2, pipe2_down[1]);
				}
			}

			calcuRank(data, 8);
			//write to FIFO
			fprintf(writeFIFO, "%s\n", argv[2]);
			for(int i = 0; i < 8; i++) {
				fprintf(writeFIFO, "%d %d\n", data[i]->id, data[i]->rank);
			}
			int writeFIFOfd = fileno(writeFIFO);
			flush_sync(writeFIFO, writeFIFOfd);

			freeData(data, 8);

			//new competition
			fscanf(readFIFO, "%d %d %d %d %d %d %d %d", &player[0],
				&player[1], &player[2], &player[3], &player[4],
				&player[5], &player[6], &player[7]);
		}

		// no work for this host!
		fprintf(write_pipe1, "-1 -1 -1 -1\n");
		fprintf(write_pipe2, "-1 -1 -1 -1\n");
		flush_sync(write_pipe1, pipe1_down[1]);
		flush_sync(write_pipe2, pipe2_down[1]);
		wait(NULL);
		wait(NULL);
		fclose(write_pipe1);
		fclose(write_pipe2);
		fclose(read_pipe1);
		fclose(read_pipe2);
		exit(0);
	}
	else if(depth == 1) {
#ifdef DEBUG
		fprintf(stderr, "depth = 1\n");
#endif
		FILE *write_pipe1 = fdopen(pipe1_down[1], "w");
		FILE *read_pipe1 = fdopen(pipe1_up[0], "r");
		FILE *write_pipe2 = fdopen(pipe2_down[1], "w");
		FILE *read_pipe2 = fdopen(pipe2_up[0], "r");
		if(!(write_pipe1 && write_pipe2 && read_pipe1 && read_pipe2))
			err_sys("depth = 1, fdopen(pipe) error");

		int player[4];
		scanf("%d %d %d %d", &player[0],
			&player[1], &player[2], &player[3]);

		while(player[0] != -1) {
			fprintf(write_pipe1, "%d %d\n", player[0], player[1]);
			fprintf(write_pipe2, "%d %d\n", player[2], player[3]);
			flush_sync(write_pipe1, pipe1_down[1]);
			flush_sync(write_pipe2, pipe2_down[1]);

			int player1_id, player2_id, player1_money, player2_money;
			for(int i = 1; i <= 10; i++) {
				fscanf(read_pipe1, "%d %d", &player1_id, &player1_money);
				fscanf(read_pipe2, "%d %d", &player2_id, &player2_money);
				
				if(player1_money > player2_money)
					printf("%d %d\n", player1_id, player1_money);
				else
					printf("%d %d\n", player2_id, player2_money);
				flush_sync(stdout, STDOUT_FILENO);

				if(i != 10) {
					int winner_id;
					scanf("%d", &winner_id);

					fprintf(write_pipe1, "%d\n", winner_id);
					fprintf(write_pipe2, "%d\n", winner_id);
					flush_sync(write_pipe1, pipe1_down[1]);
					flush_sync(write_pipe2, pipe2_down[1]);
				}
			}

			//new competition
			scanf("%d %d %d %d", &player[0],
				&player[1], &player[2], &player[3]);
		}

		// no work for this host!
		fprintf(write_pipe1, "-1 -1\n");
		fprintf(write_pipe2, "-1 -1\n");
		flush_sync(write_pipe1, pipe1_down[1]);
		flush_sync(write_pipe2, pipe2_down[1]);
		wait(NULL);
		wait(NULL);
		fclose(write_pipe1);
		fclose(write_pipe2);
		fclose(read_pipe1);
		fclose(read_pipe2);
		exit(0);
	}
	else {
#ifdef DEBUG
		fprintf(stderr, "depth = 2\n");
#endif
		int player[2];
		scanf("%d %d", &player[0], &player[1]);

#ifdef DEBUG
		fprintf(stderr, "player[0] = %d, player[1] =  %d ...\n", player[0], player[1]);
#endif

		while(player[0] != -1) {
			//leaf fork()
			create2children(&pid1, &pid2, pipe1_down, pipe1_up, pipe2_down, pipe2_up, argv[1], argv[2],
				depth, player[0], player[1]);
			FILE *write_pipe1 = fdopen(pipe1_down[1], "w");
			FILE *read_pipe1 = fdopen(pipe1_up[0], "r");
			FILE *write_pipe2 = fdopen(pipe2_down[1], "w");
			FILE *read_pipe2 = fdopen(pipe2_up[0], "r");
			if(!write_pipe1)
				err_sys("depth = 2, fdopen(write_pipe1) error");
			if(!(write_pipe1 && write_pipe2 && read_pipe1 && read_pipe2))
				err_sys("depth = 2, fdopen(pipe) error");

			int player1_id, player2_id, player1_money, player2_money;
			for(int i = 1; i <= 10; i++) {
				fscanf(read_pipe1, "%d %d", &player1_id, &player1_money);
				fscanf(read_pipe2, "%d %d", &player2_id, &player2_money);
				
				if(player1_money > player2_money)
					printf("%d %d\n", player1_id, player1_money);
				else
					printf("%d %d\n", player2_id, player2_money);
				flush_sync(stdout, STDOUT_FILENO);

				if(i != 10) {
					int winner_id;
					scanf("%d", &winner_id);

					fprintf(write_pipe1, "%d\n", winner_id);
					fprintf(write_pipe2, "%d\n", winner_id);
					flush_sync(write_pipe1, pipe1_down[1]);
					flush_sync(write_pipe2, pipe2_down[1]);
				}
			}
			wait(NULL);
			wait(NULL);
			fclose(write_pipe1);
			fclose(write_pipe2);
			fclose(read_pipe1);
			fclose(read_pipe2);

			//new competition
			scanf("%d %d", &player[0], &player[1]);
		}
		// no work for this host!
		exit(0);
	}
}
