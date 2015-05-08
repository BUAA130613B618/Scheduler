#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include "job.h"

//#define DEBUG8

int jobid=0;
int siginfo=1;
int fifo;
int globalfd;
int flag=1;

//struct waitqueue *head=NULL;
struct waitqueue *next=NULL,*current =NULL;
<<<<<<< HEAD
struct waitqueue *high = NULL, *mid = NULL, *low = NULL;
=======
struct waitqueue *high, *mid, *low;
>>>>>>> 62237940a00972325534f2d9e46c638912107b2c

/**********************************/
/********轮转调度选择合适的作业****/
struct waitqueue* jobselect()		
{
<<<<<<< HEAD
	struct jobinfo *newjob=NULL;		//作业信息结构体
	struct jobcmd cmd;			//作业调度命令
#ifdef DEBUG7
	struct waitqueue *p;
	int num, i = 0;
#endif
	int  count = 0;
	bzero(&cmd,DATALEN);			//置字节字符串前n个字节为零且包括‘\0’
	if((count=read(fifo,&cmd,DATALEN))<0)
		error_sys("read fifo failed");

#ifdef DEBUG
	printf("Reading whether other process send command!\n");
	if(count)
		printf("cmd cmdtype\t%d\ncmd defpri\t%d\ncmd data\t%s\n",cmd.type,cmd.defpri,cmd.data);
	else
		printf("no data read\n");
#endif
=======
	struct waitqueue *p,*prev,*select,*selectprev;
	int highest = -1;
>>>>>>> 62237940a00972325534f2d9e46c638912107b2c

	select = NULL;
	selectprev = NULL;
	if(high != NULL){
		select = high;
		high=high->next;		//high指向下一个作业
	}
	else if(mid != NULL)
	{
		select = mid;
		mid=mid->next;			//mid指向下一个作业
	}
	else if(low != NULL)
	{
		select = low;
		low=low->next;			//low指向下一个作业
	}
	return select;
}

/**************************************************/
/****************返回将要进队的作业号***************/
int allocjid()
{
	return ++jobid;
}
/**************************************************/
/**********更新等待队列中的作业*********************/
void updateall()
{
<<<<<<< HEAD
<<<<<<< HEAD
	struct waitqueue *p, *prev, *q;//prev用来存储进行被提升等级的项的前一项
=======
	struct waitqueue *p;
>>>>>>> origin/master
=======
	struct waitqueue *p,*prev,*head,*q;
	struct jobinfo *newjob;
	newjob = (struct jobinfo *)malloc(sizeof(struct jobinfo));
	head = (struct waitqueue*)malloc(sizeof(struct waitqueue));
	head->next =NULL;
	head->job=newjob;
>>>>>>> 62237940a00972325534f2d9e46c638912107b2c
	/* 更新作业运行时间 */
	if(current)					//当前有任务current非NULL
		current->job->run_time += 1; 		// 加1代表1000ms
<<<<<<< HEAD
	/* 更新作业等待时间及优先级 */
	if(high != NULL){
		for(p = high; p!= NULL; p = p->next){
			p->job->wait_time += 1000;
		}
	}
	if(mid != NULL){
		for(p = mid, prev = mid; p != NULL;prev = p, p = p->next){
			p->job->wait_time += 1000;
			if(p->job->wait_time >= 10000){
				prev->next = p->next;
				if(high){
					for(q = high; q->next != NULL; q = q->next);
					q->next = p;
					p->next = NULL;
				}
				else{
					high = p;
					high->next = NULL;
				}
			}
		}
	}
	if(low != NULL){
		for(p = low, prev = low; p != NULL; prev = p, p = p->next){
			p->job->wait_time += 1000;
			if(p->job->wait_time >= 10000){
				prev->next = p->next;
				if(mid){
					for(q = high; q->next != NULL; q = q->next);
					q->next = p;
					p->next = NULL;
				}
				else{
					mid = p;
					mid->next = NULL;
				}
			}
		}
	}
	//以前的代码
	/*for(p = head; p != NULL; p = p->next)
=======
	/* 更新作业等待时间及优先级 --->提高要求*/
	if(high!= NULL)
>>>>>>> 62237940a00972325534f2d9e46c638912107b2c
	{
		for(p = high; p!= NULL;p = p->next)
			p->job->wait_time += 1000;
	}
	if(mid!= NULL)
	{
		p=mid;
		prev=mid;
		while(p!=NULL)			
		{
			p->job->wait_time += 1000;
			if(p->job->wait_time >= 10000)
			{			
				q=p;				
				//进入高级优先队列
				p->job->wait_time=0;			//需要按序存放于high中么？				
				p->job->curpri=3;
				if(high==NULL)
					high=p;
				else
				{
					head = high;
					while(head->next!=NULL)
						head=head->next;
					head->next=p;
					p->next=NULL;
				}
				//删除元素
				if(prev==p)				//说明是头指针
				{
					mid=mid->next;
					p=mid;
				}
				else					//说明在中间
				{
					prev->next=p->next;
				}				
				p=q->next;		//此时前继不变
				
			}
			else
			{
				prev=p;
				p=p->next;
			}
		}
<<<<<<< HEAD
	}*/
=======
	}
<<<<<<< HEAD
>>>>>>> origin/master
#ifdef DEBUG6
	for(p = head, num = 1; p != NULL; p = p->next, num++)
		printf("After the updata!\n"
			"job%d_jid\t%d\n"
			"job%d_pid\t%d\n"
			"0\n"
			"job%d_defpri\t%d\n"
			"job%d_curpri\t%d\n"
			"job%d_ownerid\t%d\n"
			"job%d_wait_time\t%d\n"
			"job%d_run_time\t%d\n", num, p->job->jid, num, p->job->pid, num, p->job->defpri, num, p->job->curpri, num, p->job->ownerid, num, p->job->wait_time, num, p->job->run_time);
#endif
}

struct waitqueue* jobselect()		
{
	struct waitqueue *p,*prev,*select,*selectprev;
	int highest = -1;

	select = NULL;
	selectprev = NULL;
	if(high != NULL){
		select = high;
		high = NULL;
	}
	else if(mid != NULL){
		select = mid;
		mid = NULL;
	}
	else if(low != NULL){
		select = low;
		low = NULL;
	}
//以前的代码
	/*if(head)
	{
		// 遍历等待队列中的作业，找到优先级最高的作业
		for(prev = head, p = head; p != NULL; prev = p,p = p->next)
			if(p->job->curpri > highest){
				select = p;
				selectprev = prev;
				highest = p->job->curpri;
			}
			selectprev->next = select->next;
			if (select == selectprev)
				head = NULL;
<<<<<<< HEAD
	}*/
=======
	}
>>>>>>> origin/master
#ifdef DEBUG8
	if(select != NULL)
	printf("Select job's information!\n"
			"job_jid\t%d\n"
			"job_pid\t%d\n"
			"job_cmdarg\t0\n"
			"job_defpri\t%d\n"
			"job_curpri\t%d\n"
			"job_ownerid\t%d\n"
			"job_wait_time\t%d\n"
			"job_run_time\t%d\n", select->job->jid, select->job->pid, select->job->defpri, select->job->curpri, select->job->ownerid,  select->job->wait_time, select->job->run_time);
#endif
	return select;
=======
	if(low != NULL)
	{
		p=low;
		prev=low;
		while(p!=NULL)			
		{
			p->job->wait_time += 1000;
			printf("%d\n",p->job->wait_time);
			if(p->job->wait_time >= 10000)
			{			
				q=p;				
				p->job->curpri=2;
				//进入中级优先队列
				head=mid;
				p->job->wait_time=0;			//需要按序存放于mid中么？					
				if(mid==NULL)
					mid=p;
				else
				{
					while(head->next!=NULL)
						head=head->next;
					head->next=p;
					p->next=NULL;
				}
				//删除元素
				if(prev==p)				//说明是头指针
				{
					printf("touzizhi\n");					
					low=low->next;
					p=low;
				}
				else					//说明在中间
				{
					prev->next=p->next;
				}				
				p=q->next;		//此时前继不变
			}
			else
			{
				prev=p;
				p=p->next;
			}
		}
	}
>>>>>>> 62237940a00972325534f2d9e46c638912107b2c
}



void jobswitch()
{
	struct waitqueue *p;
	int i;
	if(current && current->job->state == DONE)	/* 当前作业完成 */
	{ 
		/* 作业完成，删除它 */
		for(i = 0;(current->job->cmdarg)[i] != NULL; i++)
		{
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i] = NULL;
		}
		/* 释放空间 */
		free(current->job->cmdarg);
		free(current->job);
		free(current);
		current = NULL;
	}
	//next表示从三个队列中找出的要执行的作业
	if(next == NULL && current == NULL) 		/* 没有作业要运行 */
		return;
	else if (next != NULL && current == NULL)	//开始新的作业
	{ 					
		printf("begin start new job\n");
		current = next;
		next = NULL;
		current->job->state = RUNNING;
		printf("%d\n",kill(current->job->pid,SIGCONT));
		printf("%d", current->job->pid);
		printf("-------------------------\n");
		return;
	}
	else if (next != NULL && current != NULL)	/* 切换作业 */
	{ 
		printf("switch to Pid: %d\n",next->job->pid);
		kill(current->job->pid,SIGSTOP);
		//current->job->curpri = current->job->defpri;		//由于是轮转调度就不需要置为最初的优先级叻。
		current->job->wait_time = 0;
		current->job->state = READY;
		/* 放回等待队列 */
<<<<<<< HEAD
		switch(current->job->curpri){
		case 3:
			if(high != NULL){
				for(p = head; p->next != NULL; p = p->next);
				p->next = current;
			}
			else{
				high = current;
			}
			break;
		case 2:
			if(mid != NULL){
				for(p = mid; p->next != NULL; p = p->next);
				p->next = current;
			}
			else{
				mid = current;
			}
			break;
		case 1:
			if(low != NULL){
				for(p = low; p->next != NULL; p = p->next);
				p->next = current;
			}
			else{
				low = current;
			}
			break;
		}
//以前的代码
		/*if(head){
			for(p = head; p->next != NULL; p = p->next);
			p->next = current;
		}else{
			head = current;
		}*/
=======
		switch(current->job->curpri)
		{
			case 3:		//放回高级队列
			{
				if(high)		
				{
					for(p = high; p->next != NULL; p = p->next)
						;
					p->next = current;
				}
				else
					high = current;
				break;	
			}
			case 2:		//放回中级队列
			{
				if(mid)		
				{
					for(p = mid; p->next != NULL; p = p->next)
						;
					p->next = current;
				}
				else
					mid = current;
				break;	
			}
			case 1:		//放回低级队列
			{
				if(low)		
				{
					for(p = low; p->next != NULL; p = p->next)
						;
					p->next = current;
				}
				else
					low = current;
				break;	
			}			
		}
>>>>>>> 62237940a00972325534f2d9e46c638912107b2c
		current = next;
		next = NULL;
		current->job->state = RUNNING;
		current->job->wait_time = 0;
		kill(current->job->pid,SIGCONT);
		return;
	}
	else 				/* next == NULL且current != NULL，不切换 */
	{
		if(flag)
		{
			kill(current->job->pid,SIGCONT);			
			flag--;
		}
		return;
	}		

}
/**********************调度程序*********************************/
void scheduler()
{
	struct jobinfo *newjob=NULL;		//作业信息结构体
	struct jobcmd cmd;			//作业调度命令
	int  count = 0;
	bzero(&cmd,DATALEN);			//置字节字符串前n个字节为零且包括‘\0’
	if((count=read(fifo,&cmd,DATALEN))<0)
		error_sys("read fifo failed");

#ifdef DEBUG
	printf("Reading whether other process send command!\n");
	if(count)					
		printf("cmd cmdtype\t%d\ncmd defpri\t%d\ncmd data\t%s\n",cmd.type,cmd.defpri,cmd.data);
	else
		printf("no data read\n");
#endif

	/* 更新三个等待队列中的作业 */
	printf("sfcsklj\n");
	updateall();
	switch(cmd.type)
	{
		case ENQ:		
			do_enq(newjob,cmd);
			break;
		case DEQ:
			do_deq(cmd);
			break;
		case STAT:
			do_stat(cmd);
			break;
		default:
			break;
	}
	/* 轮转调度选择高优先级作业 */
	next=jobselect();
	/* 作业切换 */
	jobswitch();
}

/****************************************************/
/***************信号处理函数**************************/
void sig_handler(int sig,siginfo_t *info,void *notused)
//sig为信号值，第二个参数中指向siginf_t结构的指针，包含了信号携带的数据值，第三个参数一般不使用
{
	int status;
	int ret;
	switch (sig)
	{
		case SIGVTALRM: 					/* 到达计时器所设置的计时间隔 */
			scheduler();
			return;
		case SIGCHLD: 						/* 子进程结束时传送给父进程的信号 */
			ret = waitpid(-1,&status,WNOHANG);
			if (ret == 0)
				return;
			if(WIFEXITED(status))				//判断子进程是不是正常结束
			{
				current->job->state = DONE;
				printf("normal termation, exit status = %d\n",WEXITSTATUS(status)); //取得子进程返回的结束代码
			}
			else if (WIFSIGNALED(status))			//如果子进程异常结束
				printf("abnormal termation, signal number = %d\n",WTERMSIG(status));
			else if (WIFSTOPPED(status))			//如果SIGCHLD是子进程暂停发送的的信号
				printf("child stopped, signal number = %d\n",WSTOPSIG(status));
			return;
		default:
			return;
	}
}
/*********************************************************/
/************************进队操作*************************/
void do_enq(struct jobinfo *newjob,struct jobcmd enqcmd)
{
	struct waitqueue *newnode,*p;
	int i=0,pid;
	char *offset,*argvec,*q;
	char **arglist;
	sigset_t zeromask;

	sigemptyset(&zeromask);

	/* 封装jobinfo数据结构 */
	newjob = (struct jobinfo *)malloc(sizeof(struct jobinfo));
	newjob->jid = allocjid();
	newjob->defpri = enqcmd.defpri;
	newjob->curpri = enqcmd.defpri;
	newjob->ownerid = enqcmd.owner;
	newjob->state = READY;
	newjob->create_time = time(NULL);
	newjob->wait_time = 0;				//作业在等待队列中等待的时间，初始时刻自然为零
	newjob->run_time = 0;				//作业运行时间，初始进队，自然为零。
	
	arglist = (char**)malloc(sizeof(char*)*(enqcmd.argnum+1));
	newjob->cmdarg = arglist;
	
	offset = enqcmd.data;
	argvec = enqcmd.data;
	while (i < enqcmd.argnum)			//这一步究竟是如何做的？以冒号为分隔取出每一个参数
	{
		if(*offset == ':')
		{
			*offset++ = '\0';
			q = (char*)malloc(offset - argvec);
			strcpy(q,argvec);
			arglist[i++] = q;
			argvec = offset;
		}
		else
			offset++;
	}
	arglist[i] = NULL;

#ifdef DEBUG
	printf("enqcmd argnum %d\n",enqcmd.argnum);
	for(i = 0;i < enqcmd.argnum; i++)
		printf("parse enqcmd:%s\n",arglist[i]);
#endif

	/*根据初始优先级向相应等待队列中增加新的作业*/
	newnode = (struct waitqueue*)malloc(sizeof(struct waitqueue));
	newnode->next =NULL;
	newnode->job=newjob;
<<<<<<< HEAD

	switch (newnode->job->curpri){
		case 3:
			if(high != NULL)
			{
				for(p = high; p->next != NULL; p = p->next);
				p->next = newnode;
			}
			else
				high = newnode;
			break;
		case 2:
			if(mid != NULL)
			{
				for(p = mid; p->next != NULL; p = p->next);
				p->next = newnode;
			}
			else
				mid = newnode;
			break;
		case 1:
			if(low != NULL)
			{
				for(p = low; p->next != NULL; p = p->next);
				p->next = newnode;
			}
			else
				low = newnode;
			break;
	}
//以前的代码
	/*if(head)		//说明当前等待队列中是有作业的
=======
	if(newnode->job->defpri==3)		//说明是高级队列中的元素,必须得直接执行
>>>>>>> 62237940a00972325534f2d9e46c638912107b2c
	{
		if(high==NULL)
			high=newnode;
		else
		{
			for(p = high; p->next != NULL; p = p->next)
				;
			p->next = newnode;			
		}
	}
	else if(newnode->job->defpri==2)	//说明是中级队列中的元素，得判断正在运行的元素是否是高级队列元素。
	{
		if(mid==NULL)
			mid=newnode;
		else
		{
			for(p = mid; p->next != NULL; p = p->next)
				;
			p->next = newnode;			
		}
	}
	else if (newnode->job->defpri==1)	//说明是低级队列中的元素，得判断正在运行的元素是否是高/中级队列元素。
	{	
		if(low==NULL)
			low=newnode;
		else
		{
			for(p = low; p->next != NULL; p = p->next)
				;
			p->next = newnode;			
		}
	}
<<<<<<< HEAD
	else
		head=newnode;*/

=======
>>>>>>> 62237940a00972325534f2d9e46c638912107b2c
	/*为作业创建进程*/
	if((pid=fork())<0)
		error_sys("enq fork failed");
	if(pid==0)				//子进程
	{
		printf("zijincheng\n");
		newjob->pid =getpid();		//得到当前进程的pid
		raise(SIGSTOP);			/*阻塞子进程,等等执行*/
		printf("begin running\n");		
#ifdef DEBUG
		printf("begin running\n");
		for(i=0;arglist[i]!=NULL;i++)
			printf("arglist %s\n",arglist[i]);
#endif

		dup2(globalfd,1);			//复制文件描述符到标准输出
		if(execv(arglist[0],arglist)<0)		// 执行命令
			printf("exec failed\n");
		exit(1);
	}
	else					//父进程
		newjob->pid=pid;
}

void do_deq(struct jobcmd deqcmd)
{
	int deqid,i;
	struct waitqueue *p,*prev,*select,*selectprev;
	deqid=atoi(deqcmd.data);

#ifdef DEBUG
	printf("deq jid %d\n",deqid);
#endif
	if (current && current->job->jid ==deqid)	//current jobid==deqid,终止当前作业，说明当前正在执行的作业就是我们要删除的作业
	{
		printf("teminate current job\n");
		kill(current->job->pid,SIGKILL);
		for(i=0;(current->job->cmdarg)[i]!=NULL;i++)
		{
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i]=NULL;
		}
		free(current->job->cmdarg);
		free(current->job);
		free(current);
		current=NULL;
	}
	else						//或者在等待队列中查找deqid
	{
		select=NULL;
		selectprev=NULL;
<<<<<<< HEAD
		//扫描所有非空队列，找到目标ID
		if(high){
			for(prev = high, p = high; p != NULL; prev = p, p = p->next)
				if(p->job->jid == deqid)
				{
					select = p;
					selectprev = prev;
					break;
				}
			selectprev->next = select->next;
			if(select == selectprev)		//说明等待队列中就一个作业
				high = NULL;
		}
		if(mid){
			for(prev = mid, p = mid; p != NULL; prev = p, p = p->next)
				if(p->job->jid == deqid)
				{
					select = p;
					selectprev = prev;
					break;
				}
			selectprev->next = select->next;
			if(select == selectprev)		//说明等待队列中就一个作业
				mid = NULL;
		}
		if(low){
			for(prev = low, p = low; p != NULL; prev = p, p = p->next)
				if(p->job->jid == deqid)
				{
					select = p;
					selectprev = prev;
					break;
				}
			selectprev->next = select->next;
			if(select == selectprev)		//说明等待队列中就一个作业
				low = NULL;
		}
//以前的代码
		/*if(head)
=======
		if(high)				//从高级队列中找起
		{
			for(prev=high,p=high;p!=NULL;prev=p,p=p->next)
				if(p->job->jid==deqid)
				{
					select=p;
					selectprev=prev;
					break;
				}
			if(select!=NULL)
			{
				selectprev->next=select->next;
				if(select==selectprev)			//说明等待队列中就一个作业
					high=NULL;		
			}
		}
		if(mid!=NULL && select==NULL)			//mid不为空并且没找到
>>>>>>> 62237940a00972325534f2d9e46c638912107b2c
		{
			for(prev=mid,p=mid;p!=NULL;prev=p,p=p->next)
				if(p->job->jid==deqid)
				{
					select=p;
					selectprev=prev;
					break;
				}
<<<<<<< HEAD
			selectprev->next=select->next;
			if(select==selectprev)			//说明等待队列中就一个作业
				head=NULL;
		}*/
		if(select)					//如果不是一个作业的话
=======
			if(select!=NULL)
			{
				selectprev->next=select->next;
				if(select==selectprev)			//说明等待队列中就一个作业
					mid=NULL;		
			}
		}
		if(low!=NULL && select==NULL)			//low不为空并且没找到
		{
			for(prev=low,p=low;p!=NULL;prev=p,p=p->next)
				if(p->job->jid==deqid)
				{
					select=p;
					selectprev=prev;
					break;
				}
			if(select!=NULL)
			{
				selectprev->next=select->next;
				if(select==selectprev)			//说明等待队列中就一个作业
					low=NULL;		
			}
		}
		if(select)					//说明有这个作业的话
>>>>>>> 62237940a00972325534f2d9e46c638912107b2c
		{
			for(i=0;(select->job->cmdarg)[i]!=NULL;i++)
			{
				free((select->job->cmdarg)[i]);
				(select->job->cmdarg)[i]=NULL;
			}
			free(select->job->cmdarg);
			free(select->job);
			free(select);
			select=NULL;
		}
	}
}

void do_stat(struct jobcmd statcmd)
{
	struct waitqueue *p;
	char timebuf[BUFLEN];
	/*
	*打印所有作业的统计信息:
	*1.作业ID
	*2.进程ID
	*3.作业所有者
	*4.作业运行时间
	*5.作业等待时间
	*6.作业创建时间
	*7.作业状态
	*/

	/* 打印信息头部 */
	printf("JOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\n");
	if(current){
		strcpy(timebuf,ctime(&(current->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			current->job->jid,
			current->job->pid,
			current->job->ownerid,
			current->job->run_time,
			current->job->wait_time,
			timebuf,"RUNNING");
	}
<<<<<<< HEAD
	for(p = high; p != NULL; p = p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY");
	}
	for(p = mid; p != NULL; p = p->next){
=======
	printf("高级队列：\n");
	for(p=high;p!=NULL;p=p->next){
>>>>>>> 62237940a00972325534f2d9e46c638912107b2c
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY");
	}
<<<<<<< HEAD
	for(p = low; p != NULL; p = p->next){
=======
	printf("中级队列：\n");
	for(p=mid;p!=NULL;p=p->next){
>>>>>>> 62237940a00972325534f2d9e46c638912107b2c
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
<<<<<<< HEAD
=======

>>>>>>> 62237940a00972325534f2d9e46c638912107b2c
			p->job->wait_time,
			timebuf,
			"READY");
	}
<<<<<<< HEAD
//以前的代码
	/*for(p=head;p!=NULL;p=p->next){
=======
	printf("低级队列：\n");
	for(p=low;p!=NULL;p=p->next){
>>>>>>> 62237940a00972325534f2d9e46c638912107b2c
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY");
<<<<<<< HEAD
	}*/
=======
	}	
>>>>>>> 62237940a00972325534f2d9e46c638912107b2c
}
/**********************************************************/
/********************主函数入口****************************/
int main()
{
	struct timeval interval;		//timeval结构体用于指定时间值，iterval间隔
	struct itimerval new,old;		//itimeval，一个用于指定间隔时间结构体
	struct stat statbuf;			//定义stat结构体
	struct sigaction newact,oldact1,oldact2;//定义信号
	if(stat("/tmp/server",&statbuf)==0)	//如果FIFO文件存在,删掉
	{
		if(remove("/tmp/server")<0)
			error_sys("remove failed");
	}
	if(mkfifo("/tmp/server",0666)<0)	//建立特殊的FIFO文件
		error_sys("mkfifo failed");
	if((fifo=open("/tmp/server",O_RDONLY|O_NONBLOCK))<0)	//在非阻塞模式下打开FIFO
		error_sys("open fifo failed");
	/* 建立信号处理函数 */
	newact.sa_sigaction=sig_handler;	//sig信号，action动作，信号到达之后的动作，即信号处理函数。
	sigemptyset(&newact.sa_mask);		//将sa_mask信号集初始化，并且清空。
	newact.sa_flags=SA_SIGINFO;		//表明使用sa_sigcation信号处理函数 
	//对于这个处理函数来说一共有两种信号值，一个是SIGVTALRM，表示计数器时间结束。
	sigaction(SIGCHLD,&newact,&oldact1);	
	sigaction(SIGVTALRM,&newact,&oldact2);

	/* 设置时间间隔为1000毫秒 */
	interval.tv_sec=1;
	interval.tv_usec=0;

	new.it_interval=interval;
	new.it_value=interval;
	
	setitimer(ITIMER_VIRTUAL,&new,&old);	//第二种计时器，进程执行的时间
	while(siginfo==1)			//死循环。
		;
	close(fifo);
	close(globalfd);
	return 0;
}
