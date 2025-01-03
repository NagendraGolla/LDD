/*Start Module

  Define GPIO Pins:
    - MOSI = GPIO pin 535
    - MISO = GPIO pin 536
    - SCK = GPIO pin 537
    - CS  = GPIO pin 529

  Define Buffers:
    - tx_buffer = "Hello SPI Slave!" (Data to send to slave)
    - rx_buffer = empty buffer to store received data

  Define File Operations:
    - spi_write: Write data from user-space to tx_buffer
    - spi_read: Read data from rx_buffer to user-space

  Define SPI Master Data Transfer:
    - For each byte in tx_buffer:
        - Set CS (Chip Select) to low (start communication)
        - For each bit in the byte (MSB first):
            - Set MOSI (Master Out Slave In) pin to current bit of the byte
            - Read the bit from MISO (Master In Slave Out) pin
            - Shift received bit into the received byte
            - Toggle the SCK (Serial Clock) pin (High to Low)
        - Store received byte in rx_buffer
        - Set CS to high (end communication)

  Define SPI Write Operation:
    - Copy data from user-space to tx_buffer
    - Call SPI data transfer function to send data to slave and receive response

  Define SPI Read Operation:
    - Copy data from rx_buffer to user-space (return received data)

  Initialize the Module:
    - Register a character device with a major number
    - Request GPIO pins for MOSI, MISO, SCK, CS
    - Configure GPIO pin directions:
        - MOSI as output
        - MISO as input
        - SCK as output
        - CS as output (active low)
    - Log the successful initialization

  Cleanup the Module:
    - Free requested GPIO pins
    - Unregister the character device

End Module*/











#include <linux/module.h>      // Required for creating kernel modules and related functions
#include <linux/gpio.h>        // Required for GPIO management functions (requesting, reading, writing)
#include <linux/uaccess.h>     // For user-space access functions like copy_to_user and copy_from_user
#include <linux/delay.h>       // For delay functions like udelay (microsecond delay) and msleep (millisecond delay)
#include <linux/fs.h>          // For file system operations like read, write, and device registration

#define DRIVER_NAME "spi_master_bitbang"  // Define the name of the driver
#define GPIO_MOSI 535       // Define GPIO pin number for MOSI (Master Out Slave In) - output pin
#define GPIO_MISO 536       // Define GPIO pin number for MISO (Master In Slave Out) - input pin
#define GPIO_SCK  537       // Define GPIO pin number for SCK (Serial Clock) - output pin
#define GPIO_CS   529       // Define GPIO pin number for CS (Chip Select) - output pin

static char tx_buffer[32] = "Hello SPI Slave!";  // Buffer to hold data to be sent to the slave
static char rx_buffer[32];                        // Buffer to hold received data from the slave
static int major=0;                               // Variable to hold the major number for device registration

// SPI Data Transfer (Bit-banging)
static void spi_master_transfer(void)
{
    int byte_idx, bit_idx;
    char received_byte = 0;

    // Loop through each byte in the tx_buffer to send to the slave
    for (byte_idx = 0; byte_idx < sizeof(tx_buffer); byte_idx++) {
        char byte_to_send = tx_buffer[byte_idx];  // Get the byte to be sent in the current iteration

        // Start communication by pulling CS (Chip Select) low
        gpio_set_value(GPIO_CS, 0);  // CS pin low indicates start of communication
        udelay(3);  // Small delay to simulate chip select activation

        // Loop through each bit of the byte to send (MSB first)
        for (bit_idx = 7; bit_idx >= 0; bit_idx--) {
            // Set MOSI to the corresponding bit in the byte
            int mosi_bit = (byte_to_send >> bit_idx) & 0x01;
            gpio_set_value(GPIO_MOSI, mosi_bit);  // Set MOSI pin to the current bit value

            // Read the MISO pin (Master In Slave Out) for the received bit
            int miso_bit = gpio_get_value(GPIO_MISO);
            received_byte = (received_byte << 1) | miso_bit;  // Shift the received bit into the received byte

            // Toggle the SCK (Serial Clock) line for each bit
            gpio_set_value(GPIO_SCK, 1);  // Set SCK high (rising edge)
            udelay(3);  // Small delay to simulate clock pulse
            gpio_set_value(GPIO_SCK, 0);  // Set SCK low (falling edge)
            udelay(3);  // Small delay to simulate clock pulse
        }

        // Store the received byte in the rx_buffer
        rx_buffer[byte_idx] = received_byte;
        pr_info("RECEIVED BYTE: %c\n", received_byte);  // Log the received byte

        // End communication by pulling CS high (deselect chip)
        gpio_set_value(GPIO_CS, 1);  // Set CS pin high to end communication
        udelay(3);  // Small delay to simulate chip deselection
    }

    // Log the complete data that was sent and received
    pr_info("SPI Master Sent Data: %s\n", tx_buffer);
    pr_info("SPI Master Received Data: %s\n", rx_buffer);
}

// File operation: Write data from user space to the tx_buffer
static ssize_t spi_write(struct file *file, const char __user *buff, size_t len, loff_t *offset)
{
    // Copy data from user space to the kernel buffer (tx_buffer)
    if (copy_from_user(tx_buffer, buff, len)) {
        pr_info("Unable to copy from user\n");  // Log an error if copy fails
        return -EINVAL;  // Return error
    }

    // Perform the SPI master transfer function (send and receive data)
    spi_master_transfer();

    return len;  // Return the length of data written
}

// File operation: Read data from the rx_buffer (received data) to user space
static ssize_t spi_read(struct file *file, char __user *buff, size_t len, loff_t *offset)
{
    // Copy data from the kernel buffer (rx_buffer) to user space
    if (copy_to_user(buff, rx_buffer, len)) {
        pr_info("Unable to copy to user\n");  // Log an error if copy fails
        return -EINVAL;  // Return error
    }

    return len;  // Return the length of data read
}

// Define the file operations for the SPI master driver
static struct file_operations fops = {
    .write = spi_write,  // Assign write function for writing data to the driver
    .read = spi_read,    // Assign read function for reading data from the driver
};

// Module initialization function (called when the module is loaded)
static int __init spi_master_init(void)
{ 
    // Register the character device with a dynamically allocated major number
    major = register_chrdev(0, "SPI_MASTER", &fops);  
    pr_info("Registered with major number %d\n", major);  // Log the major number

    pr_info("Initializing SPI Master (Bit-banging)\n");

    // Request GPIOs for MOSI, MISO, SCK, and CS pins
    if (gpio_request(GPIO_MOSI, "MOSI") ||
        gpio_request(GPIO_MISO, "MISO") ||
        gpio_request(GPIO_SCK, "SCK") ||
        gpio_request(GPIO_CS, "CS")) {
        pr_err("Failed to request GPIOs\n");  // Log an error if GPIO request fails
        return -EBUSY;  // Return error if GPIO request fails
    }

    // Configure GPIO directions (output for MOSI, SCK, CS; input for MISO)
    gpio_direction_output(GPIO_MOSI, 0);  // Set MOSI pin as output, initial value is 0
    gpio_direction_input(GPIO_MISO);      // Set MISO pin as input (receive data from slave)
    gpio_direction_output(GPIO_SCK, 0);   // Set SCK pin as output, initial value is 0
    gpio_direction_output(GPIO_CS, 1);    // Set CS pin as output, initial value is 1 (inactive)

    pr_info("SPI Master Initialized\n");

    return 0;  // Return success
}

// Module cleanup function (called when the module is unloaded)
static void __exit spi_master_exit(void)
{
    // Free the GPIOs that were previously requested
    gpio_free(GPIO_MOSI);
    gpio_free(GPIO_MISO);
    gpio_free(GPIO_SCK);
    gpio_free(GPIO_CS);

    // Unregister the character device
    unregister_chrdev(major, "SPI_MASTER");

    pr_info("SPI Master Exited\n");  // Log that the module has exited
}

// Register the module's init and exit functions
module_init(spi_master_init);   // Call spi_master_init when the module is loaded
module_exit(spi_master_exit);   // Call spi_master_exit when the module is unloaded

// Module metadata
MODULE_LICENSE("GPL");               // Specify the license type for the module
MODULE_AUTHOR("TEAM 1 & 7");            // Author's name
MODULE_DESCRIPTION("SPI Master using Bit-banging for Raspberry Pi");  // Module description

