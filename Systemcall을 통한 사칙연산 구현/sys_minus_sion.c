#include <linux/kernel.h>
#include <linux/syscalls.h>

asmlinkage int sys_minus_sion(int a, int b,int* c){
        int tmp;
        tmp = a-b;
        put_user(tmp,c);
        return 0;

}

SYSCALL_DEFINE3(minus_sion,int,a,int,b,int*,c)
{
        return sys_minus_sion(a,b,c);
}