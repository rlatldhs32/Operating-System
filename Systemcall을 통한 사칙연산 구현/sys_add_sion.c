#include <linux/kernel.h>
#include <linux/syscalls.h>

asmlinkage int sys_add_sion(int a, int b,int* c){
        int tmp;
        tmp = a+ b;
        put_user(tmp,c);
        return 0;
}

SYSCALL_DEFINE3(add_sion,int,a,int,b,int*,c)
{
        return sys_add_sion(a,b,c);
}