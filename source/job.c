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

//#define DEBUG6
// #define DEBUG7
//#define DEBUG8

int jobid=0;
int siginfo=1;
int fifo;
int globalfd;
int flag=1;

//struct waitqueue *head=NULL;
struct waitqueue *next=NULL,*current =NULL;
struct waitqueue *high, *mid, *low;

/**********************************/
/********��ת����ѡ����ʵ���ҵ****/
struct waitqueue* jobselect()		
{
	struct waitqueue *p,*prev,*select,*selectprev;
	int highest = -1;

	select = NULL;
	selectprev = NULL;
	if(high != NULL){
		select = high;
		high=high->next;		//highָ����һ����ҵ
	}
	else if(mid != NULL)
	{
		select = mid;
		mid=mid->next;			//midָ����һ����ҵ
	}
	else if(low != NULL)
	{
		select = low;
		low=low->next;			//lowָ����һ����ҵ
	}
	return select;
}

/**************************************************/
/****************���ؽ�Ҫ���ӵ���ҵ��***************/
int allocjid()
{
	return ++jobid;
}
/**************************************************/
/**********���µȴ������е���ҵ*********************/
void updateall()
{
	struct waitqueue *p,*prev,*head,*q;
	struct jobinfo *newjob;
	newjob = (struct jobinfo *)malloc(sizeof(struct jobinfo));
	head = (struct waitqueue*)malloc(sizeof(struct waitqueue));
	head->next =NULL;
	head->job=newjob;
	/* ������ҵ����ʱ�� */
	if(current)					//��ǰ������current��NULL
		current->job->run_time += 1; 		// ��1����1000ms
	/* ������ҵ�ȴ�ʱ�估���ȼ� --->���Ҫ��*/
	if(high!= NULL)
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
				//����߼����ȶ���
				p->job->wait_time=0;			//��Ҫ��������high��ô��				
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
				//ɾ��Ԫ��
				if(prev==p)				//˵����ͷָ��
				{
					mid=mid->next;
					p=mid;
				}
				else					//˵�����м�
				{
					prev->next=p->next;
				}				
				p=q->next;		//��ʱǰ�̲���
				
			}
			else
			{
				prev=p;
				p=p->next;
			}
		}
	}
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
				//�����м����ȶ���
				head=mid;
				p->job->wait_time=0;			//��Ҫ��������mid��ô��					
				if(mid==NULL)
					mid=p;
				else
				{
					while(head->next!=NULL)
						head=head->next;
					head->next=p;
					p->next=NULL;
				}
				//ɾ��Ԫ��
				if(prev==p)				//˵����ͷָ��
				{
					printf("touzizhi\n");					
					low=low->next;
					p=low;
				}
				else					//˵�����м�
				{
					prev->next=p->next;
				}				
				p=q->next;		//��ʱǰ�̲���
			}
			else
			{
				prev=p;
				p=p->next;
			}
		}
	}
}



void jobswitch()
{
	struct waitqueue *p;
	int i;
	if(current && current->job->state == DONE)	/* ��ǰ��ҵ��� */
	{ 
		/* ��ҵ��ɣ�ɾ���� */
		for(i = 0;(current->job->cmdarg)[i] != NULL; i++)
		{
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i] = NULL;
		}
		/* �ͷſռ� */
		free(current->job->cmdarg);
		free(current->job);
		free(current);
		current = NULL;
	}
	//next��ʾ�������������ҳ���Ҫִ�е���ҵ
	if(next == NULL && current == NULL) 		/* û����ҵҪ���� */
		return;
	else if (next != NULL && current == NULL)	//��ʼ�µ���ҵ
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
	else if (next != NULL && current != NULL)	/* �л���ҵ */
	{ 
		printf("switch to Pid: %d\n",next->job->pid);
		kill(current->job->pid,SIGSTOP);
		//current->job->curpri = current->job->defpri;		//��������ת���ȾͲ���Ҫ��Ϊ��������ȼ�߷��
		current->job->wait_time = 0;
		current->job->state = READY;
		/* �Żصȴ����� */
		switch(current->job->curpri)
		{
			case 3:		//�Żظ߼�����
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
			case 2:		//�Ż��м�����
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
			case 1:		//�Żصͼ�����
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
		current = next;
		next = NULL;
		current->job->state = RUNNING;
		current->job->wait_time = 0;
		kill(current->job->pid,SIGCONT);
		return;
	}
	else 				/* next == NULL��current != NULL�����л� */
	{
		if(flag)
		{
			kill(current->job->pid,SIGCONT);			
			flag--;
		}
		return;
	}		

}
/**********************���ȳ���*********************************/
void scheduler()
{
	struct jobinfo *newjob=NULL;		//��ҵ��Ϣ�ṹ��
	struct jobcmd cmd;			//��ҵ��������
	int  count = 0;
	bzero(&cmd,DATALEN);			//���ֽ��ַ���ǰn���ֽ�Ϊ���Ұ�����\0��
	if((count=read(fifo,&cmd,DATALEN))<0)
		error_sys("read fifo failed");

#ifdef DEBUG
	printf("Reading whether other process send command!\n");
	if(count)					
		printf("cmd cmdtype\t%d\ncmd defpri\t%d\ncmd data\t%s\n",cmd.type,cmd.defpri,cmd.data);
	else
		printf("no data read\n");
#endif

	/* ���������ȴ������е���ҵ */
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
	/* ��ת����ѡ������ȼ���ҵ */
	next=jobselect();
	/* ��ҵ�л� */
	jobswitch();
}

/****************************************************/
/***************�źŴ�����**************************/
void sig_handler(int sig,siginfo_t *info,void *notused)
//sigΪ�ź�ֵ���ڶ���������ָ��siginf_t�ṹ��ָ�룬�������ź�Я��������ֵ������������һ�㲻ʹ��
{
	int status;
	int ret;
	switch (sig)
	{
		case SIGVTALRM: 					/* �����ʱ�������õļ�ʱ��� */
			scheduler();
			return;
		case SIGCHLD: 						/* �ӽ��̽���ʱ���͸������̵��ź� */
			ret = waitpid(-1,&status,WNOHANG);
			if (ret == 0)
				return;
			if(WIFEXITED(status))				//�ж��ӽ����ǲ�����������
			{
				current->job->state = DONE;
				printf("normal termation, exit status = %d\n",WEXITSTATUS(status)); //ȡ���ӽ��̷��صĽ�������
			}
			else if (WIFSIGNALED(status))			//����ӽ����쳣����
				printf("abnormal termation, signal number = %d\n",WTERMSIG(status));
			else if (WIFSTOPPED(status))			//���SIGCHLD���ӽ�����ͣ���͵ĵ��ź�
				printf("child stopped, signal number = %d\n",WSTOPSIG(status));
			return;
		default:
			return;
	}
}
/*********************************************************/
/************************���Ӳ���*************************/
void do_enq(struct jobinfo *newjob,struct jobcmd enqcmd)
{
	struct waitqueue *newnode,*p;
	int i=0,pid;
	char *offset,*argvec,*q;
	char **arglist;
	sigset_t zeromask;

	sigemptyset(&zeromask);

	/* ��װjobinfo���ݽṹ */
	newjob = (struct jobinfo *)malloc(sizeof(struct jobinfo));
	newjob->jid = allocjid();
	newjob->defpri = enqcmd.defpri;
	newjob->curpri = enqcmd.defpri;
	newjob->ownerid = enqcmd.owner;
	newjob->state = READY;
	newjob->create_time = time(NULL);
	newjob->wait_time = 0;				//��ҵ�ڵȴ������еȴ���ʱ�䣬��ʼʱ����ȻΪ��
	newjob->run_time = 0;				//��ҵ����ʱ�䣬��ʼ���ӣ���ȻΪ�㡣
	
	arglist = (char**)malloc(sizeof(char*)*(enqcmd.argnum+1));
	newjob->cmdarg = arglist;
	
	offset = enqcmd.data;
	argvec = enqcmd.data;
	while (i < enqcmd.argnum)			//��һ��������������ģ���ð��Ϊ�ָ�ȡ��ÿһ������
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

	/*���ݳ�ʼ���ȼ�����Ӧ�ȴ������������µ���ҵ*/
	newnode = (struct waitqueue*)malloc(sizeof(struct waitqueue));
	newnode->next =NULL;
	newnode->job=newjob;
	if(newnode->job->defpri==3)		//˵���Ǹ߼������е�Ԫ��,�����ֱ��ִ��
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
	else if(newnode->job->defpri==2)	//˵�����м������е�Ԫ�أ����ж��������е�Ԫ���Ƿ��Ǹ߼�����Ԫ�ء�
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
	else if (newnode->job->defpri==1)	//˵���ǵͼ������е�Ԫ�أ����ж��������е�Ԫ���Ƿ��Ǹ�/�м�����Ԫ�ء�
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
	/*Ϊ��ҵ��������*/
	if((pid=fork())<0)
		error_sys("enq fork failed");
	if(pid==0)				//�ӽ���
	{
		printf("zijincheng\n");
		newjob->pid =getpid();		//�õ���ǰ���̵�pid
		raise(SIGSTOP);			/*�����ӽ���,�ȵ�ִ��*/
		printf("begin running\n");		
#ifdef DEBUG
		printf("begin running\n");
		for(i=0;arglist[i]!=NULL;i++)
			printf("arglist %s\n",arglist[i]);
#endif

		dup2(globalfd,1);			//�����ļ�����������׼���
		if(execv(arglist[0],arglist)<0)		// ִ������
			printf("exec failed\n");
		exit(1);
	}
	else					//������
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
	if (current && current->job->jid ==deqid)	//current jobid==deqid,��ֹ��ǰ��ҵ��˵����ǰ����ִ�е���ҵ��������Ҫɾ������ҵ
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
	else						//�����ڵȴ������в���deqid
	{ 
		select=NULL;
		selectprev=NULL;
		if(high)				//�Ӹ߼�����������
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
				if(select==selectprev)			//˵���ȴ������о�һ����ҵ
					high=NULL;		
			}
		}
		if(mid!=NULL && select==NULL)			//mid��Ϊ�ղ���û�ҵ�
		{
			for(prev=mid,p=mid;p!=NULL;prev=p,p=p->next)
				if(p->job->jid==deqid)
				{
					select=p;
					selectprev=prev;
					break;
				}
			if(select!=NULL)
			{
				selectprev->next=select->next;
				if(select==selectprev)			//˵���ȴ������о�һ����ҵ
					mid=NULL;		
			}
		}
		if(low!=NULL && select==NULL)			//low��Ϊ�ղ���û�ҵ�
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
				if(select==selectprev)			//˵���ȴ������о�һ����ҵ
					low=NULL;		
			}
		}
		if(select)					//˵���������ҵ�Ļ�
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
	*��ӡ������ҵ��ͳ����Ϣ:
	*1.��ҵID
	*2.����ID
	*3.��ҵ������
	*4.��ҵ����ʱ��
	*5.��ҵ�ȴ�ʱ��
	*6.��ҵ����ʱ��
	*7.��ҵ״̬
	*/

	/* ��ӡ��Ϣͷ�� */
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
	printf("�߼����У�\n");
	for(p=high;p!=NULL;p=p->next){
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
	printf("�м����У�\n");
	for(p=mid;p!=NULL;p=p->next){
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
	printf("�ͼ����У�\n");
	for(p=low;p!=NULL;p=p->next){
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
}
/**********************************************************/
/********************���������****************************/
int main()
{
	struct timeval interval;		//timeval�ṹ������ָ��ʱ��ֵ��iterval���
	struct itimerval new,old;		//itimeval��һ������ָ�����ʱ��ṹ��
	struct stat statbuf;			//����stat�ṹ��
	struct sigaction newact,oldact1,oldact2;//�����ź�
	if(stat("/tmp/server",&statbuf)==0)	//���FIFO�ļ�����,ɾ��
	{
		if(remove("/tmp/server")<0)
			error_sys("remove failed");
	}
	if(mkfifo("/tmp/server",0666)<0)	//���������FIFO�ļ�
		error_sys("mkfifo failed");
	if((fifo=open("/tmp/server",O_RDONLY|O_NONBLOCK))<0)	//�ڷ�����ģʽ�´�FIFO
		error_sys("open fifo failed");
	/* �����źŴ����� */
	newact.sa_sigaction=sig_handler;	//sig�źţ�action�������źŵ���֮��Ķ��������źŴ�������
	sigemptyset(&newact.sa_mask);		//��sa_mask�źż���ʼ����������ա�
	newact.sa_flags=SA_SIGINFO;		//����ʹ��sa_sigcation�źŴ����� 
	//���������������˵һ���������ź�ֵ��һ����SIGVTALRM����ʾ������ʱ�������
	sigaction(SIGCHLD,&newact,&oldact1);	
	sigaction(SIGVTALRM,&newact,&oldact2);

	/* ����ʱ����Ϊ1000���� */
	interval.tv_sec=1;
	interval.tv_usec=0;

	new.it_interval=interval;
	new.it_value=interval;
	
	setitimer(ITIMER_VIRTUAL,&new,&old);	//�ڶ��ּ�ʱ��������ִ�е�ʱ��
	while(siginfo==1)			//��ѭ����
		;
	close(fifo);
	close(globalfd);
	return 0;
}
