/*Pseudo Code:
1. Open the UART device (/dev/serial0) for reading and writing.
2. Set the UART configuration (9600 baud rate, 8 data bits, no parity).
3. Send the "senddata\n" message through UART.
4. Read the data received through UART, waiting for a newline character to indicate the end of the response.
5. Extract and process the received data into X, Y, and Z coordinates.
6. Format the data into a string with appropriate units (e.g., 'g' for gravity).
7. Open the I2C device (/dev/lcd_i2c) and write the formatted data string to it.
8. Close all file descriptors and exit.
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <errno.h>

#define UART_DEVICE "/dev/serial0" // Default UART device path
#define I2C_DEVICE "/dev/lcd_i2c"  // I2C device path (LCD display)

// Main function
int main() {
    int uart_fd;            // File descriptor for UART device
    struct termios options; // Structure for UART configuration
    char tx_buffer[] = "senddata\n";  // Data to send to the UART device
    char rx_buffer[256];    // Buffer to hold received data
    char data[20];          // Buffer to store formatted data to be written to the I2C device
    int bytes_written = 0, bytes_read = 0, x = 0, y = 0, z = 0; // Variables for data transmission and received values
    float X = 0, Y = 0, Z = 0; // Processed float values for X, Y, Z

    // Step 1: Open UART device for reading and writing, with no control terminal and non-blocking mode
    uart_fd = open(UART_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_fd == -1) {  // Check if UART device opened successfully
        perror("Failed to open UART");
        return 1;  // Exit on failure
    }

    // Step 2: Configure UART settings
    tcgetattr(uart_fd, &options); // Get the current UART configuration
    options.c_cflag = B9600 | CS8 | CLOCAL | CREAD; // Set baud rate to 9600, 8 data bits, enable receiver
    options.c_iflag = IGNPAR;                        // Ignore parity errors
    options.c_oflag = 0;                             // Disable output processing
    options.c_lflag = 0;                             // Set to non-canonical mode (input is not line-buffered)
    tcflush(uart_fd, TCIFLUSH); // Flush any unread data in UART buffer
    tcsetattr(uart_fd, TCSANOW, &options); // Apply the new UART settings immediately

    // Step 3: Send the data "senddata\n" through UART
    bytes_written = write(uart_fd, tx_buffer, strlen(tx_buffer)); // Write to UART
    if (bytes_written < 0) {  // Check if writing to UART was successful
        perror("Failed to write to UART");
        close(uart_fd);  // Close UART file descriptor
        return 1;  // Exit on failure
    }
    printf("Sent: %s", tx_buffer);  // Print the sent data

    // Step 4: Read data from UART response (loop until a newline character is received)
    usleep(100000); // Introduce a small delay to allow data to loop back
    int count = 0;  // Counter for number of bytes read
    while (1) {
        bytes_read = read(uart_fd, rx_buffer + count, 1); // Read one byte at a time
        if (bytes_read > 0) {
            count += bytes_read;  // Increment count for each byte read
        }
        if (rx_buffer[count - 1] == '\0') { // Break loop when newline character is received
            break;
        }
    }

    // Step 5: Close UART device after reading
    close(uart_fd);

    // Step 6: Process the received data into X, Y, Z values (assuming the data is raw sensor data)
    x = rx_buffer[0] | ((short int)rx_buffer[1] << 8); // Combine two bytes into a short integer for X
    X = (float)(x >> 6) / 256; // Convert raw data to a float and scale to expected range
    y = rx_buffer[2] | ((short int)rx_buffer[3] << 8); // Combine two bytes for Y
    Y = (float)(y >> 6) / 256; // Scale Y value
    z = rx_buffer[4] | ((short int)rx_buffer[5] << 8); // Combine two bytes for Z
    Z = (float)(z >> 6) / 256; // Scale Z value

    // Step 7: Format the data as a string to be sent to the I2C device (LCD display)
    sprintf(&data[0], "%.1f", X);  // Format X value with one decimal place
    data[3] = 'g';  // Append 'g' for gravity units
    data[4] = ' ';  // Add space after X value
    sprintf(&data[5], "%.1f", Y);  // Format Y value
    data[8] = 'g';  // Append 'g' for Y value
    data[9] = ' ';  // Add space after Y value
    sprintf(&data[10], "%.1f", Z); // Format Z value
    data[13] = 'g'; // Append 'g' for Z value
    data[14] = ' '; // Add space after Z value
    data[15] = '\0'; // Null-terminate the string

    // Step 8: Open I2C device and write the formatted data
    int fd = open(I2C_DEVICE, O_RDWR);  // Open I2C device
    write(fd, data, strlen(data) + 1); // Write the data to the I2C device
    close(fd);  // Close the I2C device file descriptor

    // Step 9: Print the formatted data to the console
    printf("data = %s\n", data);

    return 0;  // Exit the program
}
