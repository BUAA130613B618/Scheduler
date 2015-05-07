#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include "job.h"

#define DEBUG
/* 
 * 命令语法格式
 *     deq jid
 */
void usage()
{
	printf("Usage: deq jid\n"
		"\tjid\t\t the job id\n");
}

int main(int argc,char *argv[])
{
	struct jobcmd deqcmd;
	int fd;
	
	if(argc!=2)
	{
		usage();
		return 1;
	}

	deqcmd.type=DEQ;
	deqcmd.defpri=0;
	deqcmd.owner=getuid();
	deqcmd.argnum=1;

	strcpy(deqcmd.data,*++argv);
	printf("jid %s\n",deqcmd.data);
	#ifdef DEBUG//调试五
		printf("deqcmd cmdtype\t%d(-1 means ENQ, -2means DEQ, -3 means STAT)\n"
			"deqcmd owner\t%d\n"
			"deqcmd defpri\t%d\n"
			"deqcmd data\t%s\n"
			"deqcmd argnum\t%d\n",	deqcmd.type,deqcmd.owner,deqcmd.defpri,deqcmd.data,deqcmd.argnum);
	#endif
	if((fd=open("/tmp/server",O_WRONLY))<0)
		error_sys("deq open fifo failed");

	if(write(fd,&deqcmd,DATALEN)<0)
		error_sys("deq write failed");

	close(fd);
	return 0;
}
