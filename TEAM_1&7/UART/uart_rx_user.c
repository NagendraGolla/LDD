#include<stdio.h>
#include<string.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>

#define PATH "/dev/uart_driver"

void main(int argc,char **argv)
{
	int fd=open(PATH,O_RDWR);
	if(fd<0){
		perror("open");
		return;
	}
	char c[55];
	read(fd,c,sizeof(c));
	if(strcmp(c,"ON")==0)
		write(fd,"1",2);
	else if(strcmp(c,"0FF")==0)
		write(fd,"0",2);
	else 
		write(fd,"INVALID DATA\n",14);

}
