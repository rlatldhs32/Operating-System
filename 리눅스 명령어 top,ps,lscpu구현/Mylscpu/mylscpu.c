#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

int Kbyte_to_Mbyte(int a)
{
	return a/1024;
}


int  return_size(char* path)
{
	FILE * fd;
	int size=0;
	char buf[1000];
	char tmp[100];
	memset(tmp,0,100);
	
	strcpy(tmp,path);
	strcat(tmp,"/size");
	if((fd=fopen(tmp,"r"))  ==NULL)
        {
           	fprintf(stderr,"%s Open Error!",tmp);
               	return -1;
         }
   	   fgets(buf,10,fd);
           size = atoi(buf);
	  fclose(fd);		
	
	return size; 
}
int main()
{

	char buf[1000];
	FILE* fd;
	if( (fd =fopen("/proc/cpuinfo","r"))==NULL)
	{
           	fprintf(stderr,"/proc/cpuinfo Open Error!");
		return -1;
	}
	int i=0;
	
	while(i<10)
	{
		fgets(buf,70,fd);
		if(i==1||i==2 || i==3 ||i==4|| i==7){
			buf[0]+='A'-'a';
			printf("%s",buf);
		}
		if(i==7)
			break;
		i++;
	}
	
	int L1i =0;
	int L1d =0;
	int L2 =0;
	int L3 =0;

	DIR	*dir_ptr = NULL;
	DIR	*dir_ptr2 = NULL;
	struct dirent *file =NULL;



	if((dir_ptr = opendir("/sys/devices/system/cpu"))==NULL)
	{
		fprintf(stderr,"cpu directory 정보 읽을 수 없습니다.\n");
		return -1;
	}

	while((file= readdir(dir_ptr)) !=NULL)
	{
		if( strstr(file->d_name,"cpu0")
		     ||strstr(file->d_name,"cpu1")
		      ||strstr(file->d_name,"cpu2")
		      ||strstr(file->d_name,"cpu3")
		      ||strstr(file->d_name,"cpu4")
		      ||strstr(file->d_name,"cpu5")
		      ||strstr(file->d_name,"cpu6")
		      ||strstr(file->d_name,"cpu7")
		      ||strstr(file->d_name,"cpu8")
		      ||strstr(file->d_name,"cpu9")
		      )
		{
			char tmp[100],tmp2[100],tmp3[100],tmp4[100];
			memset(tmp,0,100);
			memset(tmp2,0,100);
			memset(tmp3,0,100);
			memset(tmp4,0,100);

			strcpy(tmp,"/sys/devices/system/cpu/");
			strcat(tmp, file->d_name);
			
			strcpy(tmp2,tmp);
			strcpy(tmp3,tmp);
			strcpy(tmp4,tmp);

			strcat(tmp,"/cache/index0");
			strcat(tmp2,"/cache/index1");
			strcat(tmp3,"/cache/index2");
			strcat(tmp4,"/cache/index3");
			
			L1i += return_size(tmp);
			L1d += return_size(tmp2);
			L2 += return_size(tmp3);
			L3 += return_size(tmp4);
	  	}
	}
	closedir(dir_ptr);
	L3=Kbyte_to_Mbyte(L3);
	printf("L1i 		: %d	KiB\n",L1i);
	printf("L1d 		: %d	KiB\n",L1d);
	printf("L2		: %d	KiB\n",L2);
	printf("L3		: %d	MiB\n",L3);
	return 0;
}
