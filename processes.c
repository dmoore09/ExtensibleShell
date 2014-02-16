void process_init(struct Processes* p, har* pname, int ppid, int ppgid, int pstate){
	p->name = (char*)malloc(sizeof(char)*100);
	p->strcpy(name,pname);
	p->pid = ppid;
	p->pgid = ppgid;
	p->state = pstate;
}  

