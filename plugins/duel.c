#include<stdlib.h>
#include<stdio.h>
#include<time.h>
#include<stdbool.h>
#include<unistd.h>
#include<sys/types.h>
#include"esh.h"
#include"../esh-sys-utils.h"
#include<signal.h>

static bool
init_plugin(struct esh_shell *shell){
	printf("Plugin 'duel' loaded...\n");
	return true;
}

static bool
duel_builtin(struct esh_command *cmd){
	if(strcmp(cmd->argv[0], "duel"))
		return false;


	printf("Welcome to DUEL\n\n Before time runs out, press\n\t1\tshield\n\t2\tattack\n\t3\tcharge\n");
	printf("\nRULES:\nEach player will have 10 health for the entire game. Each round continues until one person gets hit. If both players attack at the same time, both players will be damaged. Each round will have the same amount of time to make a decision, but time will decrease as the round goes on.\n\nCharging\n\t1 charge: 2 damage\n\t2 charges: 3 damage\n\t3 charges: 5 damage and cannot be blocked\n\tnote:Charges will reset to 0 upon being hit\n");
	srand(time(NULL));
	int cpu = 10;
	int cpucharge = 0;
	int player = 10;
	int playercharge = 0;
	int tm = 3;
	int choice;
	int comp = 0;
	bool fin = false;
	//game check
	while(cpu>0 && player>0 && !fin){
		printf("Please make your choice\n");
		scanf("%d",&choice);
		comp = rand() % 3 + 1;
		if(choice == 2 && comp == 2){
			cpu = cpu - damage(playercharge);
			printf("You dealt %d damage\n",damage(playercharge);
			player = player - damage(cpucharge);
			printf("You were damaged %d\n",damage(cpucharge);
			reset();
			status();
			prompt();
			continue;
		}
		if(choice == 3){
			playercharge++;
		}
		if(comp == 3){
			cpucharge++;
		}
		if(choice == 2){
			if(comp == 1){
				if(playercharge > 3){
					cpu = cpu - damage(playercharge);
					printf("You dealt %d damage\n",damage(playercharge);
					reset();
				}else{
					printf("Your attack was blocked\n");
				}
			}else{
				cpu = cpu - damage(playercharge);
				printf("You dealt %d damage\n",damage(playercharge);
				reset();
			}
		}
		if(comp == 2){
			if(choice == 1){
                                if(cpucharge > 3){
                                        player = player - damage(cpucharge);
                                        printf("You were damaged %d\n",damage(cpucharge);
                                        reset();
                                }else{
                                        printf("You blocked the attack\n");
                                }
                        }else{
                                cpu = cpu - damage(playercharge);
                                printf("You were damaged %d\n",damage(cpucharge);
                	        reset();
        	        }
		}
		status();
		prompt();
	}
	return true;
}

int getDamage(int ch){
	if(ch == 0){
		return 1;
	}else if(ch == 1){
		return 2;
	}else if(ch == 2){
		return 3;
	}
	return 5;
}

void reset(){
	playercharge = 0;
	cpucharge = 0;
}

void status(){
	printf("Player:\n\tHealth: %d\n\tCharge: %d\n",player,playercharge);
	printf("Opponent:\n\tHealth: %d\n\tCharge: %d\n",cpu,cpucharge);
}

void prompt(){
	char* ch;
	ch = (char*)malloc(sizeof(char)*10);
	printf("Do you want to continue? [y/n]: ");
	scanf("%s",&ch);
	if(!strcmp(ch,"n")){
		fin = false;
	}else if(!strcmp(ch,"n")){
		fin = true;
	}else{
		printf("Your choice was not valid\n");
		prompt();
	}
}

struct esh_plugin esh_module = {
   .rank = 5,
   .init = init_plugin,
   .process_builtin = duel_builtin
};
