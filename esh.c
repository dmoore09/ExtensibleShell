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
#include <unistd.h>
#include <stdlib.h>

void catch_sigint(int sig, siginfo_t* info, void* context);
void catch_sigtstp(int sig, siginfo_t* info, void* context);
void catch_child(int sig, siginfo_t* info, void* context);
struct list_elem* findPID(struct list* plist, int pid);
/* executes jobs command */
void jobs(struct list* pipelineList);
/* executes kill command */
void killProcess(int pid);
/* exectues the stop command */
void stopProcess(int pid);
/* gives a process group terminal control  used in fg, starting new process */
void giveTerminalControl(int pid);
/* give a process group control of the terminal credit: project1 faq #4*/
void give_terminal_to(pid_t pgrp, struct termios *pg_tty_state);
/* initialize pipeline fields that can be defined once a process is started */
void initializeJob(struct esh_pipeline* job, int pgrp, enum job_status status);

//list of processes
static struct list jobs_list;
//shell state used to restore state
struct termios *shell_state;

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
		list_rbegin (&jobs_list);
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

/* catch sigint (^c)  kill a foreground process  
   TODO sigsetjmp()/siglongjmp() FAQ 13 */
void catch_sigint(int sig, siginfo_t* info, void* context){
	printf("caught sigint\n");	
	int status;
        int  pid;
	while ((pid = waitpid(-1, &status, WNOHANG|WCONTINUED|WUNTRACED)) > 0){
		//get process with the right pid and remove from process list
		struct list_elem *e = findPID(&jobs_list, pid);
		if (e){
			list_remove(e);
		}	
	}
}

/* catch sigtstp (^Z)  stop a foreground process 
   TODO sigsetjmp()/siglongjmp() FAQ 13*/
void catch_sigtstp(int sig, siginfo_t* info, void* context){
	int status;
        int  pid;
	while ((pid = waitpid(-1, &status, WNOHANG|WCONTINUED|WUNTRACED)) > 0){
		//get process with the right pid and change the state
		struct list_elem *e = findPID(&jobs_list, pid);
		if (e){
			struct esh_pipeline *pipe = list_entry (e, struct esh_pipeline, elem);
			pipe->status = STOPPED;
		}
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
	struct list_elem *e = findPID(&jobs_list, pid);
	

      //process exited
      if( WIFEXITED(status)) {
          printf("Received sigal that child process (%d) terminated. \n", pid);
	  //remove process with pid from list
	  if (e){
		list_remove(e);
	   }
	   else{
		printf("ERROR: could not find child on list EXITED");
	   }	
      }
      //process stopped
      if (WIFSTOPPED(status)) { 
          printf("Received signal that child process (%d) stopped. \n", pid);
	   //set process with pid as stopped
	   if (e){
		struct esh_pipeline *pipe = list_entry (e, struct esh_pipeline, elem);
		pipe->status = STOPPED;
	   }
	   else{
		printf("ERROR: could not find child on list STOPPED");
	   }		  
      }
      if (WIFCONTINUED(status)) {
          printf("Received signal that child process (%d) continued. \n", pid);
	   // need to set process with pid as continued
	   if (e){
		struct esh_pipeline *pipe = list_entry (e, struct esh_pipeline, elem);
		pipe->status = FOREGROUND;
	   }
	   else{
		printf("ERROR: could not find child on list CONTINUED");
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
	list_init (&jobs_list);
    //back up shell state
    shell_state = esh_sys_tty_init();

    int opt;
    list_init(&esh_plugin_list);
    //Sets up the commands
    int cmdsize = 0;
    char** cmds = (char**)malloc(sizeof(char*)*100);
    int i = 0;
    for(i=0;i<100;i++){
            cmds[i] = (char*)malloc(sizeof(char)*10);
    }
    strcpy(cmds[0],"ls");
    strcpy(cmds[1],"cd");
    cmdsize = 2;

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
	
	//TODO loop for all pipelines
	//look at command determine what needs to be done
	//get first pipeline
	struct list_elem* element1 = list_front(&(cline->pipes));
	struct esh_pipeline *pipe = list_entry(element1, struct esh_pipeline, elem);
	
	//TODO loop for all commands in pipeline
	//get the first command
	struct list_elem* element2 = list_front(&(pipe->commands));
	struct esh_command *firstCommand = list_entry(element2, struct esh_command, elem);

	//get string of first command test
	if (strcmp(firstCommand->argv[0], "jobs") == 0){
		//print out all jobs
		jobs(&jobs_list);
	}
	else if (strcmp(firstCommand->argv[0], "fg") == 0){

	}
	else if (strcmp(firstCommand->argv[0], "bg") == 0){

	}
	else if (strcmp(firstCommand->argv[0], "kill") == 0){
		int killPid = atoi(firstCommand->argv[1]);
		killProcess(killPid);
	}
	else if (strcmp(firstCommand->argv[0], "stop") == 0){
		//TODO save terminal State! FAQ #12
		int stopPid = atoi(firstCommand->argv[1]);
		stopProcess(stopPid);
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
			
		        int j = 0;
   		        for(j = 0;j < cmdsize;j++){
                       		if(strcmp(cmds[j],firstCommand->argv[0]) == 0){
                       			fprintf(stderr,"This is a basic command");
					execvp(firstCommand->argv[0], firstCommand->argv);
                		}
        		}
			
				
			//make process leader of its own process group
			//setpgid(0, 0);

                        int ret = 0;
                        char* path = (char*)malloc(sizeof(char)*100);
			getcwd(path, 100);
                        strcat(path,"/");
			strcat(path,firstCommand->argv[0]);

                        ret = execv(path,firstCommand->argv);	
                        if(ret!=0){     
			  printf("Execution failed\n");
			  exit(0);
                        }
		}
		//in the parent
		else {
			//check for &
			//int counter = 0;
			//bool bg = false;
			//while (firstCommand->argv[counter] != NULL){
			//	if (strcmp(firstCommand->argv[counter], "&") == 0){
			//		bg = true;
			//		break;
			//	}
			//}		
			
			//initialize child process pid
			printf("child pid: %d\n", pid);			
			firstCommand->pid = pid;

			//double check, set child into process group 
			//TODO first process in pipe olny!!!
			//setpgid(pid, pid);

			//add new job to jobs list
			//TODO make sure pgrp is the first process and not reset
			//TODO need distinction from background and foreground
			initializeJob(pipe, pid, FOREGROUND);
			list_remove(&(pipe->elem));
			list_push_back(&jobs_list, &(pipe->elem));
			printf("job added\n");
		}
                
	}
	
        esh_command_line_free(cline);
    }
    return 0;
}

/* our version of the jobs command
   goes through list of available pipelines and prints them out */
void jobs(struct list* jobList){
	//numerical counter for jobs
	int jobNum = 1;
	struct list_elem *e;
	for (e = list_begin (jobList); e != list_end (jobList); e = list_next (e))
        {
	     //get current job
             struct esh_pipeline *job = list_entry (e, struct esh_pipeline, elem);
             
	     //print out pipeline data
	     printf("[%d]", jobNum);
	    
	     if (job->status == STOPPED || job->status == NEEDSTERMINAL){
		printf("Stopped		");
	     }
	     else{
		printf("Running		");
	     }
	     
	     struct list_elem* element2 = list_front(&(job->commands));
	     struct esh_command *firstCommand = list_entry(element2, struct esh_command, elem);

	     //print command name
	     printf("%s\n", firstCommand->argv[0]);
	}
}


/* finds correct list element and returns it */
struct list_elem* findPID(struct list* list, int pid){

	struct list_elem *e;

	for (e = list_begin (list); e != list_end (list);
         	  e = list_next (e))
        {	
            	 struct esh_pipeline *pipe = list_entry (e, struct esh_pipeline, elem);
		 
		 struct list_elem *e2;

		for (e2 = list_begin (&(pipe->commands)); e2 != list_end (&(pipe->commands)); e2 = list_next (e2))
       		{	
			struct esh_command *cmd = list_entry (e2, struct esh_command, elem);
	    		 if (cmd->pid == pid){
				printf("Found process with: %d\n", pid);
				return e;
	     		}  
		}
	}
	return NULL;
}

/* execute the kill command */
void killProcess(int pid){
	//send a SIGKILL to child process with pid
	kill(pid, SIGINT);
}

/* stop a process */
void stopProcess(int pid){
	kill (pid, SIGSTOP);
}

/**
 * Assign ownership of ther terminal to process group
 * pgrp, restoring its terminal state if provided.
 *
 * Before printing a new prompt, the shell should
 * invoke this function with its own process group
 * id (obtained on startup via getpgrp()) and a
 * sane terminal state (obtained on startup via
 * esh_sys_tty_init()).
 */
void give_terminal_to(pid_t pgrp, struct termios *pg_tty_state)
{
    esh_signal_block(SIGTTOU);
    int rc = tcsetpgrp(esh_sys_tty_getfd(), pgrp);
    if (rc == -1)
        esh_sys_fatal_error("tcsetpgrp: ");

    if (pg_tty_state)
        esh_sys_tty_restore(pg_tty_state);
    esh_signal_unblock(SIGTTOU);
}

/* initialize a job for the jobs list. Initialize fields of esh_pipeline once job has started */
void initializeJob(struct esh_pipeline* job, int pgrp,  enum job_status status){
	job->pgrp = pgrp;
	job->status = status;
}
