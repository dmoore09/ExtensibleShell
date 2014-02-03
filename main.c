#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"

int main(){
	char* input = (char*)malloc(sizeof(char)*100);
	while(true){
		printf("Hello: ");
		fgets(input,100,stdin);

	}
}
