#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>

int main()
{
        int N1,N2;
        char operator;
        int result=0;
        printf(">> ");
        scanf("%d%c%d",&N1,&operator,&N2);
        switch(operator)
        {
                case '+' : syscall(442,N1,N2,&result); break;
                case '-' : syscall(443,N1,N2,&result); break;
                case '*' : syscall(444,N1,N2,&result); break;
                case '%' : syscall(445,N1,N2,&result); break;
                default : printf("wrong operator input. use +-*%% only.\nDo not use blank in operation\n");
                          return 0;
        }
        printf("result : %d\n",result);
        return 0;
}          
