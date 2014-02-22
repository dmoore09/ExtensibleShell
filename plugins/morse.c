#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include "../esh.h"
#include "../esh-sys-utils.h"

static bool
init_plugin(struct esh_shell *shell)
{
	printf("Plugin 'Morse' initialized...\n");
	return true;
}

static bool
morse_builtin(struct esh_command *cmd)
{
	char* out = (char*)malloc(sizeof(char)*100);
	if (strcmp(cmd->argv[0], "morse"))
		return false;
	
	//check the number of words
	int ct = 0;
	while(strcmp(cmd->argv[ct],NULL)){
		ct++;
	}

	int i = 0;
	int j = 0;
	int len = 0;
	for(i = 1; i < ct; i++){
		len = strlen(cmd->argv[i]);
		for(j = 0; j < len; j++){
			printf("%s",translate(cmd->argv[i][j], out));
		}
		printf(" ");
	}
}

char* translate(char l, char* ret){
	l = tolower(&l);
	if(l == 'a'){
		strcpy(ret, ".-");
	}else if(l == 'b'){
		strcpy(ret,"-...");
	}else if(l == 'c'){
		strcpy(ret,"-.-.");
	}else if(l == 'd'){
		strcpy(ret,"-..");
	}else if(l == 'e'){
		strcpy(ret,".");
	}else if(l == 'f'){
		strcpy(ret,"..-.");
	}else if(l == 'g'){
		strcpy(ret,"--.");
	}else if(l == 'h'){
		strcpy(ret,"....");
	}else if(l == 'i'){
		strcpy(ret,"..");
	}else if(l == 'j'){
		strcpy(ret,".---");
	}else if(l == 'k'){
		strcpy(ret,"-.-");
	}else if(l == 'l'){
		strcpy(ret,".-..");
	}else if(l == 'm'){
		strcpy(ret,"--");
	}else if(l == 'n'){
		strcpy(ret,"-.");
	}else if(l == 'o'){
		strcpy(ret,"---");
	}else if(l == 'p'){
		strcpy(ret,".--.");
	}else if(l == 'q'){
		strcpy(ret,"--.-");
	}else if(l == 'r'){
		strcpy(ret,".-.");
	}else if(l == 's'){
		strcpy(ret,"...");
	}else if(l == 't'){
		strcpy(ret,"-");
	}else if(l == 'u'){
		strcpy(ret,"..-");
	}else if(l == 'v'){
		strcpy(ret,"...-");
	}else if(l == 'w'){
		strcpy(ret,".--");
	}else if(l == 'x'){
		strcpy(ret,"-..-");
	}else if(l == 'y'){
		strcpy(ret,"-.--");
	}else if(l == 'z'){
		strcpy(ret,"--..");
	}else if(l == '1'){
		strcpy(ret,".----");
	}else if(l == '2'){
		strcpy(ret,"..---");
	}else if(l == '3'){
		strcpy(ret,"...--");
	}else if(l == '4'){
		strcpy(ret,"....-");
	}else if(l == '5'){
		strcpy(ret,".....");
	}else if(l == '6'){
		strcpy(ret,"-....");
	}else if(l == '7'){
		strcpy(ret,"--...");
	}else if(l == '8'){
		strcpy(ret,"---..");
	}else if(l == '9'){
		strcpy(ret,"----.");
	}else if(l == '0'){
		strcpy(ret,"-----");
	}else{
		strcpy(ret,"");
	}
	return ret;
}
