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
#define DEBUG9

int jobid=0;
int siginfo=1;
int fifo;
int globalfd;

struct waitqueue *head=NULL;
struct waitqueue *next=NULL,*current =NULL;

/* µ÷¶È³ÌĞò */
void scheduler()
{
	struct jobinfo *newjob=NULL;
	struct jobcmd cmd;
	int  count = 0;
	bzero(&cmd,DATALEN);
	if((count=read(fifo,&cmd,DATALEN))<0)
		error_sys("read fifo failed");
#ifdef DEBUG9

	/*if(count){
		printf("cmd cmdtype\t%d\ncmd defpri\t%d\ncmd data\t%s\n",cmd.type,cmd.defpri,cmd.data);
	}
	else
		printf("no data read\n");*/
#endif

	/* ¸üĞÂµÈ´ı¶ÓÁĞÖĞµÄ×÷Òµ */
	updateall();

	switch(cmd.type){
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

	/* Ñ¡Ôñ¸ßÓÅÏÈ¼¶×÷Òµ */
	next=jobselect();
	/* ×÷ÒµÇĞ»» */
	jobswitch();
}

int allocjid()
{
	return ++jobid;
}

void updateall()
{
	struct waitqueue *p;

	/* ¸üĞÂ×÷ÒµÔËĞĞÊ±¼ä */
	if(current)
		current->job->run_time += 1; /* ¼Ó1´ú±í1000ms */

	/* ¸üĞÂ×÷ÒµµÈ´ıÊ±¼ä¼°ÓÅÏÈ¼¶ */
	for(p = head; p != NULL; p = p->next){
		p->job->wait_time += 1000;
		if(p->job->wait_time >= 5000 && p->job->curpri < 3){
			p->job->curpri++;
			p->job->wait_time = 0;
		}
	}
}

struct waitqueue* jobselect()
{
	struct waitqueue *p,*prev,*select,*selectprev;
	int highest = -1;

	select = NULL;
	selectprev = NULL;
	if(head){
		/* ±éÀúµÈ´ı¶ÓÁĞÖĞµÄ×÷Òµ£¬ÕÒµ½ÓÅÏÈ¼¶×î¸ßµÄ×÷Òµ */
		for(prev = head, p = head; p != NULL; prev = p,p = p->next)
			if(p->job->curpri > highest){
				select = p;
				selectprev = prev;
				highest = p->job->curpri;
			}
			selectprev->next = select->next;
			if (select == selectprev)
				head = NULL;
	}
	return select;
}

void jobswitch()
{
	struct waitqueue *p;
	int i;
#ifdef DEBUG9
	if(current!=NULL)
        {printf("before execute\n");
	printf("current job ID:%d,current pid ID:%d\n",current->job->jid,current->job->pid);
	printf("waiting queue!\n");
	printf("---------------------------\n");
	for(p=head;p!=NULL;p=p->next){
		printf("job ID:%d, process ID:%d\n",p->job->jid,p->job->pid);
	}}
	else{
		printf("current is NULL\n");
		printf("before execute\n");
	}
#endif

	if(current && current->job->state == DONE){ /* µ±Ç°×÷ÒµÍê³É */
		/* ×÷ÒµÍê³É£¬É¾³ıËü */
#ifdef DEBUG9
		printf("current job finished!\n");
#endif
		for(i = 0;(current->job->cmdarg)[i] != NULL; i++){
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i] = NULL;
		}
		/* é‡Šæ”¾ç©ºé—´ */
		free(current->job->cmdarg);
		free(current->job);
		free(current);

		current = NULL;
	}

	if(next == NULL && current == NULL) {/* Ã»ÓĞ×÷ÒµÒªÔËĞĞ */
#ifdef DEBUG9
		printf("No jobs\n");
#endif

		return;}
	else if (next != NULL && current == NULL){ /* ¿ªÊ¼ĞÂµÄ×÷Òµ */

		printf("begin start new job\n");
		current = next;
		next = NULL;
		current->job->state = RUNNING;
		kill(current->job->pid,SIGCONT);
#ifdef DEBUG9
		printf("after execute\n");
		printf("current job ID:%d,current pid ID:%d\n",current->job->jid,current->job->pid);
		printf("waiting queue!\n");
		printf("---------------------------\n");
		for(p=head;p!=NULL;p=p->next){
			printf("job ID:%d, process ID:%d\n",p->job->jid,p->job->pid);
		}
#endif
		return;
	}
	else if (next != NULL && current != NULL){ /* ÇĞ»»×÷Òµ */

		printf("switch to Pid: %d\n",next->job->pid);
		kill(current->job->pid,SIGSTOP);
		current->job->curpri = current->job->defpri;
		current->job->wait_time = 0;
		current->job->state = READY;

		/* ·Å»ØµÈ´ı¶ÓÁĞ */
		if(head){
			for(p = head; p->next != NULL; p = p->next);
			p->next = current;
		}else{
			head = current;
		}
		current = next;
		next = NULL;
		current->job->state = RUNNING;
		current->job->wait_time = 0;
		kill(current->job->pid,SIGCONT);
#ifdef DEBUG9
		printf("after execute\n");
		printf("current job ID:%d,current pid ID:%d\n",current->job->jid,current->job->pid);
		printf("waiting queue!\n");
		printf("---------------------------\n");
		for(p=head;p!=NULL;p=p->next){
			printf("job ID:%d, process ID:%d\n",p->job->jid,p->job->pid);
		}
#endif
		return;
	}else{ /* next == NULLÇÒcurrent != NULL£¬²»ÇĞ»» */
#ifdef DEBUG9
		printf("after execute\n");
		printf("current job ID:%d,current pid ID:%d\n",current->job->jid,current->job->pid);
		printf("waiting queue!\n");
		printf("---------------------------\n");
		for(p=head;p!=NULL;p=p->next){
			printf("job ID:%d, process ID:%d\n",p->job->jid,p->job->pid);
		}
#endif
		return;
	}
}

void sig_handler(int sig,siginfo_t *info,void *notused)
{
	int status;
	int ret;

	switch (sig) {
case SIGVTALRM: /* µ½´ï¼ÆÊ±Æ÷ËùÉèÖÃµÄ¼ÆÊ±¼ä¸ô */
	scheduler();
	return;
case SIGCHLD: /* ×Ó½ø³Ì½áÊøÊ±´«ËÍ¸ø¸¸½ø³ÌµÄĞÅºÅ */
	ret = waitpid(-1,&status,WNOHANG);
	if (ret == 0)
		return;
	if(WIFEXITED(status)){
		current->job->state = DONE;
		printf("normal termation, exit status = %d\n",WEXITSTATUS(status));
	}else if (WIFSIGNALED(status)){
		printf("abnormal termation, signal number = %d\n",WTERMSIG(status));
	}else if (WIFSTOPPED(status)){
		printf("child stopped, signal number = %d\n",WSTOPSIG(status));
	}
	return;
	default:
		return;
	}
}

void do_enq(struct jobinfo *newjob,struct jobcmd enqcmd)
{
	struct waitqueue *newnode,*p;
	int i=0,pid;
	char *offset,*argvec,*q;
	char **arglist;
	sigset_t zeromask;

	sigemptyset(&zeromask);

	/* å°è£…jobinfoæ•°æ®ç»“æ„ */
	newjob = (struct jobinfo *)malloc(sizeof(struct jobinfo));
	newjob->jid = allocjid();
	newjob->defpri = enqcmd.defpri;
	newjob->curpri = enqcmd.defpri;
	newjob->ownerid = enqcmd.owner;
	newjob->state = READY;
	newjob->create_time = time(NULL);
	newjob->wait_time = 0;
	newjob->run_time = 0;
	arglist = (char**)malloc(sizeof(char*)*(enqcmd.argnum+1));
	newjob->cmdarg = arglist;
	offset = enqcmd.data;
	argvec = enqcmd.data;
	while (i < enqcmd.argnum){
		if(*offset == ':'){
			*offset++ = '\0';
			q = (char*)malloc(offset - argvec);
			strcpy(q,argvec);
			arglist[i++] = q;
			argvec = offset;
		}else
			offset++;
	}

	arglist[i] = NULL;

#ifdef DEBUG9

	printf("enqcmd argnum %d\n",enqcmd.argnum);
	for(i = 0;i < enqcmd.argnum; i++)
		printf("parse enqcmd:%s\n",arglist[i]);

#endif

	/*ÏòµÈ´ı¶ÓÁĞÖĞÔö¼ÓĞÂµÄ×÷Òµ*/
	newnode = (struct waitqueue*)malloc(sizeof(struct waitqueue));
	newnode->next =NULL;
	newnode->job=newjob;

	if(head)
	{
		for(p=head;p->next != NULL; p=p->next);
		p->next =newnode;
	}else
		head=newnode;

	/*Îª×÷Òµ´´½¨½ø³Ì*/
	if((pid=fork())<0)
		error_sys("enq fork failed");

	if(pid==0){
		newjob->pid =getpid();
		/*×èÈû×Ó½ø³Ì,µÈµÈÖ´ĞĞ*/
		raise(SIGSTOP);
#ifdef DEBUG9

		printf("begin running\n");
		for(i=0;arglist[i]!=NULL;i++)
			printf("arglist %s\n",arglist[i]);
#endif

		/*¸´ÖÆÎÄ¼şÃèÊö·ûµ½±ê×¼Êä³ö*/
		dup2(globalfd,1);
		/* Ö´ĞĞÃüÁî */
		if(execv(arglist[0],arglist)<0)
			printf("exec failed\n");
		exit(1);
	}else{
		newjob->pid=pid;
	}
}

void do_deq(struct jobcmd deqcmd)
{
	int deqid,i;
	struct waitqueue *p,*prev,*select,*selectprev;
	deqid=atoi(deqcmd.data);

#ifdef DEBUG9
	printf("deq jid %d\n",deqid);
#endif

	/*current jodid==deqid,ÖÕÖ¹µ±Ç°×÷Òµ*/
	if (current && current->job->jid ==deqid){
		printf("teminate current job\n");
		kill(current->job->pid,SIGKILL);
		for(i=0;(current->job->cmdarg)[i]!=NULL;i++){
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i]=NULL;
		}
		free(current->job->cmdarg);
		free(current->job);
		free(current);
		current=NULL;
	}
	else{ /* »òÕßÔÚµÈ´ı¶ÓÁĞÖĞ²éÕÒdeqid */
		select=NULL;
		selectprev=NULL;
		if(head){
			for(prev=head,p=head;p!=NULL;prev=p,p=p->next)
				if(p->job->jid==deqid){
					select=p;
					selectprev=prev;
					break;
				}
				selectprev->next=select->next;
				if(select==selectprev)
					head=NULL;
		}
		if(select){
			for(i=0;(select->job->cmdarg)[i]!=NULL;i++){
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
	*æ‰“å°æ‰€æœ‰ä½œä¸šçš„ç»Ÿè®¡ä¿¡æ¯:
	*1.ä½œä¸šID
	*2.è¿›ç¨‹ID
	*3.ä½œä¸šæ‰€æœ‰è€…
	*4.ä½œä¸šè¿è¡Œæ—¶é—´
	*5.ä½œä¸šç­‰å¾…æ—¶é—´
	*6.ä½œä¸šåˆ›å»ºæ—¶é—´
	*7.ä½œä¸šçŠ¶æ€
	*/

	/* æ‰“å°ä¿¡æ¯å¤´éƒ¨ */
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

	for(p=head;p!=NULL;p=p->next){
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

int main()
{
	struct timeval interval;
	struct itimerval new,old;
	struct stat statbuf;
	struct sigaction newact,oldact1,oldact2;

	if(stat("/tmp/server",&statbuf)==0){
		/* Èç¹ûFIFOÎÄ¼ş´æÔÚ,É¾µô */
		if(remove("/tmp/server")<0)
			error_sys("remove failed");
	}

	if(mkfifo("/tmp/server",0666)<0)
		error_sys("mkfifo failed");
	/* ÔÚ·Ç×èÈûÄ£Ê½ÏÂ´ò¿ªFIFO */
	if((fifo=open("/tmp/server",O_RDONLY|O_NONBLOCK))<0)
		error_sys("open fifo failed");

	/* ½¨Á¢ĞÅºÅ´¦Àíº¯Êı */
	newact.sa_sigaction=sig_handler;
	sigemptyset(&newact.sa_mask);
	newact.sa_flags=SA_SIGINFO;
	sigaction(SIGCHLD,&newact,&oldact1);
	sigaction(SIGVTALRM,&newact,&oldact2);

	/* è®¾ç½®æ—¶é—´é—´éš”ä¸º1000æ¯«ç§’ */
	interval.tv_sec=1;
	interval.tv_usec=0;

	new.it_interval=interval;
	new.it_value=interval;
	setitimer(ITIMER_VIRTUAL,&new,&old);

	while(siginfo==1);

	close(fifo);
	close(globalfd);
	return 0;
}
