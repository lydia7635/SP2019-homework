#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<linux/fcntl.h>
#include<string.h>

typedef struct{
	int id;
	int balance;
} Account;

int main()
{
	int file_fd;
	file_fd = open("account_list", O_RDWR);
	if(file_fd == -1)
		fprintf(stderr, "fail to open file");
	lseek(file_fd, 0, SEEK_SET);
	for(int i = 0; i < 20; i++) {
		Account account;
		read(file_fd, &account, sizeof(Account));
		fprintf(stderr, "%d %d\n", account.id, account.balance);
	}
	fprintf(stderr, "....file ends\n");
	close(file_fd);
	return 0;
}
