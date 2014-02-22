#include <time.h>
#include <stdbool.h>
#include "../esh.h"
#include "../esh-sys-utils.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
static bool
init_plugin(struct esh_shell * shell){
	printf("Plugin 'random' initialized\n");
	return true;
}

static bool 
random_builtin(struct esh_command *cmd)
{
	//checks to make sure the right command was entered
	if(strcmp(cmd->argv[0], "random"))
		return false;
	srand(time(NULL));
	int out;
	//no parameters, choose random number 1 - 100
	if(!strcmp(cmd->argv[1], NULL)){
		out = rand() % 100 + 1;
	}else if(!strcmp(cmd->argv[2],NULL)){ //1 param, 0 - argv[2]
		int max = atoi(cmd->argv[1]);
		out = rand() % max + 1;
	}else{ //2 param, argv[1] - argv[2]
		int max = atoi(cmd->argv[2]);
		int min = atoi(amd->argv[1]);
		int dif = max - min + 1;
		out = rand() % dif + min;
	}
	printf("Random: %1",out);
	return true;
	

}

struct esh_plugin esh_module = {
   .rank = 10;
   .init = init_plugin,
   .process_builtin = random_builtin
};

