#include "header.h"


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
