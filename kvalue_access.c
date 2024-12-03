#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<fcntl.h>
void main(int argc,char **argv)
{
	int i,j;
	char s[50],d[20];
	int fd=open(argv[1],O_WRONLY);
	strcpy(s,argv[2]);
	write(fd,s,sizeof(s));
	close(fd);
	fd=open(argv[1],O_RDONLY);
	read(fd,d,sizeof(d));
	printf("%s\n",d);
	close(fd);
	
}
