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
	strcat(dir,"/bin/");
	strcat(dir,argv[1]);
	strcat(cmd,argv[1]);
  	execl(dir,cmd,NULL);
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
