#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]){
//     int i;
//    pid_t childpid;
	for(int i = 1; i < argc; i++){
		fprintf(stderr,"arg%d: %s\n",i,argv[i]);
	}
	char* dir = (char*)malloc(sizeof(char)*100);
	char* cmd = (char*)malloc(sizeof(char)*20);
//        if(fork()==0){
                int ret = 0;
                char* path = (char*)malloc(sizeof(char)*100);
		strcat(path,"/bin/");
                strcat(path,argv[1]);
                ret = execv(path,&argv[1]);
                if(ret!=0){
                        printf("Execution failed");
                }
//        }

	//adds bin commands to check
	int cmdsize = 0;
	char** cmds = (char**)malloc(sizeof(char*)*100);	
	int i = 0;
	for(i=0;i<100;i++){
		cmds[i] = (char*)malloc(sizeof(char)*10);
	}
	strcpy(cmds[0],"ls");
	cmdsize = 1;

	//checks commands
	int j = 0;
	for(j = 0;j < cmdsize;j++){
		if(strcmp(cmds[j],argv[0])){
			fprintf(stderr,"This is a basic command");
			//call execvp
		}
	}

	
/**
	strcat(dir,"/bin/");
	strcat(dir,argv[1]);
	strcat(cmd,argv[1]);
  	execl(dir,cmd,NULL);
*/


    /**
    childpid = fork();
    if(childpid == -1)
    {
        perror("Failed to Fork");
        return 1;
    }

    if(childpid == 0)
        printf("You are in Child: %ld\n", (long)getpid());
    else
        printf("You are in Parent: %ld\n", (long)getpid());
    return 0;
*/
    return 0;
}
/**
void elseifstatement(char *av[]){
	if(false){
	}else{
		//you are in the child
		if(fork()==0){
			int ret = 0;
			char* path = "/bin/";
			strcat(path,firstCommand->argv[0]);
			ret = execv(path,firstCommand->argv);
			if(ret!=0){
				printf("Execution failed");
			}
		}
	}
}
*/
