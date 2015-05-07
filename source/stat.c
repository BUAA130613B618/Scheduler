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
 *     stat
 */
void usage()
{
	printf("Usage: stat\n");		
}

int main(int argc,char *argv[])
{
	struct jobcmd statcmd;
	int fd;

	if(argc!=1)
	{
		usage();
		return 1;
	}

	statcmd.type=STAT;
	statcmd.defpri=0;
	statcmd.owner=getuid();
	statcmd.argnum=0;
   	 #ifdef DEBUG//调试五
		printf("statcmd cmdtype\t%d(-1 means stat, -2means DEQ, -3 means STAT)\n"
			"statcmd owner\t%d\n"
			"statcmd defpri\t%d\n"//这里不需要data
			"statcmd argnum\t%d\n",	statcmd.type,statcmd.owner,statcmd.defpri,statcmd.argnum);

   	 #endif 
	if((fd=open("/tmp/server",O_WRONLY))<0)
		error_sys("stat open fifo failed");

	if(write(fd,&statcmd,DATALEN)<0)
		error_sys("stat write failed");

	close(fd);
	return 0;
}
