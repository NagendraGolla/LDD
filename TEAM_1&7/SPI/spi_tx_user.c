#include<stdio.h>
#include<string.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>

#define PATH "/dev/spi_bitbang"

void main(int argc,char **argv)
{
	int fd=open(PATH,O_RDWR);
	write(fd,argv[1],strlen(argv[1])+1);
//	sleep(2);
	char s[30];
	read(fd,s,sizeof(s));
	if(strcmp(s,"ON")==0){
	printf("LED Turned on sucessfully\n");
	}
	else if(strcmp(s,"OFF")==0){
	printf("LED Turned off sucessfully\n");
	}
	close(fd);
}
