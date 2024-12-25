#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>

#define READ_PATH    "/dev/spi_read_device"    // file path to read the received data
#define WRITE_PATH   "/dev/spi_write_device"   // file path to write the transmit data
void main()
{
	int fd1,fd2,ret;
	char buffer[32];

	fd1=open(READ_PATH,O_RDWR);   // opening the spi read driver
	fd2=open(WRITE_PATH,O_RDWR);  // opening the spi write driver

	while(1)
	{
		ret=read(fd1,buffer,sizeof(buffer)); 
		if(ret)
		{
			write(fd2,buffer,strlen(buffer)+1);
			break;
		}


	}
}
