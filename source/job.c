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

int jobid=0;
int siginfo=1;
int fifo;
int globalfd;
int flag = 1;
int timer = 1,flag1=0;;

struct waitqueue *next=NULL,*current =NULL,*youxian;
struct waitqueue *high = NULL, *mid = NULL, *low = NULL;

/**********************\B5\F7\B6ȳ\CC\D0\F2*********************************/
void scheduler()
{
	struct waitqueue *p;
	struct jobinfo *newjob=NULL;		//\D7\F7ҵ\D0\C5Ϣ\BDṹ\CC\E5
	struct jobcmd cmd;			//\D7\F7ҵ\B5\F7\B6\C8\C3\FC\C1\EE
	int i;
	int  count = 0;
	bzero(&cmd,DATALEN);			//\D6\C3\D7ֽ\DA\D7ַ\FB\B4\AEǰn\B8\F6\D7ֽ\DAΪ\C1\E3\C7Ұ\FC\C0\A8\A1\AE\0\A1\AF
	if((count=read(fifo,&cmd,DATALEN))<0)
		error_sys("read fifo failed");

	/* \B8\FC\D0µȴ\FD\B6\D3\C1\D0\D6е\C4\D7\F7ҵ */
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
	/* ѡ\D4\F1\B8\DF\D3\C5\CFȼ\B6\D7\F7ҵ */
#ifdef TEST
	
	if(high){printf("high\n");
		for(p = high; p != NULL; p = p->next){
			printf("job_pid\tjob_curpri\n");
			printf("%d\t%d\n",p->job->pid,p->job->curpri);
		}
	}
	
	if(mid){printf("mid\n");
		for(p = mid; p != NULL; p = p->next){
			printf("job_pid\tjob_curpri\n");
			printf("%d\t%d\n",p->job->pid,p->job->curpri);
		}
	}
	
	if(low){printf("low\n");
		for(p = low; p != NULL; p = p->next){
			printf("job_pid\tjob_curpri\n");
			printf("%d\t%d\n",p->job->pid,p->job->curpri);
		}
	}
#endif
	if(timer == 1){
		next=jobselect();
		/* \D7\F7ҵ\C7л\BB */
		jobswitch();
	}
	else
		timer--;
}
/**************************************************/
/****************\B7\B5\BBؽ\ABҪ\BD\F8\B6ӵ\C4\D7\F7ҵ\BA\C5***************/
int allocjid()
{
	return ++jobid;
}
/**************************************************/
/**********\B8\FC\D0µȴ\FD\B6\D3\C1\D0\D6е\C4\D7\F7ҵ*********************/
void updateall()
{
	struct waitqueue *p, *prev, *q,*head;//prev\D3\C3\C0\B4\B4洢\BD\F8\D0б\BB\CC\E1\C9\FD\B5ȼ\B6\B5\C4\CF\EE\B5\C4ǰһ\CF\EE
	/* \B8\FC\D0\C2\D7\F7ҵ\D4\CB\D0\D0ʱ\BC\E4 */

	if(current)					//\B5\B1ǰ\D3\D0\C8\CE\CE\F1current\B7\C7NULL
		current->job->run_time += 1; 		// \BC\D31\B4\FA\B1\ED1000ms
	/* \B8\FC\D0\C2\D7\F7ҵ\B5ȴ\FDʱ\BC估\D3\C5\CFȼ\B6 */
	if(high != NULL){
		for(p = high; p!= NULL; p = p->next){
			p->job->wait_time += 1000;
		}
	}
	if(mid != NULL){
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
	}
	if(low != NULL){
		p=low;
		prev=low;
		while(p!=NULL)			
		{
			p->job->wait_time += 1000;
			//printf("%d\n",p->job->wait_time);
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
					//printf("touzizhi\n");					
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
}

struct waitqueue* jobselect()		
{
	struct waitqueue *p,*prev,*select,*selectprev;
	int highest = -1;
	select = NULL;
	selectprev = NULL;	
	if(flag1==1)		//说明有抢占作业
	{
		select=youxian;	
		flag1=0;	//恢复标记位
		switch(youxian->job->defpri)
		{			
			case 3:
			{
				for(prev = high, p = high; p != NULL; prev = p,p = p->next)
					if(p==youxian)
					{
						select = p;
						selectprev = prev;
					}
				selectprev->next = select->next;
				if (select == selectprev)
					high = NULL;
				break;			
			}	
			case 2:
			{
				for(prev = mid, p = mid; p != NULL; prev = p,p = p->next)
					if(p==youxian)
					{
						select = p;
						selectprev = prev;
					}
				selectprev->next = select->next;
				if (select == selectprev)
					mid = NULL;
				break;		
			}
			case 1:
			{
				for(prev =low, p =low; p != NULL; prev = p,p = p->next)
					if(p==youxian)
					{
						select = p;
						selectprev = prev;
					}
				selectprev->next = select->next;
				if (select == selectprev)
					low = NULL;
				break;	
			}
		}
	}
	else
	{
		if(high != NULL){
			select = high;
			//printf("123\n");
			high = high->next;
		}
		else if(mid != NULL){
			select = mid;
			mid = mid->next;
		}
		else if(low != NULL){
			select = low;
			low = low->next;
		}
	}
	return select;
}

void jobswitch()
{
	struct waitqueue *p;
	int i;

	if(current && current->job->state == DONE){ /* \B5\B1ǰ\D7\F7ҵ\CD\EA\B3\C9 */
		/* \D7\F7ҵ\CD\EA\B3ɣ\ACɾ\B3\FD\CB\FC */
		for(i = 0;(current->job->cmdarg)[i] != NULL; i++){
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i] = NULL;
		}
		/* \CAͷſռ\E4 */
		free(current->job->cmdarg);
		free(current->job);
		free(current);

		current = NULL;
	}

	if(next == NULL && current == NULL) /* û\D3\D0\D7\F7ҵҪ\D4\CB\D0\D0 */

		return;
	else if (next != NULL && current == NULL){ /* \BF\AAʼ\D0µ\C4\D7\F7ҵ */

		printf("begin start new job\n");
		current = next;
		current->next=NULL;
		next = NULL;
		switch(current->job->curpri){
			case 1: timer = 5;	break;
			case 2: timer = 2;	break;
			case 3: timer = 1;	break;
		}
		current->job->state = RUNNING;
		kill(current->job->pid,SIGCONT);
		return;
	}
	else if (next != NULL && current != NULL){ /* \C7л\BB\D7\F7ҵ */

		printf("switch to Pid: %d\n",next->job->pid);
		kill(current->job->pid,SIGSTOP);
		current->job->curpri = current->job->defpri;		//\B2\BB\BF\C9\D2\D4\D4ٻָ\B4\D3\C5\CFȼ\B6\C1\CB
		current->job->wait_time = 0;
		current->job->state = READY;

		/* \B7Żصȴ\FD\B6\D3\C1\D0 */
		current->next=NULL;
		switch(current->job->curpri){
		case 3:
			if(high != NULL){
				for(p = high; p->next != NULL; p = p->next);
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
		current = next;
		current->next=NULL;
		next = NULL;
		switch(current->job->curpri){
			case 1: timer = 5;	break;
			case 2: timer = 2;	break;
			case 3: timer = 1;	break;
		}
		current->job->state = RUNNING;
		current->job->wait_time = 0;
		kill(current->job->pid,SIGCONT);
		return;
	}else{ /* next == NULL\C7\D2current != NULL\A3\AC\B2\BB\C7л\BB */
		if(flag){
			kill(current->job->pid,SIGCONT);
			flag--;
		}
		return;
	}
}

/****************************************************/
/***************\D0źŴ\A6\C0\ED\BA\AF\CA\FD**************************/
void sig_handler(int sig,siginfo_t *info,void *notused)
//sigΪ\D0ź\C5ֵ\A3\AC\B5ڶ\FE\B8\F6\B2\CE\CA\FD\D6\D0ָ\CF\F2siginf_t\BDṹ\B5\C4ָ\D5룬\B0\FC\BA\AC\C1\CB\D0ź\C5Я\B4\F8\B5\C4\CA\FD\BE\DDֵ\A3\AC\B5\DA\C8\FD\B8\F6\B2\CE\CA\FDһ\B0㲻ʹ\D3\C3
{
	int status;
	int ret;
	switch (sig)
	{
		case SIGVTALRM: 					/* \B5\BD\B4\EF\BC\C6ʱ\C6\F7\CB\F9\C9\E8\D6õļ\C6ʱ\BC\E4\B8\F4 */
			scheduler();
			#ifdef DEBUG
				printf("SIGVTALRM RECEIVED!\n");
			#endif
			return;
		case SIGCHLD: 						/* \D7ӽ\F8\B3̽\E1\CA\F8ʱ\B4\AB\CB͸\F8\B8\B8\BD\F8\B3̵\C4\D0ź\C5 */
			ret = waitpid(-1,&status,WNOHANG);
			if (ret == 0)
				return;
			if(WIFEXITED(status))				//\C5ж\CF\D7ӽ\F8\B3\CC\CAǲ\BB\CA\C7\D5\FD\B3\A3\BD\E1\CA\F8
			{
				current->job->state = DONE;
				printf("normal termation, exit status = %d\n",WEXITSTATUS(status)); //ȡ\B5\C3\D7ӽ\F8\B3̷\B5\BBصĽ\E1\CA\F8\B4\FA\C2\EB
			}
			else if (WIFSIGNALED(status))			//\C8\E7\B9\FB\D7ӽ\F8\B3\CC\D2쳣\BD\E1\CA\F8
				printf("abnormal termation, signal number = %d\n",WTERMSIG(status));
			else if (WIFSTOPPED(status))			//\C8\E7\B9\FBSIGCHLD\CA\C7\D7ӽ\F8\B3\CC\D4\DDͣ\B7\A2\CB͵ĵ\C4\D0ź\C5
				printf("child stopped, signal number = %d\n",WSTOPSIG(status));
			return;
		default:
			return;
	}
}
/*********************************************************/
/************************\BD\F8\B6Ӳ\D9\D7\F7*************************/
void do_enq(struct jobinfo *newjob,struct jobcmd enqcmd)
{
	struct waitqueue *newnode,*p;
	int i=0,pid,flag1;
	char *offset,*argvec,*q;
	char **arglist;
	sigset_t zeromask;

	sigemptyset(&zeromask);

	/* \B7\E2װjobinfo\CA\FD\BEݽṹ */
	newjob = (struct jobinfo *)malloc(sizeof(struct jobinfo));
	newjob->jid = allocjid();
	newjob->defpri = enqcmd.defpri;
	newjob->curpri = enqcmd.defpri;
	newjob->ownerid = enqcmd.owner;
	newjob->state = READY;
	newjob->create_time = time(NULL);
	newjob->wait_time = 0;				//\D7\F7ҵ\D4ڵȴ\FD\B6\D3\C1\D0\D6еȴ\FD\B5\C4ʱ\BC䣬\B3\F5ʼʱ\BF\CC\D7\D4ȻΪ\C1\E3
	newjob->run_time = 0;				//\D7\F7ҵ\D4\CB\D0\D0ʱ\BC䣬\B3\F5ʼ\BD\F8\B6ӣ\AC\D7\D4ȻΪ\C1㡣
	
	arglist = (char**)malloc(sizeof(char*)*(enqcmd.argnum+1));
	newjob->cmdarg = arglist;
	
	offset = enqcmd.data;
	argvec = enqcmd.data;
	while (i < enqcmd.argnum)			//\D5\E2һ\B2\BD\BE\BF\BE\B9\CA\C7\C8\E7\BA\CE\D7\F6\B5ģ\BF\D2\D4ð\BA\C5Ϊ\B7ָ\F4ȡ\B3\F6ÿһ\B8\F6\B2\CE\CA\FD
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

	/*\CF\F2\B5ȴ\FD\B6\D3\C1\D0\D6\D0\D4\F6\BC\D3\D0µ\C4\D7\F7ҵ*/
	newnode = (struct waitqueue*)malloc(sizeof(struct waitqueue));
	newnode->next =NULL;
	newnode->job=newjob;
	if(current==NULL || newnode->job->defpri>=current->job->curpri)
	{
		flag1=1;
		youxian=newnode;
	}
		switch (newnode->job->defpri){
		case 3:			
			if(high != NULL)
			{
				if(current->job->curpri <= newnode->job->curpri){
					newnode->next = mid->next;
					mid->next = newnode;
				}
			}
			else
				high = newnode;
			break;
		case 2:
			if(mid != NULL)
			{//\C8\E7\B9\FB\D0½\F8\C3\FC\C1\EE\B5\C4\D3\C5\CFȼ\B6\B4\F3\D3ڵ\C8\D3ڶ\D3\C1\D0\D3\C5\CFȼ\B6\A3\AC\C4\C7ô\BE\CD\CC\ED\BCӵ\BD\B6\D3\CA\D7,ʵ\CF\D6\C7\C0ռ
				if(current->job->curpri <= newnode->job->curpri){
					newnode->next = mid->next;
					mid->next = newnode;
				}
				else{
					for(p = mid; p->next != NULL; p = p->next);
					p->next = newnode;
				}
			}
			else
				mid = newnode;
			break;
		case 1:
			if(low != NULL)
			{
				if(current->job->curpri <= newnode->job->curpri){
					newnode->next = low->next;
					low->next = newnode;
				}
				else{
					for(p = low; p->next != NULL; p = p->next);
					p->next = newnode;
				}
			}
			else
				low = newnode;
			break;
		}
		/*Ϊ\D7\F7ҵ\B4\B4\BD\A8\BD\F8\B3\CC*/
		if((pid=fork())<0)
			error_sys("enq fork failed");
		if(pid==0)				//\D7ӽ\F8\B3\CC
		{
			newjob->pid =getpid();		//\B5õ\BD\B5\B1ǰ\BD\F8\B3̵\C4pid
			raise(SIGSTOP);			/*\D7\E8\C8\FB\D7ӽ\F8\B3\CC,\B5ȵ\C8ִ\D0\D0*/
			dup2(globalfd,1);			//\B8\B4\D6\C6\CEļ\FE\C3\E8\CA\F6\B7\FB\B5\BD\B1\EA׼\CA\E4\B3\F6
			if(execv(arglist[0],arglist)<0)		// ִ\D0\D0\C3\FC\C1\EE
				printf("exec failed\n");
			exit(1);
		}
		else					//\B8\B8\BD\F8\B3\CC
		{
			newjob->pid=pid;
			wait(NULL);
		}
}

void do_deq(struct jobcmd deqcmd)
{
	int deqid,i;
	struct waitqueue *p,*prev,*select,*selectprev;
	deqid=atoi(deqcmd.data);

#ifdef DEBUG
	printf("deq jid %d\n",deqid);
#endif


	if (current && current->job->jid ==deqid)	//current jobid==deqid,\D6\D5ֹ\B5\B1ǰ\D7\F7ҵ\A3\AC˵\C3\F7\B5\B1ǰ\D5\FD\D4\DAִ\D0е\C4\D7\F7ҵ\BE\CD\CA\C7\CE\D2\C3\C7Ҫɾ\B3\FD\B5\C4\D7\F7ҵ
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
	else						//\BB\F2\D5\DF\D4ڵȴ\FD\B6\D3\C1\D0\D6в\E9\D5\D2deqid
	{
		select=NULL;
		selectprev=NULL;
		//ɨ\C3\E8\CB\F9\D3зǿն\D3\C1У\AC\D5ҵ\BDĿ\B1\EAID
		if(high){
			for(prev = high, p = high; p != NULL; prev = p, p = p->next)
				if(p->job->jid == deqid)
				{
					select = p;
					selectprev = prev;
					break;
				}
			selectprev->next = select->next;
			if(select == selectprev)		//˵\C3\F7\B5ȴ\FD\B6\D3\C1\D0\D6о\CDһ\B8\F6\D7\F7ҵ
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
			if(select == selectprev)		//˵\C3\F7\B5ȴ\FD\B6\D3\C1\D0\D6о\CDһ\B8\F6\D7\F7ҵ
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
			if(select == selectprev)		//˵\C3\F7\B5ȴ\FD\B6\D3\C1\D0\D6о\CDһ\B8\F6\D7\F7ҵ
				low = NULL;
		}
//\D2\D4ǰ\B5Ĵ\FA\C2\EB
		/*if(head)
		{
			for(prev=head,p=head;p!=NULL;prev=p,p=p->next)
				if(p->job->jid==deqid)
				{
					select=p;
					selectprev=prev;
					break;
				}
			selectprev->next=select->next;
			if(select==selectprev)			//˵\C3\F7\B5ȴ\FD\B6\D3\C1\D0\D6о\CDһ\B8\F6\D7\F7ҵ
				head=NULL;
		}*/
		if(select)					//\C8\E7\B9\FB\B2\BB\CA\C7һ\B8\F6\D7\F7ҵ\B5Ļ\B0
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
	*\B4\F2ӡ\CB\F9\D3\D0\D7\F7ҵ\B5\C4ͳ\BC\C6\D0\C5Ϣ:
	*1.\D7\F7ҵID
	*2.\BD\F8\B3\CCID
	*3.\D7\F7ҵ\CB\F9\D3\D0\D5\DF
	*4.\D7\F7ҵ\D4\CB\D0\D0ʱ\BC\E4
	*5.\D7\F7ҵ\B5ȴ\FDʱ\BC\E4
	*6.\D7\F7ҵ\B4\B4\BD\A8ʱ\BC\E4
	*7.\D7\F7ҵ״̬
	*/

	/* \B4\F2ӡ\D0\C5Ϣͷ\B2\BF */
	printf("JOBID\tPID\tcurpri\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\n");
	if(current){
		strcpy(timebuf,ctime(&(current->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("current\n%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			current->job->jid,
			current->job->pid,
			current->job->defpri,
			current->job->ownerid,
			current->job->run_time,
			current->job->wait_time,
			timebuf,"RUNNING");
	}
	for(p = high; p != NULL; p = p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("highshabi\n%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			p->job->jid,
			p->job->pid,
			p->job->defpri,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY");
	}
	for(p = mid; p != NULL; p = p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("midshabi\n%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			p->job->jid,
			p->job->pid,
			p->job->defpri,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY");
	}
	for(p = low; p != NULL; p = p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("lowshabi\ns%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			p->job->jid,
			p->job->pid,
			p->job->defpri,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY");
	}
//\D2\D4ǰ\B5Ĵ\FA\C2\EB
	/*for(p=head;p!=NULL;p=p->next){
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
	}*/
}
/**********************************************************/
/********************\D6\F7\BA\AF\CA\FD\C8\EB\BF\DA****************************/
int main()
{
	struct timeval interval;		//timeval\BDṹ\CC\E5\D3\C3\D3\DAָ\B6\A8ʱ\BC\E4ֵ\A3\ACiterval\BC\E4\B8\F4
	struct itimerval new,old;		//itimeval\A3\ACһ\B8\F6\D3\C3\D3\DAָ\B6\A8\BC\E4\B8\F4ʱ\BC\E4\BDṹ\CC\E5
	struct stat statbuf;			//\B6\A8\D2\E5stat\BDṹ\CC\E5
	struct sigaction newact,oldact1,oldact2;//\B6\A8\D2\E5\D0ź\C5
	//\B5\F7\CA\D4һ
	#ifdef DEBUG
		printf("DEBUG IS OPEN!\n");
	#endif	
	if(stat("/tmp/server",&statbuf)==0)	//\C8\E7\B9\FBFIFO\CEļ\FE\B4\E6\D4\DA,ɾ\B5\F4
	{
		if(remove("/tmp/server")<0)
			error_sys("remove failed");
	}
	if(mkfifo("/tmp/server",0666)<0)	//\BD\A8\C1\A2\CC\D8\CA\E2\B5\C4FIFO\CEļ\FE
		error_sys("mkfifo failed");
	if((fifo=open("/tmp/server",O_RDONLY|O_NONBLOCK))<0)	//\D4ڷ\C7\D7\E8\C8\FBģʽ\CF´\F2\BF\AAFIFO
		error_sys("open fifo failed");
	/* \BD\A8\C1\A2\D0źŴ\A6\C0\ED\BA\AF\CA\FD */
	newact.sa_sigaction=sig_handler;	//sig\D0źţ\ACaction\B6\AF\D7\F7\A3\AC\D0źŵ\BD\B4\EF֮\BA\F3\B5Ķ\AF\D7\F7\A3\AC\BC\B4\D0źŴ\A6\C0\ED\BA\AF\CA\FD\A1\A3
	sigemptyset(&newact.sa_mask);		//\BD\ABsa_mask\D0źż\AF\B3\F5ʼ\BB\AF\A3\AC\B2\A2\C7\D2\C7\E5\BFա\A3
	newact.sa_flags=SA_SIGINFO;		//\B1\ED\C3\F7ʹ\D3\C3sa_sigcation\D0źŴ\A6\C0\ED\BA\AF\CA\FD 
	sigaction(SIGCHLD,&newact,&oldact1);
	sigaction(SIGVTALRM,&newact,&oldact2);

	/* \C9\E8\D6\C3ʱ\BC\E4\BC\E4\B8\F4Ϊ1000\BA\C1\C3\EB */
	interval.tv_sec=1;
	interval.tv_usec=0;

	new.it_interval=interval;
	new.it_value=interval;
	setitimer(ITIMER_VIRTUAL,&new,&old);	//\B5ڶ\FE\D6ּ\C6ʱ\C6\F7\A3\AC\BD\F8\B3\CCִ\D0е\C4ʱ\BC\E4
	while(siginfo==1)			//\CB\C0ѭ\BB\B7\A1\A3
		;
	close(fifo);
	close(globalfd);
	return 0;
}
