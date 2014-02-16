#ifndef __PROCESS_H
#define __PROCESS_H
/* Static header file the contains type/structure definitions for 
   processes and process groups */

#include "list.h"
#include <string.h>
#include <stdlib.h>
/* Structure that represents a process. id is the proccess id */
struct Process {
	//process id
	int pid;
	int pgid;
	//current state will be used in commands like ps, jobs
	// 0 running fg
	// 1 stopped fg
	// 2 running bg
	// 3 stopped bg
	int state;
	char* name;
	//is part of a list of processes
	struct list_elem elem;
};

/* Structure that represents a process group. It contains a linked
   list groups that contains all processes in the group*/
void process_init(struct Process* p, char* pname, int ppid, int ppgid, int pstate);
