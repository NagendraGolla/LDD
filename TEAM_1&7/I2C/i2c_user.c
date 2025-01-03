#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
int main() {
    int file;
    char *filename = "/dev/my_i2c_dev"; // Path to the character device
    char data[6];
    ssize_t bytes_read;
    short int x,y,z;
    float X,Y,Z;

    file = open(filename, O_RDONLY); // Open for writing
    if (file < 0) {
        perror("Failed to open the device");
        return 1;
    }
while(1){
    bytes_read = read(file, data, 6);
    if (bytes_read < 0) {
        perror("Failed to write to the device");
        close(file);
        return 1;
    }

    x=data[0]|((short int)data[1]<<8);
    X=(float)(x>>6)/256;
    y=data[2]|((short int)data[3]<<8);
    Y=(float)(y>>6)/256;
    z=data[4]|((short int)data[5]<<8);
    Z=(float)(z>>6)/256;

    printf("X=%-8fg Y=%-8fg Z=%-8fg\n",X,Y,Z);
    sleep(1);
}
close(file);
    return 0;
}
