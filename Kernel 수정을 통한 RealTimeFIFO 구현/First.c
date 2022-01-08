#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>

#define SMALL 10000;
#define MIDDLE 50000;
#define BIG 100000;

pid_t pid[21];

void Calc(int n){
	int N;
	if(0<=n && n<7) { N=SMALL;}
	else if (7<=n && n<14){ N=MIDDLE;}
	else if (14<=n && n<21){ N=BIG;}
	else
	{ printf("\nplease put 1~3 number.\n"); return;}
	
	for(int i=0;i<N;i++){
		for(int j=0;j<N;j++){
			int A = 10 + 10;
		}
	}
}
int main(){
	int status;
	for(int i=0;i<21;i++)
	{
		pid[i]=fork();

		if(pid[i]<0){
			printf("error happnened.\n");
			exit(1);
		}	
		else if( pid[i]>0){
			printf("%d program begins \n",pid[i]);
		}
		if(pid[i]==0){
			Calc(i);		
			exit(0);		
		}
	}
	for(int i=0;i<21;i++){
		pid_t pid = wait(&status);
		printf("%d process ends \n",pid);
	}
	printf("---------- All processes end ----------\n");
	return 0;
}