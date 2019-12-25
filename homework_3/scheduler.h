/* IMPORTANT: 	THE NAMES OF THE VARIABLES ARE FIXED. SO, YOU MUST KNOW THE MEANING OF EACH VARIABLE BEFORE WRITING THE CODE */
/* idx is the index of arr. You should use arr[idx++] = '1'  to append characters*/
extern int idx;
extern char arr[10000];
/*Scheduler's jmp_buf, you must use setjmp(SCHEDULER) to jump to the Scheduler function*/
extern jmp_buf SCHEDULER;

/*The name of each function, funct_5 is the dummy function in the spec.*/
void funct_1();
void funct_2();
void funct_3();
void funct_4();
void funct_5(int name);

/*FCB_NODE is a function control block. FCB_ptr is a pointer which points to a FCB_NODE */
typedef struct FCB_NODE *FCB_ptr;
typedef struct FCB_NODE
{
	jmp_buf Environment;
	int Name;
	FCB_ptr Next;
	FCB_ptr Previous;
}FCB;

/*Head is the head of circular linked list. Current is the pointer points to the FCB which should be switched to*/
extern FCB_ptr Current, Head;

/* Scheduler function. We have provided the pseudo code in the spec. You must follow the pseudo code to implement this function*/
void Scheduler();
