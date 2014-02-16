/*
 * esh - the 'pluggable' shell.
 *
 * Developed by Godmar Back for CS 3214 Fall 2009
 * Virginia Tech.
 */
#include <stdio.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "esh-sys-utils.h"
#include "esh.h"
#include "list.h"
#include "processes.h"
#include <string.h>

void catch_sigint(int sig, siginfo_t* info, void* context);
void catch_sigtstp(int sig, siginfo_t* info, void* context);
void catch_child(int sig, siginfo_t* info, void* context);
list_elem* findPID(struct list* plist, int pid);
/* executes jobs command */
void jobs(struct list* pipelineList);
/* executes kill command */
void kill(struct list* pipelineList, int pid);

//list of processes
static struct list process_list;

static void
usage(char *progname)
{
    printf("Usage: %s -h\n"
        " -h            print this help\n"
        " -p  plugindir directory from which to load plug-ins\n",
        progname);

    exit(EXIT_SUCCESS);
}

/* Build a prompt by assembling fragments from loaded plugins that 
 * implement 'make_prompt.'
 *
 * This function demonstrates how to iterate over all loaded plugins.
 */
static char *
build_prompt_from_plugins(void)
{
    char *prompt = NULL;
    struct list_elem * e = list_begin(&esh_plugin_list);

    for (; e != list_end(&esh_plugin_list); e = list_next(e)) {
        struct esh_plugin *plugin = list_entry(e, struct esh_plugin, elem);

        if (plugin->make_prompt == NULL)
            continue;

        /* append prompt fragment created by plug-in */
        char * p = plugin->make_prompt();
        if (prompt == NULL) {
            prompt = p;
        } else {
            prompt = realloc(prompt, strlen(prompt) + strlen(p) + 1);
            strcat(prompt, p);
            free(p);
        }
		//return list pointer to the beginning
		list_rbegin (&process_list);
    }

    /* default prompt */
    if (prompt == NULL)
        prompt = strdup("esh> ");

    return prompt;
}

/* The shell object plugins use.
 * Some methods are set to defaults.
 */
struct esh_shell shell =
{
    .build_prompt = build_prompt_from_plugins,
    .readline = readline,       /* GNU readline(3) */ 
    .parse_command_line = esh_parse_command_line /* Default parser */
};

/* catch sigint (^c)  kill a foreground process */
void catch_sigint(int sig, siginfo_t* info, void* context){
	printf("caught sigint\n");	
	int status;
        int  pid;
	while ((pid = waitpid(-1, &status, WNOHANG|WCONTINUED|WUNTRACED)) > 0){
		//get process with the right pid and remove from process list
		struct list_elem *e = findPID(&process_list, pid);
		if (e){
			list_remove(e);
		}	
	}
}

/* catch sigtstp (^Z)  stop a foreground process */
void catch_sigtstp(int sig, siginfo_t* info, void* context){
	int status;
        int  pid;
	while ((pid = waitpid(-1, &status, WNOHANG|WCONTINUED|WUNTRACED)) > 0){
		//get process with the right pid and change the state
		struct list_elem *e = findPid(&process_list, pid);
		struct Process *pro = list_entry (e, struct Process, elem);
		pro->state = 1;
	}
	
}

/* catch sigchld when one or more child processes change state 
   taken and modified from Signal5 example */
void catch_child(int sig, siginfo_t* info, void* context){
    
    /* reap all children and/or report status change */
    int status;
    int  pid;
    while ((pid = waitpid(-1, &status, WNOHANG|WCONTINUED|WUNTRACED)) > 0){
	
	//get process
	struct list_elem *e = findPID(&process_list, pid);
	

      //process exited
      if( WIFEXITED(status)) {
          printf("Received sigal that child process (%d) terminated. \n", pid);
	  //remove process with pid from list
	  if (e){
		list_remove(e);
	   }
	   else{
		printf("ERROR: could not find child on list EXITED")
	   }	
      }
      //process stopped
      if (WIFSTOPPED(status)) { 
          printf("Received signal that child process (%d) stopped. \n", pid);
	   //set process with pid as stopped
	   if (e){
		struct Process *pro = list_entry (e, struct Process, elem);
		pro->state = 1;
	   }
	   else{
		printf("ERROR: could not find child on list STOPPED")
	   }		  
      }
      if (WIFCONTINUED(status)) {
          printf("Received signal that child process (%d) continued. \n", pid);
	   // need to set process with pid as continued
	   if (e){
		struct Process *pro = list_entry (e, struct Process, elem);
		pro->state = 0;
	   }
	   else{
		printf("ERROR: could not find child on list CONTINUED")
	   }	
		  
      }
      if( WIFSIGNALED(status)) {
          printf("Received signal that child process (%d) received signal [%d] \n", 
                  pid, WTERMSIG(status));
	  //TODO dont know...
      }
    }
}

int
main(int ac, char *av[])
{
    //set handlers for signals
    esh_signal_sethandler(SIGINT, catch_sigint);
    esh_signal_sethandler(SIGTSTP, catch_sigtstp);
    esh_signal_sethandler(SIGCHLD, catch_child);
	
	//initialize process list
	list_init (&process_list);

    int opt;
    list_init(&esh_plugin_list);

    /* Process command-line arguments. See getopt(3) */
    while ((opt = getopt(ac, av, "hp:")) > 0) {
        switch (opt) {
        case 'h':
            usage(av[0]);
            break;

        case 'p':
            esh_plugin_load_from_directory(optarg);
            break;
        }
    }

    esh_plugin_initialize(&shell);

    /* Read/eval loop. */
    for (;;) {
        /* Do not output a prompt unless shell's stdin is a terminal */
        char * prompt = isatty(0) ? shell.build_prompt() : NULL;
        char * cmdline = shell.readline(prompt);
        free (prompt);

        if (cmdline == NULL)  /* User typed EOF */
            break;

        struct esh_command_line * cline = shell.parse_command_line(cmdline);
        free (cmdline);
        if (cline == NULL)                  /* Error in command line */
            continue;

        if (list_empty(&cline->pipes)) {    /* User hit enter */
            esh_command_line_free(cline);
            continue;
        }
	
	//look at command determine what needs to be done
	//get first pipeline
	struct list_elem* element1 = list_front(&(cline->pipes));
	struct esh_pipeline *firstPipe = list_entry(element1, struct esh_pipeline, elem);
	
	//get the first command
	struct list_elem* element2 = list_front(&(firstPipe->commands));
	struct esh_command *firstCommand = list_entry(element2, struct esh_command, elem);

	//get string of first command test
	if (strcmp(firstCommand->argv[0], "jobs") == 0){
		printf("jobs initiated\n");
		//print out all jobs
		jobs(&process_list);
	}
	else if (strcmp(firstCommand->argv[0], "fg") == 0){
		printf("fg initiated\n");
	}
	else if (strcmp(firstCommand->argv[0], "bg") == 0){
		printf("bg initiated\n");
	}
	else if (strcmp(firstCommand->argv[0], "kill") == 0){
		printf("kill initiated\n");
		kill(&process_list, firstCommand->argv[1])
	}
	else if (strcmp(firstCommand->argv[0], "stop") == 0){
		printf("stop initiated\n");
	}
	else if(strcmp(firstCommand->argv[0], "logout") == 0){
		printf("logging out of shell\n");
		return 0;
	}
	//user wants to start a program
	else{	
		printf("user wants to start a program\n");
		 //you are in the child
		int pid = fork();
                if(pid == 0){
                        int ret = 0;
                        char* path = (char*)malloc(sizeof(char)*100);
                        strcat(path,"/bin/");
                        strcat(path,firstCommand->argv[0]);
                        ret = execv(path,firstCommand->argv);
                        if(ret!=0){
                                printf("Execution failed");
                        }
		//in the parent
		else {
			//initialize a process
			struct Process newProcess;
			newProcess->state = 0;
			newProcess->pid = pid;
			//TODO process group??
			//TODO initialize with constructors
			list_push_back(&process_list, newProcess->elem)
		}
                }
	}
	
        esh_command_line_print(cline);
        esh_command_line_free(cline);
    }
    return 0;
}

/* our version of the jobs command
   goes through list of available pipelines and prints them out */
void jobs(struct list* processList){
	//numerical counter for jobs
	int jobNum = 1;
	for (e = list_begin (list); e != list_end (list); e = list_next (e))
        {
	     //get current pipeline
             struct Process *pro = list_entry (e, struct Process, elem);
             
	     //print out pipeline data
	     printf("[%d]", jobNum);
	    
	     if (*pro->state == 1 || *pro->state == 3){
		printf("Stopped		");
	     }
	     else{
		printf("Running		");
	     }
	     
	     //TODO add name member get command name
	     //printf("%s\n", firstCommand->name);
	}
}

/* finds correct list element and returns it */
list_elem* findPID(struct list* list, int pid){

	struct list_elem *e;

	for (e = list_begin (list); e != list_end (list);
         	  e = list_next (e))
        {	
            	 struct Process *pro = list_entry (e, struct Process, elem);
	    	 if (pro->pid == pid){
			return e;
	     	}  
	}
	return NULL;
}

/* execute the kill command */
void kill(struct list* processList, int pid){
	//remove process from process list
	struct list_elem *e = findPID(processList, pid);
	if (e){
		list_remove(e);
	}
	
	//send a SIGKILL to child process with pid
	kill(pid, SIGINT);
}
