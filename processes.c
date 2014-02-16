#include "processes.h"

void process_init(struct Process* p, char* pname, int ppid, int ppgid, int pstate){
	p->name = (char*)malloc(sizeof(char)*1000);
	strcpy(p->name,pname);
	p->pid = ppid;
	p->pgid = ppgid;
	p->state = pstate;
}  

