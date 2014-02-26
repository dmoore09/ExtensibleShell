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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void catch_sigint(int sig, siginfo_t* info, void* context);
void catch_sigtstp(int sig, siginfo_t* info, void* context);
void catch_child(int sig, siginfo_t* info, void* context);
struct list_elem* findPID(struct list* plist, int pid);
/* executes jobs command */
void jobs(struct list* pipelineList);
/* executes kill command */
void killProcess(int jid);
/* exectues the stop command */
void stopProcess(int jid);
/* gives a process group terminal control  used in fg, starting new process */
void giveTerminalControl(int pid);
/* give a process group control of the terminal credit: project1 faq #4*/
void give_terminal_to(pid_t pgrp, struct termios *pg_tty_state);
/* initialize pipeline fields that can be defined once a process is started */
void initializeJob(struct esh_pipeline* job, int pgrp, enum job_status status);
/* update the status of a job  inpsired by Signals 5 demo catch_child*/
void updateStatus(int pid, int status);
/* finds job with specified jid */
struct esh_pipeline * findJID(int jid);
void printName(struct esh_pipeline* pipe);
/* set up pipes */
void runPipe(int numPipes, int counter, int * pipes);

void updateJID(void);

bool checkPlugin(struct esh_command* command);

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
		list_rbegin (&esh_plugin_list);
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
	//printf("caught sigint\n");	
	int status;
        int  pid;
	while ((pid = waitpid(-1, &status, WNOHANG|WCONTINUED|WUNTRACED)) > 0){
		//get process with the right pid and remove from process list
		struct list_elem *e = findPID(&jobs_list, pid);
		if (e){
			//printf("kill process sigint\n");
			list_remove(e);
			killProcess(pid);
			updateJID();
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
			stopProcess(pid);
			//printf("kill process sigtstp\n");
		}
	}
	
}


/* catch sigchld when one or more child processes change state 
   taken and modified from Signal5 example */
void catch_child(int sig, siginfo_t* info, void* context){
    //printf("catch Child \n");
    /* reap all children and/or report status change */
    int status;
    int  pid;
    while ((pid = waitpid(-1, &status, WNOHANG|WCONTINUED|WUNTRACED)) > 0){
	updateStatus(pid, status);
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
	struct esh_pipeline *pipe1 = list_entry(element1, struct esh_pipeline, elem);
	

	
	struct list_elem* e;
	bool start = true;
	//if piped is true then pipeline has more then one command
	bool piped = false;
	int cmdCounter = 0;
	//number of pipes needed
	int pipeNum = (list_size(&pipe1->commands) - 1);
	int fd1[2];
	int fd2[2];
	if (pipeNum > 1){
		piped = true;
		//create an array of pipe s
	}
	
	//loop through all commands in pipeline
	//NOTE: basic commands can be piped but will only be the first command!
	for (e = list_begin (&(pipe1->commands)); e != list_end (&(pipe1->commands)); e = list_next (e)){
		
		//get the first command
		struct esh_command *firstCommand = list_entry(e, struct esh_command, elem);

		if (piped && (cmdCounter != pipeNum)){
			pipe(fd1);
			pipe(fd2);
		}

		//check for plugins
		if (checkPlugin(firstCommand)){
			continue;
		}
		//get string of first command test
		else if (strcmp(firstCommand->argv[0], "jobs") == 0){
			//print out all jobs
			jobs(&jobs_list);
		}
		else if (strcmp(firstCommand->argv[0], "fg") == 0){
			int fgJID = atoi(firstCommand->argv[1]);
			struct esh_pipeline* fgJob = findJID(fgJID);
	
			if (fgJob){
		
				//if job is in stopped send it SIGCONT
				if (fgJob->status == STOPPED){
					kill(fgJob->pgrp, SIGCONT);
				}
				fgJob->status = FOREGROUND;
		
					

				give_terminal_to(fgJob->pgrp, shell_state);

				//give fg status and wait
				int status;
				int pidT;
				//print out the name
				printName(fgJob);
				printf("\n");
				if((pidT = waitpid(-1, &status, WUNTRACED)) > -1){		
					give_terminal_to(getpgrp(), shell_state);
					updateStatus(pidT, status);	
				}		
			}
		}
		else if (strcmp(firstCommand->argv[0], "bg") == 0){
			int bgJID = atoi(firstCommand->argv[1]);
			struct esh_pipeline* bgJob = findJID(bgJID);
			kill(bgJob->pgrp, SIGCONT);
		}
		//TODO Does not work when a fg job is stopped
		else if (strcmp(firstCommand->argv[0], "kill") == 0){
			int killJid = atoi(firstCommand->argv[1]);
			killProcess(killJid);
		}
		else if (strcmp(firstCommand->argv[0], "stop") == 0){
			//TODO save terminal State! FAQ #12
			int stopJid = atoi(firstCommand->argv[1]);
			stopProcess(stopJid);
		}
		else if(strcmp(firstCommand->argv[0], "logout") == 0){
			printf("logging out of shell\n");
			return 0;
		}
		//user wants to start a program
		else{	
			//printf("user wants to start a program\n");
			 //you are in the child
			int pid = fork();
			if(pid == 0){
		
				if (start){
					//make process leader of its own process group
					start = false;
					setpgid(pid, pid);
				}
				else{
					//add to process group
					setpgid(pid, pipe1->pgrp);
				}
				
				//io redirection output
				if (firstCommand->iored_output){
					printf("in here\n");
					int newOut;
					//append
					if (firstCommand->append_to_output){
						newOut = open(firstCommand->iored_output, O_WRONLY|O_CREAT|O_TRUNC, 0600);	
					}
					//dont append
					else{
						newOut = open(firstCommand->iored_output, O_RDWR|O_CREAT|O_APPEND, 0600);	
					}
					dup2(newOut, fileno(stdout));
					close(newOut);
				}
				//io redirection input
				if (firstCommand->iored_input){
					int newIn = open(firstCommand->iored_input, O_WRONLY | S_IRWXU);
					dup2(newIn, fileno(stdin));
					close(newIn);
				}
				
				//printf("comdCounter: %d\n", cmdCounter);			
				//piped and the first element
				if(piped && cmdCounter != 0){
					//connect out end of pipe to stdout
					close(fd1[1]);					
					dup2(fd1[0], 0);
					close(fd1[0]);
				}
				//piped and the last element
				else if(piped && cmdCounter != pipeNum){
					close(fd2[0]);
					dup2(fd2[1], 1);
					close(fd2[1]);
				}
				
				int ret = 0;
				ret = execvp(firstCommand->argv[0],firstCommand->argv);	
				if(ret!=0){     
				  printf("Execution failed\n");
				  exit(0);
				}
			}
			//in the parent
			else {			
				//piped and the first element
				if(piped && cmdCounter != 0){
					//connect out end of pipe to stdout
					close(fd1[1]);
					close(fd1[0]);
				}
				//piped and the last element
				if(piped && cmdCounter == pipeNum){
					close(fd1[1]);
					close(fd1[0]);
					close(fd2[1]);
					close(fd2[0]);
				}
				//piped and not the first element or last
				if (piped && (cmdCounter != pipeNum)){
					fd1[1] = fd2[1];
					fd1[0] = fd2[0];
				}
				cmdCounter++;
					

				//add new job to jobs list
				if (!pipe1->bg_job && start){
					start = false;	
					initializeJob(pipe1, pid, FOREGROUND);
				}
				else if (start){
					start = false;
					initializeJob(pipe1, pid, BACKGROUND);
				}
				list_remove(&(pipe1->elem));
				
				//initialize child process pid
				//printf("child pid: %d\n", pid);			
				firstCommand->pid = pid;

				//double check, set child into process group 
				//TODO first process in pipe olny!!!
				setpgid(pid, pipe1->pgrp);

				
		
				//block signal when adding
				esh_signal_block(SIGCHLD);
				list_push_back(&jobs_list, &(pipe1->elem));
				esh_signal_unblock(SIGCHLD);
				pipe1->jid = list_size(&(jobs_list));

				if (!pipe1->bg_job){
					give_terminal_to(pid, shell_state);

					//wait for new child to finish TODO only if foreground
					int status;
					int pidT;
					if((pidT = waitpid(-1, &status, WUNTRACED)) > -1){		
						give_terminal_to(getpgrp(), shell_state);
						//printf("shell has access\n");
						updateStatus(pidT, status);	
					}	
				}
				//bg job
				else{
					printf("[%d] %d\n", pipe1->jid, pid);
				}
				//increment command counter
				
			}
		
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
		printf("+  Stopped                 ");
	     }
	     else{
		printf("-  Running                 ");
	     }
	     //print command name
	     printf("(");
	     printName(job);
	     printf(")\n");
	     jobNum++;
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
				//printf("Found process with: %d\n", pid);
				return e;
	     		}  
		}
	}
	return NULL;
}

//whenever a job is removed update the jid or remaining processes
void updateJID(){
	int jid = 1;
	struct list_elem *e;
	for (e = list_begin (&(jobs_list)); e != list_end (&(jobs_list)); e = list_next (e))
        {	
            	 struct esh_pipeline *pipe = list_entry (e, struct esh_pipeline, elem);
		 pipe->jid = jid;
		 jid++; 
	}
}

/* execute the kill command */
void killProcess(int jid){
	//send a SIGKILL to child process with pid
	struct esh_pipeline *pipe = findJID(jid);
	if (pipe){
		kill (pipe->pgrp, SIGINT);
	}	
	
}

/* stop a process */
void stopProcess(int jid){
	struct esh_pipeline *pipe = findJID(jid);	
	kill (pipe->pgrp, SIGSTOP);
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

/* update the status of a job  inpsired by Signals 5 demo catch_child*/
void updateStatus(int pid, int status){
	//get process
	struct list_elem *e = findPID(&jobs_list, pid);
	

      //process exited
      if( WIFEXITED(status)) {
          //printf("Received sigal that child process (%d) terminated 2. \n", pid);
	  //remove process with pid from list
	  if (e){	
		give_terminal_to(getpid(), shell_state);
		list_remove(e);
		updateJID();
	   }
	   else{
		printf("ERROR: could not find child on list EXITED");
	   }	
      }
      //process stopped
      if (WIFSTOPPED(status)) { 
          //printf("Received signal that child process (%d) stopped. \n", pid);
	   //set process with pid as stopped
	   if (e){
		struct esh_pipeline *pipe = list_entry (e, struct esh_pipeline, elem);
		pipe->status = STOPPED;
		//print out 
		printf("\n");
		printf("[%d]+  Stopped                 (", pipe->jid);
		//print out name
		printName(pipe);
		printf(")\n");
		

	   }
	   else{
		printf("ERROR: could not find child on list STOPPED");
	   }		  
      }
      if (WIFCONTINUED(status)) {
          //printf("Received signal that child process (%d) continued. \n", pid);
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
         // printf("Received signal that child process (%d) received signal [%d] \n", 
          //        pid, WTERMSIG(status));
	   if (e){
		list_remove(e);
		updateJID();
	   }
	   else{
		printf("ERROR: could not find child on list SIGNALED");
	   }	
      }
}

/* finds job with specified jid */
struct esh_pipeline * findJID(int searchJid){
	int jid = 1;
	struct list_elem *e;
	for (e = list_begin (&(jobs_list)); e != list_end (&(jobs_list)); e = list_next (e))
        {	
            	 struct esh_pipeline *pipe = list_entry (e, struct esh_pipeline, elem);
		 if (searchJid == jid){
			return pipe;
		 }
		 jid++; 
	}
	printf("No job with specified JID\n");
	return NULL;
}

/* print out name of the pipeline */
void printName(struct esh_pipeline* pipe){
	struct list_elem *e;
	int counter = 0;
	for (e = list_begin (&(pipe->commands)); e != list_end (&(pipe->commands)); e = list_next (e)){
			struct esh_command *command = list_entry(e, struct esh_command, elem);
			while (command->argv[counter] != NULL){
				printf("%s", command->argv[counter]);
				if (command->argv[counter + 1] != NULL){
					printf(" ");
				}
				counter++;
			}	
	}
}

/* set up pipes for a process */
void runPipe(int numPipes, int counter, int* pipes){
       //0 is read, 1 is write
       //directs the pipes (except for last)
	if (counter < numPipes){
      		 dup2(pipes[(2*counter)+1],(2*counter)+2);
	}

       //pipes final command to stdout
       if (counter == numPipes){
       		dup2(pipes[(2*(numPipes-1))+1],1);
	}
        close(pipes[2*counter]);
        close(pipes[(2*counter)+1]);
}

bool checkPlugin(struct esh_command* command){
	struct list_elem * e = list_begin(&esh_plugin_list);

        for (; e != list_end(&esh_plugin_list); e = list_next(e)) {
        	struct esh_plugin *plugin = list_entry(e, struct esh_plugin, elem);
		if (plugin->process_builtin){
			if (plugin->process_builtin(command)){
				return true;
			}
		}

	}
	return false;
}
