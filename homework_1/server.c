#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include<stdbool.h>

#define ERR_EXIT(a) { perror(a); exit(1); }

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    // you don't need to change this.
	int item;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

typedef struct{
    int id;
    int balance;
} Account;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";

// Forwards

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

static int handle_read(request* reqP);
// return 0: socket ended, request done.
// return 1: success, message (without header) got this time is in reqP->buf with reqP->buf_len bytes. read more until got <= 0.
// It's guaranteed that the header would be correctly set after the first read.
// error code:
// -1: client connection error

bool save(int file_fd, int id, int money);
bool withdraw(int file_fd, int id, int money);
bool transfer(int file_fd, int id, int another, int money);
bool Balance(int file_fd, int id, int money);

int main(int argc, char** argv) {
    int i, ret;

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd;  // fd for file that we open for reading
    char buf[512];
    int buf_len;

    struct flock lock;

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // Get file descripter table size and initize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    //open account_list
    file_fd = open("account_list", O_RDWR);
    if(file_fd < 0)
        fprintf(stderr, "no file names account_list\n");

    //set read lock and write lock in same process
    bool readLock[21] = {false}, writeLock[21] = {false};

    //init fd set table
    struct timeval timeout;
    fd_set original_set, working_set;

    FD_ZERO(&original_set);
    FD_SET(svr.listen_fd, &original_set);

    //set the listener "svr.listen_fd" to be nonblocking
    fcntl(svr.listen_fd, F_SETFL, O_NONBLOCK);

    while (1) {
        // TODO: Add IO multiplexing
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;

        memcpy(&working_set, &original_set, sizeof(original_set));
        if(select(maxfd, &working_set, NULL, NULL, &timeout) <= 0)
            continue;
        
        if(FD_ISSET(svr.listen_fd, &working_set)) {
            // Check new connection
            clilen = sizeof(cliaddr);
            conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
            if (conn_fd < 0) {
                if (errno == EINTR || errno == EAGAIN) continue;  // try again
                if (errno == ENFILE) {
                    (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                    continue;
                }
                ERR_EXIT("accept")
            }

            FD_SET(conn_fd, &original_set);
            continue;
        }
        else {
            conn_fd = -1;
            for(int i = 3; i < maxfd; i++) {
                if(i != svr.listen_fd && FD_ISSET(i, &working_set)) {
                    conn_fd = i;
                    break;
                } 
            }
            if(conn_fd == -1)
                continue;
        }
        
        requestP[conn_fd].conn_fd = conn_fd;
        strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
        fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);

		ret = handle_read(&requestP[conn_fd]); // parse data from client to requestP[conn_fd].buf
		if (ret < 0) {
			fprintf(stderr, "bad request from %s\n", requestP[conn_fd].host);
			continue;
		}

#ifdef READ_SERVER
        Account account;
		requestP[conn_fd].item = atoi(requestP[conn_fd].buf);

        //set lockptr
        lock.l_type = F_RDLCK;
        lock.l_start = sizeof(Account) * (requestP[conn_fd].item - 1);
        lock.l_whence = SEEK_SET;
        lock.l_len = sizeof(Account);

        if(!writeLock[requestP[conn_fd].item] && fcntl(file_fd, F_SETLK, &lock) != -1){
            //no lock, we set read lock
            readLock[requestP[conn_fd].item] = true;

            lseek(file_fd, sizeof(Account) * (requestP[conn_fd].item - 1), SEEK_SET);
            read(file_fd, &account, sizeof(Account));
            sprintf(buf, "%d %d\n", account.id, account.balance);
            write(requestP[conn_fd].conn_fd, buf, strlen(buf));
            
            //unset read lock
            lock.l_type = F_UNLCK;
            lock.l_start = sizeof(Account) * (requestP[conn_fd].item - 1);
            lock.l_whence = SEEK_SET;
            lock.l_len = sizeof(Account);
            fcntl(file_fd, F_SETLK, &lock);
            readLock[requestP[conn_fd].item] = false;
        }
        else {
            //it is locked
            sprintf(buf, "This account is locked.\n");
            write(requestP[conn_fd].conn_fd, buf, strlen(buf));
        }
        
#else
        if(requestP[conn_fd].item == 0) {
            //we didn't get account id
            requestP[conn_fd].item = atoi(requestP[conn_fd].buf);

            //set lockptr
            lock.l_type = F_WRLCK;
            lock.l_start = sizeof(Account) * (requestP[conn_fd].item - 1);
            lock.l_whence = SEEK_SET;
            lock.l_len = sizeof(Account);
            lock.l_pid = getpid();

            if(!writeLock[requestP[conn_fd].item] && !readLock[requestP[conn_fd].item]
                && fcntl(file_fd, F_SETLK, &lock) != -1) {
                //no lock, we set write lock
                writeLock[requestP[conn_fd].item] = true;

                sprintf(buf, "This account is modifiable.\n");
                write(requestP[conn_fd].conn_fd, buf, strlen(buf));
                continue;
            }
            else {
                //it is locked
                sprintf(buf, "This account is locked.\n");
                write(requestP[conn_fd].conn_fd, buf, strlen(buf));
            }
        }
        else {
            //do operations
            int money, another;
            switch(requestP[conn_fd].buf[0]) {
            case 's': //save money
                sscanf(requestP[conn_fd].buf, "save %d", &money);
                if(!save(file_fd, requestP[conn_fd].item, money)) {
                    sprintf(buf, "Operation failed.\n");
                    write(requestP[conn_fd].conn_fd, buf, strlen(buf));
                }
                break;
            case 'w': //withdraw money
                sscanf(requestP[conn_fd].buf, "withdraw %d", &money);
                if(!withdraw(file_fd, requestP[conn_fd].item, money)) {
                    sprintf(buf, "Operation failed.\n");
                    write(requestP[conn_fd].conn_fd, buf, strlen(buf));
                }
                break;
            case 't': //transfer another money
                sscanf(requestP[conn_fd].buf, "transfer %d %d", &another, &money);
                if(!transfer(file_fd, requestP[conn_fd].item, another, money)) {
                    sprintf(buf, "Operation failed.\n");
                    write(requestP[conn_fd].conn_fd, buf, strlen(buf));
                }
                break;
            case 'b': //balance money
                sscanf(requestP[conn_fd].buf, "balance %d", &money);
                if(!Balance(file_fd, requestP[conn_fd].item, money)) {
                    sprintf(buf, "Operation failed.\n");
                    write(requestP[conn_fd].conn_fd, buf, strlen(buf));
                }
                break;
            default:
                sprintf(buf, "Operation failed.\n");
                write(requestP[conn_fd].conn_fd, buf, strlen(buf));
                break;
            }

            //unset write lock
            lock.l_type = F_UNLCK;
            lock.l_start = sizeof(Account) * (requestP[conn_fd].item - 1);
            lock.l_whence = SEEK_SET;
            lock.l_len = sizeof(Account);
            fcntl(file_fd, F_SETLK, &lock);
            writeLock[requestP[conn_fd].item] = false;
        }
#endif
		close(requestP[conn_fd].conn_fd);
		free_request(&requestP[conn_fd]);
        FD_CLR(conn_fd, &original_set);
    }
    free(requestP);
    return 0;
}

bool save(int file_fd, int id, int money)
{
    if(money < 0)
        return false;

    Account account;
    lseek(file_fd, sizeof(Account) * (id - 1), SEEK_SET);
    read(file_fd, &account, sizeof(Account));
    account.balance += money;
    lseek(file_fd, sizeof(Account) * (id - 1), SEEK_SET);
    write(file_fd, &account, sizeof(Account));
    return true;
}

bool withdraw(int file_fd, int id, int money)
{
    Account account;
    lseek(file_fd, sizeof(Account) * (id - 1), SEEK_SET);
    read(file_fd, &account, sizeof(Account));
    if(money < 0 || money > account.balance)
        return false;

    account.balance -= money;
    lseek(file_fd, sizeof(Account) * (id - 1), SEEK_SET);
    write(file_fd, &account, sizeof(Account));
    return true;
}

bool transfer(int file_fd, int id, int B, int money)
{
    Account account;
    lseek(file_fd, sizeof(Account) * (id - 1), SEEK_SET);
    read(file_fd, &account, sizeof(Account));
    if(money < 0 || money > account.balance)
        return false;
    
    account.balance -= money;
    lseek(file_fd, sizeof(Account) * (id - 1), SEEK_SET);
    write(file_fd, &account, sizeof(Account));

    read(file_fd, &account, sizeof(Account));
    account.balance += money;
    lseek(file_fd, sizeof(Account) * (B - 1), SEEK_SET);
    write(file_fd, &account, sizeof(Account));
    return true;
}

bool Balance(int file_fd, int id, int money)
{
    if(money < 0)
        return false;

    Account account;
    lseek(file_fd, sizeof(Account) * (id - 1), SEEK_SET);
    read(file_fd, &account, sizeof(Account));
    account.balance = money;
    lseek(file_fd, sizeof(Account) * (id - 1), SEEK_SET);
    write(file_fd, &account, sizeof(Account));
    return true;
}


// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void* e_malloc(size_t size);


static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->item = 0;
    reqP->wait_for_write = 0;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

// return 0: socket ended, request done.
// return 1: success, message (without header) got this time is in reqP->buf with reqP->buf_len bytes. read more until got <= 0.
// It's guaranteed that the header would be correctly set after the first read.
// error code:
// -1: client connection error
static int handle_read(request* reqP) {
    int r;
    char buf[512];

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
	char* p1 = strstr(buf, "\015\012");
	int newline_len = 2;
	// be careful that in Windows, line ends with \015\012
	if (p1 == NULL) {
		p1 = strstr(buf, "\012");
		newline_len = 1;
		if (p1 == NULL) {
			ERR_EXIT("this really should not happen...");
		}
	}
	size_t len = p1 - buf + 1;
	memmove(reqP->buf, buf, len);
	reqP->buf[len - 1] = '\0';
	reqP->buf_len = len-1;
    return 1;
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }
}

static void* e_malloc(size_t size) {
    void* ptr;

    ptr = malloc(size);
    if (ptr == NULL) ERR_EXIT("out of memory");
    return ptr;
}

