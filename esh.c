/*
 * esh - the 'pluggable' shell.
 *
 * Developed by Godmar Back for CS 3214 Fall 2009
 * Virginia Tech.
 */
#include <stdio.h>
#include <readline/readline.h>
#include <unistd.h>
#include <signal.h>
#include "esh-sys-utils.h"
#include "esh.h"
#include "list.h"

void catch_sigint(int sig, siginfo_t* info, void* context);
void catch_sigtstp(int sig, siginfo_t* info, void* context);
void catch_child(int sig, siginfo_t* info, void* context);

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
}

/* catch sigtstp (^x)  stop a foreground process */
void catch_sigtstp(int sig, siginfo_t* info, void* context){
	printf("caught sigtstp\n");
}

/* catch sigchld when one or more child processes change state 
   taken and modified from Signal5 example 
   TODO need to implement the process list, this method will change
   the state of process */
void catch_child(int sig, siginfo_t* info, void* context){
    
    /* reap all children and/or report status change */
    int status;
    int  pid;
    while ((pid = waitpid(-1, &status, WNOHANG|WCONTINUED|WUNTRACED)) > 0){
      //process exited
      if( WIFEXITED(status)) {
          printf("Received sigal that child process (%d) terminated. \n", pid);
	  //TODO need to remove process with pid from list
      }
      //process stopped
      if (WIFSTOPPED(status)) { 
          printf("Received signal that child process (%d) stopped. \n", pid);
	  //TODO need to set process with pid as stopped
      }
      if (WIFCONTINUED(status)) {
          printf("Received signal that child process (%d) continued. \n", pid);
	  //TODO need to set process with pid as continued
      }
      if( WIFSIGNALED(status)) {
          printf("Received signal that child process (%d) received signal [%d] \n", 
                  pid, WTERMSIG(status));
	  //TODO not sure....
          done++;
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

        esh_command_line_print(cline);
        esh_command_line_free(cline);
    }
    return 0;
}
