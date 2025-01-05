#include <stdio.h>    // Standard input/output functions
#include <fcntl.h>    // File control options
#include <unistd.h>   // POSIX operating system API
#include <string.h>   // String manipulation (not used in this simple version)
#include <stdlib.h>   // General utilities (atoi)

#define FILE_PATH "/dev/pm_driver" // Path to the device file

int main(int argc, char **argv)
{
    // Pseudo-code:
    // 1. Check command-line arguments.
    // 2. Open the device file.
    // 3. Loop:
    //    a. Read data from the device.
    //    b. Process data (combine bytes, scale).
    //    c. Print data.
    //    d. Sleep.
    // 4. Close the device file.

    if (argc != 2) { // Check if the number of command-line arguments is correct
        printf("Usage:./a.out sleep_value\n"); // Print usage instructions
        return 0; // Exit the program
    }

    int fd;         // File descriptor
    char data[6];   // Data buffer
    ssize_t bytes_read; // Number of bytes read
    short int x, y, z;   // Raw accelerometer data
    float X, Y, Z;       // Scaled accelerometer data

    fd = open(FILE_PATH, O_RDONLY); // Open the device file in read-only mode
    if (fd < 0) { // Check if the file was opened successfully
        perror("Failed to open the device"); // Print an error message
        return 1; // Exit with an error code
    }

    while (1) { // Infinite loop
        // Pseudo-code:
        // 1. Read data.
        // 2. Combine bytes.
        // 3. Scale.
        // 4. Print.
        // 5. Sleep.

        bytes_read = read(fd, data, 6); // Read 6 bytes from the device
        if (bytes_read < 0) { // Check if read was successful
            perror("Failed to write to the device"); // Print error message (incorrect message, should say "read")
            close(fd); // Close the file descriptor
            return 1; // Exit with an error code
        }

        // Pseudo-code: Combine two bytes into a short int for each axis.
        x = data[0] | ((short int)data[1] << 8); 
        X = (float)(x >> 6) / 256; 
        y = data[2] | ((short int)data[3] << 8); 
        Y = (float)(y >> 6) / 256; 
        z = data[4] | ((short int)data[5] << 8);
        Z = (float)(z >> 6) / 256; 

        // Pseudo-code: Print the raw and scaled data.
        printf("X=%-8dg Y=%-8dg Z=%-8dg\n", x, y, z); // Print raw data
        printf("X=%-8fg Y=%-8fg Z=%-8fg\n", X, Y, Z); // Print scaled data

        // Pseudo-code: Sleep.
        sleep(atoi(argv[1])); // Sleep for the specified number of seconds
    } // End of while loop

    close(fd); // Close the file descriptor (this line is unreachable due to the infinite loop)
    return 0; // Exit the program
}
