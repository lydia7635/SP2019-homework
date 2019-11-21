#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<linux/fcntl.h>
#include<string.h>

#define default_balance 3000
typedef struct{
	int id;
	int balance;
} Account;

int main()
{
	int file_fd;
	file_fd = open("account_list", O_RDWR | O_CREAT, 0644);
	if(file_fd == -1)
		fprintf(stderr, "fail to open file");
	lseek(file_fd, 0, SEEK_SET);
	for(int i = 1; i <= 20; i++) {
		Account account;
		account.id = i;
		account.balance = default_balance;
		write(file_fd, &account, sizeof(Account));
	}
	fprintf(stderr, "initialize account\n");
	close(file_fd);
	return 0;
}
