#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

char* home;
char *input;
pid_t shell_id;
pid_t shell_pgid;
int bg;
int fd_shell;

typedef struct command
{
	int argc;
	char *argv[100];
	struct command* next;
}command;

typedef struct process
{
	char *name;
	int pid;
	struct process* next;
}process;	

command* command_head=NULL;
process* process_head=NULL;


void initialize()
{
	home=malloc(100);
	home=getcwd(home,1000);
	fd_shell=STDERR_FILENO;bg=0;				
	signal (SIGINT, SIG_IGN);
	signal (SIGTSTP, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
	signal (SIGTTIN, SIG_IGN);
	signal (SIGTTOU, SIG_IGN);
	shell_pgid=getpid();
	if(setpgid(shell_pgid,shell_pgid)<0)
	{
		perror("Can't make shell a member of it's own process group");
		_exit(1);
	}
	tcsetpgrp(fd_shell,shell_pgid);	
}

void command_prompt()
{
//	printf("prompt called\n");
	char username[32];
	getlogin_r(username,32);
	char host[100];
	gethostname(host,100);
	char dir[1000];
	getcwd(dir,1000);
	char display_dir[1000];
	char* temp=strstr(dir,home);
	if (temp)
	{
		strcpy(display_dir,dir+strlen(home));
		printf("%s@%s:~%s",username,host,display_dir);
		fflush(stdout);
	}
	else
	{
		strcpy(display_dir,dir);
		printf("%s@%s:%s ",username,host,display_dir);
		fflush(stdout);
	}
}

void insert_command(int argc,char *argv[100],char* input,char* output)
{
	command* temp=malloc(sizeof(command));
	int i;
	for(i=0;i<argc;i++)
	{
		temp->argv[i]=malloc(sizeof(char)*100);
		strcpy(temp->argv[i],argv[i]);
	}
	temp->argc=argc;
	
	if (command_head==NULL)
	{
		command_head=temp;
		return;
	}
	else 
	{
		command* temp1=command_head;
		while(temp1->next!=NULL) temp1=temp1->next;
		temp1->next=temp;
		return;
	}

}

void insert_process(char *name,int pid)
{
	process* temp=malloc(sizeof(process));
	temp->name=malloc(100);
	strcpy(temp->name,name);
	temp->pid=pid;
	temp->next=NULL;
	if(process_head==NULL) process_head=temp;
	else
	{
		process* t=process_head;
		while(t->next!=NULL) t=t->next;
		t->next=temp;
		return;
	}
}

void delete_process(int pid)
{
	process* t=process_head;
	if(t->pid==pid) process_head=process_head->next;
	t->next=NULL; free(t);
	while(t->next!=NULL)
	{
		if(t->next->pid==pid)
		{
			process* temp=t->next;
			t->next=temp->next;
			temp->next=NULL;
			free(temp);
			return;
		}
		
	}
}

process* get_process_pid(int pid)
{
	process* temp=process_head;
	while(temp!=NULL)
	{
		if(temp->pid==pid) return temp;
		temp=temp->next;
	}
	return  NULL;
}

process* get_process_job(int n)
{
	process* temp=process_head;
	while(1) 
	{
		if(n==1) return temp;
		if(temp==NULL) return NULL;
		n--;
		temp=temp->next;
	}
	return temp;
}

void sighandler(int sig)
{
	if(sig==SIGINT)
	{
		printf("\n");
		command_prompt();
		fflush(stdout);
		}
	if(sig==SIGCHLD)
	{
		pid_t pid,status;
		while((pid=waitpid(-1,&status,WNOHANG))>0)
		{
			if(pid!=-1 && pid!=0)
			{
				if(WIFEXITED(status))				
				{
					process* temp=get_process_pid(pid);
					if(temp!=NULL)
					{
						printf("\n%s with pid %d exited normally\n",temp->name,temp->pid);
						command_prompt();
						fflush(stdout);
						delete_process(pid);
					}
				}
				else if(WIFSIGNALED(status))
				{
					process* temp=get_process_pid(pid);
					if(temp!=NULL)
					{
						printf("\n%s with pid %d signalled to exit\n",temp->name,temp->pid);
						command_prompt();	
						fflush(stdout);
						delete_process(pid);
					}
				}
			}
		 }		
	}

}

int parse(char* input)
{
	int count=0;
	char* sp1;
	char* sp2;
	char* token=strtok_r(input,";",&sp1);
	while(token!=NULL)
	{
		count++;
		int argc=0;
		char* input=NULL;
		char* output=NULL;
		char *argv[100];
		int i; for(i=0;i<100;i++) argv[i]=NULL;
		char* temp_token=strtok_r(token," ",&sp2);
		while(temp_token!=NULL)
		{	
			
			argv[argc]=malloc(sizeof(char)*100);
			strcpy(argv[argc++],temp_token);
			temp_token=strtok_r(NULL," ",&sp2);
		}
		insert_command(argc,argv,input,output);
		token=strtok_r(NULL,";",&sp1);
	}
	return count;
}

void execute()
{
	
	command* temp=command_head;
	int ret_value;
	if(strcmp(temp->argv[0],"pwd")==0 )
	{
		printf("%s\n",getcwd(NULL,0));
	}
	else if (strcmp(temp->argv[0],"echo")==0)
	{
		int j;
		for(j=1;j<temp->argc;j++)
		{
			printf("%s ",temp->argv[j]);
		}
		printf("\n");
	}
	else if (strcmp(temp->argv[0],"cd")==0)
	{
		if(temp->argv[1]==NULL) ret_value=chdir(home);
		else if(temp->argv[1][0]=='~') 
		{
			char ch_dir[1000]; 
			int j,l1=strlen(temp->argv[1]),l2=strlen(home);
			strcpy(ch_dir,home);
			for(j=1;j<l1;j++) ch_dir[j+l2-1]=temp->argv[1][j];
			ch_dir[l2+j-1]='\0';
			chdir(ch_dir);
		}
		else ret_value=chdir(temp->argv[1]);
		if(ret_value==-1)
		{
			printf("No such directory exists\n");
			}
	}
	else if(strcmp(temp->argv[0],"exit")==0)
	{
		_exit(0);
	}
	else
	{
		int pid=fork();
		printf("%d\n",pid);
		if(pid==0)
		{
			
			setpgid(0,0);
			if(bg==0) 
				tcsetpgrp(fd_shell,getpid());
			signal (SIGINT, SIG_DFL);
			signal (SIGQUIT, SIG_DFL);
			signal (SIGTSTP, sighandler);
			signal (SIGTTIN, SIG_DFL);
			signal (SIGTTOU, SIG_DFL);
			signal (SIGCHLD, SIG_DFL);
			execvp(temp->argv[0],temp->argv);
			exit(0);
		}
		if (bg==0)
		{
			int status;
			tcsetpgrp(fd_shell,pid);
			waitpid(pid,&status,WUNTRACED);
			signal (SIGTSTP, sighandler);
			tcsetpgrp(fd_shell,shell_pgid);
		}
		else if(bg==1)
		{
			printf("background process %s with id %d created\n",temp->argv[0],pid);
			insert_process(temp->argv[0],pid);
		}
	}
	fflush(stdout);

}

void semi_colon_parsing(){
	while(command_head!=NULL){
		execute();
		command_head=command_head->next;
	}
}

void delete_command(command* head)
{
	if(head==NULL) return;
	if(head->next==NULL) { free(head); return ; }
	delete_command(head->next);
	free(head);
}


int main(){
	
	initialize();
	input=malloc(sizeof(char)*1000);
	while(1)
	{
		signal(SIGCHLD,sighandler);
		signal(SIGINT,sighandler);
		signal(SIGTSTP,sighandler);
		bg=0;
		command_prompt();
		scanf(" %[^\n]",input);
		//input=readline();
		int i;
		for(i=strlen(input);i>=0;i--)
		{
			if(input[i]==' ') continue;
			else if(input[i]=='&') { input[i]=' '; bg=1; break; }
		}
		int num=parse(input);
		//print_command();
		if (num==1) execute();
		else semi_colon_parsing();
		fflush(stdout);
		command_head=NULL;
		delete_command(command_head);
	}
}
