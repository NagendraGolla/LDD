/* Pseudocode:
 * 1. Initialize GPIO pins for SPI (MOSI, MISO, SCK, CS, LED).
 * 2. Set MOSI, SCK, and CS pins as input, and MISO, LED pins as output.
 * 3. Register a character device to allow user-space interaction.
 * 4. Wait for the master to initiate SPI communication by pulling CS low.
 * 5. On each clock cycle (SCK), read the bit from MOSI and shift it into a byte.
 * 6. Simultaneously, set the bit on the MISO line to send data back to the master.
 * 7. Once a full byte is received, store it in a buffer.
 * 8. When the user reads from the device, provide the received data to user-space.
 * 9. Control an LED based on the received data (turn it on/off).
 * 10. Clean up resources (GPIO pins and character device) during module removal.
 */


#include <linux/module.h>     // Required for module functions like module_init and module_exit
#include <linux/gpio.h>       // Required for GPIO control functions (requesting, reading, setting values)
#include <linux/delay.h>      // Required for delay functions like msleep and udelay
#include <linux/fs.h>         // For file operations like open, read, and close
#include <linux/uaccess.h>    // For user-space access functions like copy_to_user

#define DRIVER_NAME "spi_slave_bitbang"   // Driver name
#define GPIO_MOSI 535     // GPIO pin for MOSI (Master Out Slave In) (input)
#define GPIO_MISO 536     // GPIO pin for MISO (Master In Slave Out) (output)
#define GPIO_SCK  537     // GPIO pin for SCK (Serial Clock) (input)
#define GPIO_CS   529     // GPIO pin for CS (Chip Select) (input)
#define GPIO_LED  530     // GPIO pin for LED (output)

// Global variables to store received and transmitted data
static int major_number;                // For storing major number of character device
static char rx_buffer[20];              // Buffer for receiving data (20 bytes)
static char tx_buffer[20] = "HELLOMASTER";  // Buffer for transmitting data (initial message)

// Function to receive data from SPI (bit-banging)
static void spi_slave_receive(void)
{
    int byte_idx = 0, bit_idx;           // Indices to track byte and bit positions
    char received_byte = 0;              // Buffer for received byte

    // Wait for CS to be pulled low (indicates start of communication)
    while (gpio_get_value(GPIO_CS) == 1) {
        msleep(1);  // Wait for chip select to go low, indicating the start of SPI communication
    }

    pr_info("SPI Slave Started\n");       // Log that SPI slave communication has started

    // Loop through receiving each byte while CS is low
    while (gpio_get_value(GPIO_CS) == 0 && byte_idx < sizeof(rx_buffer)) {
        received_byte = 0;               // Reset received byte
        char byte_send = tx_buffer[byte_idx];  // Get the byte to be sent on MISO
        
        // Loop through each bit in the byte (7 bits, MSB first)
        for (bit_idx = 7; bit_idx >= 0; bit_idx--) {
            // Wait for SCK to go high (rising edge) to read data
            while (gpio_get_value(GPIO_SCK) == 0) {
                udelay(4);  // Delay to sync with clock
            }

            // Read data from MOSI pin (bit-by-bit)
            int mosi_bit = gpio_get_value(GPIO_MOSI);
            received_byte = (received_byte << 1) | mosi_bit;  // Shift and add bit

            // Set MISO pin with the received bit to send back to master
            int miso_bit = mosi_bit;  // Slave sends same bit back (in this case)
            gpio_set_value(GPIO_MISO, miso_bit);  // Set MISO pin to send bit

            // Wait for SCK to go low (falling edge) before moving to next bit
            while (gpio_get_value(GPIO_SCK) == 1) {
                udelay(4);  // Delay to sync with clock
            }
        }

        // Store the received byte in the buffer and log it
        rx_buffer[byte_idx] = received_byte;
        pr_info("SPI Slave Received Byte: 0x%02x\n", received_byte);
        pr_info("SPI Slave transferred Byte: 0x%02x\n", byte_send);
        byte_idx++;  // Move to the next byte
    }

    pr_info("SPI Slave Received Data: %s\n", rx_buffer);  // Log the full received data
}

// Function to read from the device (called when user-space reads data)
static ssize_t device_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset) {
   
    // Call the SPI slave receive function to read data
    spi_slave_receive();

    // Check the received data and control the LED based on it
    if (strstr(rx_buffer, "ON") == 0)  // If received data contains "ON", turn on the LED
        gpio_set_value(GPIO_LED, 1);
    else  // If received data contains "OFF", turn off the LED
        gpio_set_value(GPIO_LED, 0);

    // Copy the data from kernel space (rx_buffer) to user-space
    if (copy_to_user(user_buffer, rx_buffer + *offset, size)) {
        return -EFAULT;  // Return error if copying to user fails
    }
       
    *offset += size;  // Update the offset after reading
    printk(KERN_INFO "simple_device: Sent %zu bytes to the user\n", size);
    return size;  // Return number of bytes successfully read
}

// File operations structure: contains function pointers for device operations
static struct file_operations fops = {
    // .open = device_open,   // Optional open function (not used in this code)
    // .release = device_close,  // Optional close function (not used in this code)
    .read = device_read,   // Read function
    // .write = device_write,  // Optional write function (not used in this code)
};

// Module initialization function
static int __init spi_slave_init(void)
{
    pr_info("Initializing SPI Slave (Bit-banging)\n");

    // Request GPIOs for MOSI, MISO, SCK, CS, and LED pins
    if (gpio_request(GPIO_MOSI, "MOSI") ||
        gpio_request(GPIO_MISO, "MISO") ||
        gpio_request(GPIO_SCK, "SCK") ||
        gpio_request(GPIO_CS, "CS") ||
        gpio_request(GPIO_LED, "LED")) {
        pr_err("Failed to request GPIOs\n");
        return -EBUSY;  // Return error if any GPIO request fails
    }

    // Configure GPIO directions (input for MOSI, SCK, CS; output for MISO, LED)
    gpio_direction_input(GPIO_MOSI);   // MOSI pin as input
    gpio_direction_output(GPIO_MISO, 0);  // MISO pin as output, initialized to 0
    gpio_direction_input(GPIO_SCK);    // SCK pin as input
    gpio_direction_input(GPIO_CS);     // CS pin as input
    gpio_direction_output(GPIO_LED, 0);  // LED pin as output, initialized to off
    pr_info("SPI Slave Initialized\n");

    // Register the character device
    major_number = register_chrdev(0, "SPI_DRIVER", &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "simple_device: Failed to register device\n");
        return major_number;  // Return error if device registration fails
    }
    printk(KERN_INFO "simple_device: Registered with major number %d\n", major_number);
    return 0;  // Return success
}

// Module cleanup function
static void __exit spi_slave_exit(void)
{
    // Free the requested GPIOs
    gpio_free(GPIO_MOSI);
    gpio_free(GPIO_MISO);
    gpio_free(GPIO_SCK);
    gpio_free(GPIO_CS);
    gpio_free(GPIO_LED);

    pr_info("SPI Slave Exited\n");  // Log that the module is exiting
}

// Register the module's init and exit functions
module_init(spi_slave_init);
module_exit(spi_slave_exit);

// Module metadata
MODULE_LICENSE("GPL");  // GPL license
MODULE_AUTHOR("TechDhaba");  // Author name
MODULE_DESCRIPTION("SPI Slave using Bit-banging for Raspberry Pi");  // Description of the module

