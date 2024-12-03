#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<sys/stat.h>
void main(int argc,char**argv)
{
int fd = open(argv[1],O_WRONLY,0664);
write(fd,argv[2],strlen(argv[2])+1);
close(fd);
int fd1 = open(argv[1],O_RDONLY,0664);
char s[55];
read(fd1,s,sizeof(s));
int n= atoi(s);
printf("%s\n",s);
printf("%d\n",n);
close(fd1);
}
