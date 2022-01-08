#include <linux/kernel.h>
#include <linux/syscalls.h>

asmlinkage int sys_mod_sion(int a, int b,int *c){
        int tmp;
        tmp = a%b;
        put_user(tmp,c);
        return 0;

}

SYSCALL_DEFINE3(mod_sion,int,a,int,b,int*,c)
{
        return sys_mod_sion(a,b,c);
}