/* Static header file the contains type/structure definitions for 
   processes and process groups */

#include "list.h"

/* Structure that represents a process. id is the proccess id */
typedef struct Process {
	//process id
	int pid;
	//current state will be used in commands like ps, jobs
	String state;
	
};

/* Structure that represents a process group. It contains a linked
   list groups that contains all processes in the group*/
typedef struct Process_Group {
	struct list groups;
};
