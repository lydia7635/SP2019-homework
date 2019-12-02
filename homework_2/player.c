#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<limits.h>

void flush_sync(FILE *pipe, int pipeFd)
{
	fflush(pipe);
	fsync(pipeFd);
	return;
}

int main(int argc, char** argv)
{
	int n;
	char buf[PIPE_BUF];

	int player_id = atoi(argv[1]);
	int money = player_id * 100;
	int winner_id;

	for(int i = 0; i < 10; i++) {
		printf("%d %d\n", player_id, money);
		flush_sync(stdout, STDOUT_FILENO);

#ifdef DEBUG
		sprintf(buf, "round %2d: %d %d\n", i + 1, player_id, money);
		write(STDERR_FILENO, buf, strlen(buf));
#endif
		if(i != 9)
			scanf("%d", &winner_id);
	}
	exit(0);
}