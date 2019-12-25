void Scheduler(void)
{
	int n, c;
	n = setjmp(SCHEDULER);
	if(n == -2)
	{
		if(Current->Next == Current)
		{
			arr[idx] = '\0';
			printf("%s\n", arr);
			fflush(stdout);
			exit(0);
		}
		else
		{
			Current->Next->Previous = Current->Previous;
			Current->Previous->Next = Current->Next;
		}
	}
	Current = Current->Next;
	longjmp(Current->Environment, 1);

}
