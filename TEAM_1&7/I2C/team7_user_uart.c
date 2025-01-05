/*
 * Program to read data from UART, process it, and use the data to interact with an I2C device.
 * - Opens UART and I2C devices.
 * - Reads a command over UART and processes it.
 * - If command is "senddata", reads data from an I2C device and processes it.
 * - Sends the processed data back over UART.
 */

#include <stdio.h>      // Include standard I/O functions for printing and file handling
#include <fcntl.h>      // Include file control options (open flags)
#include <unistd.h>     // Include POSIX functions for I/O operations (read, write, close, etc.)
#include <termios.h>    // Include termios for serial port configuration
#include <string.h>     // Include string handling functions (strcmp, strlen, etc.)
#include <errno.h>      // Include error handling (perror)
#include <stdlib.h>     // Include standard library functions (exit, malloc, etc.)

// Define device paths for UART and I2C
#define UART_DEVICE    "/dev/serial0"  // Default UART device (e.g., Raspberry Pi UART port)
#define PATH           "/dev/i2c_rx"   // Path to I2C device driver (replace with actual path)

int main()
{
    // Declare file descriptors for UART and I2C
    int uart_fd, i2c_fd;
    
    // Declare a struct to hold the UART configuration settings
    struct termios options;
    
    // Declare buffers for storing transmitted and received data
    char tx_buffer[256];    // Transmission buffer for sending data over UART
    char rx_buffer[256];    // Reception buffer for receiving data from UART
    
    // Declare variables to track the number of bytes read/written
    int bytes_written, bytes_read;
    
    // Declare a counter to keep track of the number of bytes read from UART
    int count = 0;
    
    // Declare variables to store raw data from the I2C device (e.g., sensor values)
    int x, y, z;
    
    // Declare variables to store processed/scaled data from the I2C sensor
    float X, Y, Z;

    // Open the UART device for reading and writing, check for errors
    uart_fd = open(UART_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_fd == -1)  // Check if UART was opened successfully
    {
        perror("Failed to open UART");  // Print error message if opening fails
        return 1;  // Exit the program with an error code
    }

    // Open the I2C driver device for reading, check for errors
    i2c_fd = open(PATH, O_RDONLY);
    if (i2c_fd == -1)  // Check if I2C device was opened successfully
    {
        perror("Failed to open I2C driver");  // Print error message if opening fails
        return 1;  // Exit the program with an error code
    }

    // Get the current UART configuration and store it in the 'options' structure
    tcgetattr(uart_fd, &options);  

    // Configure UART settings (baud rate, data bits, etc.)
    options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;  // Set 9600 baud, 8 data bits, enable receiver
    options.c_iflag = IGNPAR;  // Ignore parity errors during reception
    options.c_oflag = 0;       // No output processing
    options.c_lflag = 0;       // Non-canonical mode (no buffering or special processing)
    
    // Clear any existing input data from the UART buffer
    tcflush(uart_fd, TCIFLUSH);     

    // Apply the updated UART configuration settings
    tcsetattr(uart_fd, TCSANOW, &options);

    // Start reading data from UART until newline character is encountered
    while(1)
    {
        // Read one byte at a time from the UART into the reception buffer
        bytes_read = read(uart_fd, rx_buffer + count, 1);  
        
        // If a byte is successfully read, increment the count
        if (bytes_read > 0)
        {
            count += bytes_read;  // Increment byte count
        }

        // Check if a newline character is encountered
        if (rx_buffer[count - 1] == '\n')  
            break;  // Exit the loop when newline is found
    }

    // Null terminate the string by replacing the newline character with null byte
    rx_buffer[count - 1] = 0;

    // Print the complete string received from UART
    printf("\nComplete string is %s\n", rx_buffer);

    // If the received command is "senddata", proceed to read data from I2C
    if (strcmp("senddata", rx_buffer) == 0)
    {
        // Read 6 bytes of data (X, Y, Z) from the I2C device
        bytes_read = read(i2c_fd, tx_buffer, 6);

        // Convert the 2-byte raw data for each axis into 16-bit integers
        x = tx_buffer[0] | ((short int)tx_buffer[1] << 8);  // Combine bytes for X axis
        X = (float)(x >> 6) / 256;  // Scale the 10-bit value for X axis
        
        y = tx_buffer[2] | ((short int)tx_buffer[3] << 8);  // Combine bytes for Y axis
        Y = (float)(y >> 6) / 256;  // Scale the 10-bit value for Y axis
        
        z = tx_buffer[4] | ((short int)tx_buffer[5] << 8);  // Combine bytes for Z axis
        Z = (float)(z >> 6) / 256;  // Scale the 10-bit value for Z axis

        // Print both raw and scaled data for X, Y, Z axes
        printf("X=%-8dg Y=%-8dg Z=%-8dg\n", x, y, z);  
        printf("X=%-8fg Y=%-8fg Z=%-8fg\n", X, Y, Z);  

        // If I2C read fails (no data), print an error and close devices
        if (bytes_read == 0)
        {
            perror("Failed to read data from ADXL");  // Print error message
            close(uart_fd);   // Close UART device
            close(i2c_fd);    // Close I2C device
            return 0;         // Exit the program
        }
    }
    else
    {
        // If the command is not "senddata", print an error message and exit
        printf("Received invalid command\n");
        close(uart_fd);  // Close UART device
        close(i2c_fd);   // Close I2C device
        return 0;        // Exit the program
    }

    // Add a newline character to the end of the message to be sent
    tx_buffer[6] = '\n';  
    sleep(1);  // Optional: Sleep for 1 second

    // Write the processed data (tx_buffer) to UART for sending
    bytes_written = write(uart_fd, tx_buffer, strlen(tx_buffer) + 1);  
    if (bytes_written < 0)  // Check if the write operation was successful
    {
        perror("Failed to write to UART driver");  // Print error if write fails
        close(uart_fd);  // Close UART device
        close(i2c_fd);   // Close I2C device
        return 1;         // Exit the program with an error code
    }

    // Print the message sent over UART
    printf("Sent: %s\n", tx_buffer);

    // Close both UART and I2C devices after communication is complete
    close(uart_fd);  
    close(i2c_fd);

    return 0;  // Return 0 to indicate successful program execution
}

