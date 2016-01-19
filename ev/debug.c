/**
 *   author       :   丁雪峰
 *   time         :   2016-01-02 15:30:08
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<string.h>
#include<execinfo.h>

void debug_handler(int signum)
{
	const int len=1024;
	void *func[len];
	size_t size, i;
	char **funs;

	signal(signum,SIG_DFL);

	size=backtrace(func, len);
	funs=(char**)backtrace_symbols(func,size);

	fprintf(stderr, "System error, Stack trace:\n");

	for(i=0; i < size; ++i)
        fprintf(stderr, ">> %2lu %s \n", i, funs[i]);

	free(funs);
}


void debug()
{
	signal(SIGSEGV,debug_handler); //Invaild memory address
	signal(SIGABRT,debug_handler); // Abort signal
}
