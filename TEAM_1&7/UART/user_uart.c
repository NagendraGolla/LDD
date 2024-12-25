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
	write(fd,argv[1],strlen(argv[1])+1);
	printf("data sent : %s\n",argv[1]);
	char c;
	read(fd,&c,sizeof(c));
	if(strcmp(&c,'1')==0)
		printf("LED TURNED ON SUCESSFULLY\n");
	else if(strcmp(&c,'0')==0)
		printf("LED TURNED OFF SUCESSFULLY\n");
	else 
		printf("INVALID DATA\n");

}
