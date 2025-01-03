#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
void main()
{
	int fd;
	fd=open("/dev/spi_bitbang",O_RDWR);   // opening the driver
	perror("open");
        if(fd<0)
		return;
	char arr[32];

	read(fd,arr,sizeof(arr));
	printf("Received data is %s\n",arr);
}
