/* Static header file the contains type/structure definitions for 
   processes and process groups */

#include "list.h"

/* Structure that represents a process. id is the proccess id */
 struct Process {
	//process id
	int pid;
	//current state will be used in commands like ps, jobs
	// 0 running fg
	// 1 stopped fg
	// 2 running bg
	// 3 stopped bg
	int state;
	
	//is part of a list of processes
	struct list_elem elem;
};

/* Structure that represents a process group. It contains a linked
   list groups that contains all processes in the group*/
//typedef struct Process_Group {

//};
