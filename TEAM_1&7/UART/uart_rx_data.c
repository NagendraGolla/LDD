/*pseudo code                                                                                                                                                                1. Define base address for UART0 and other necessary constants.
2. Define GPIO pin for controlling LED (e.g., GPIO pin 529).
3. Implement file operations for the UART device (open, read, write, close).
   - Implement uart_open(): This is invoked when the device is opened.
   - Implement uart_write(): This handles writing data to the UART device.
   - Implement uart_read(): This handles reading data from the UART device and controlling the LED based on the command received.
   - Implement uart_close(): This is invoked when the device is closed.
4. Initialize the UART registers, configure the GPIO pin, and register the device.
   - Map the UART registers into virtual memory.
   - Set up the UART parameters (baud rate, 8N1 format, etc.).
   - Request and configure the GPIO pin for output to control the LED.
5. Clean up resources when the module is removed:
   - Unregister the device, unmap the UART base address, and release the GPIO pin.*/







#include <linux/module.h>      // For module macros like module_init, module_exit
#include <linux/kernel.h>      // For printk() to output logs
#include <linux/init.h>         // For initialization and cleanup functions
#include <linux/io.h>          // For ioremap() to access hardware registers
#include <linux/fs.h>          // For file operations like open(), read(), write()
#include <linux/uaccess.h>     // For copy_from_user() and copy_to_user()
#include <linux/device.h>      // For creating device files

#define UART0_BASE   0x3F201000 // Base address for UART0 (Raspberry Pi 3/4)
#define UART_REG_SIZE 0x1000    // Memory size to map for UART registers

#define LED 529  // Define GPIO pin for LED (use an appropriate GPIO pin for your platform)

// UART Register Offsets
#define UART_DR      0x00 // Data Register
#define UART_FR      0x18 // Flag Register
#define UART_IBRD    0x24 // Integer Baud Rate Divisor
#define UART_FBRD    0x28 // Fractional Baud Rate Divisor
#define UART_LCRH    0x2C // Line Control Register
#define UART_CR      0x30 // Control Register
#define UART_IMSC    0x38 // Interrupt Mask Set/Clear Register

// UART Flags
#define UART_FR_TXFF 0x20 // Transmit FIFO Full
#define UART_FR_RXFE 0x10 // Receive FIFO Empty

static void __iomem *uart_base; // Pointer to the base address of UART registers

// Function to handle the opening of the UART device
static int uart_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "UART device opened\n"); // Log message when the device is opened
    return 0; // Successful open
}

// Function to handle writing data to the UART device
static ssize_t uart_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    char c;

    // Write data to the UART
    while (count--) {
        if (copy_from_user(&c, buf++, 1))  // Copy a byte of data from user space
            return -EFAULT;  // Return error if copy fails

        // Wait until Transmit FIFO is not full
        while (readl(uart_base + UART_FR) & UART_FR_TXFF)
            cpu_relax(); // Relax the CPU to prevent busy-waiting

        writel(c, uart_base + UART_DR); // Write the character to the data register
    }

    return count; // Return the number of bytes written
}

// Function to handle reading data from the UART device
static ssize_t uart_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    char c, data[4], i = 0;

    // Wait until Receive FIFO is not empty
    while (readl(uart_base + UART_FR) & UART_FR_RXFE)
        cpu_relax(); // Relax CPU until the data is available

    c = readl(uart_base + UART_DR) & 0xFF; // Read a byte from the UART data register
    data[i++] = c; // Store the byte in the data buffer

    if (copy_to_user(buf, &c, 1)) // Copy the byte to user space
        return -EFAULT; // Return error if copy fails

    // Handle specific commands to control the LED based on received data
    if (strcmp(data, "ON") == 0) {
        gpio_set_value(LED, 1); // Turn LED ON (set GPIO pin high)
        writel(1, uart_base + UART_DR); // Write "1" to UART data register to acknowledge
    } else if (strcmp(data, "OFF") == 0) {
        gpio_set_value(LED, 0); // Turn LED OFF (set GPIO pin low)
        writel(0, uart_base + UART_DR); // Write "0" to UART data register to acknowledge
    } else {
        pr_err("Invalid command\n"); // Print error if the command is invalid
        return -EINVAL; // Return invalid argument error
    }

    return 1; // Return the number of bytes read
}

// Function to handle the closing of the UART device
static int uart_close(struct inode *inode, struct file *file) {
    printk(KERN_INFO "UART device closed\n"); // Log message when the device is closed
    return 0; // Successful close
}

// File operations for the UART device
static const struct file_operations uart_fops = {
    .owner = THIS_MODULE,
    .open = uart_open,  // Call uart_open when the device is opened
    .write = uart_write, // Call uart_write when writing to the device
    .read = uart_read,   // Call uart_read when reading from the device
    .release = uart_close, // Call uart_close when the device is closed
};

static int major;          // Variable to store the major number
static struct class *uart_class; // Class for the UART device

// Module initialization function
static int __init uart_init(void) {
    printk(KERN_INFO "Initializing UART driver\n");

    // Map UART registers
    uart_base = ioremap(UART0_BASE, UART_REG_SIZE);  // Map the base address of UART0 to virtual memory
    if (!uart_base) {
        printk(KERN_ERR "Failed to map UART registers\n");
        return -ENOMEM; // Return memory allocation error if mapping fails
    }

    // Disable UART
    writel(0, uart_base + UART_CR); // Disable UART by clearing control register

    // Configure UART (115200 baud rate, 8N1 format)
    writel(1, uart_base + UART_IBRD);    // Set integer part of baud rate
    writel(40, uart_base + UART_FBRD);   // Set fractional part of baud rate
    writel((3 << 5) | (1 << 4), uart_base + UART_LCRH); // Set line control for 8 data bits, no parity, 1 stop bit
    writel((1 << 9) | (1 << 8) | 1, uart_base + UART_CR); // Enable UART

    // Request and configure the GPIO pin for the LED
    int ret = gpio_request(LED, "GPIO_LED"); // Request the GPIO pin for the LED
    if (ret) {
        pr_err("Unable to request GPIO\n");
        return ret; // Return error if GPIO request fails
    }
    
    // Set GPIO pin direction to output (initial value is LOW)
    ret = gpio_direction_output(LED, 0); // Set GPIO pin direction as output and initialize to 0 (OFF)
    if (ret) {
        pr_err("Failed to set GPIO direction for pin %d\n", LED);
        return ret; // Return error if setting GPIO direction fails
    }

    // Register the UART device
    major = register_chrdev(0, "uart", &uart_fops); // Register the character device
    if (major < 0) {
        printk(KERN_ERR "Failed to register UART device\n");
        iounmap(uart_base); // Unmap UART registers if registration fails
        return major; // Return error code if registration fails
    }

    // Create a device file for the UART device
    uart_class = class_create(THIS_MODULE, "uart");  // Create a class for the UART device
    if (IS_ERR(uart_class)) {
        unregister_chrdev(major, "uart"); // Unregister device if class creation fails
        iounmap(uart_base); // Unmap UART registers
        return PTR_ERR(uart_class); // Return error if class creation fails
    }

    device_create(uart_class, NULL, MKDEV(major, 0), NULL, "uart"); // Create the device file
    printk(KERN_INFO "UART driver initialized successfully\n");

    return 0; // Return 0 if initialization is successful
}

// Module cleanup function
static void __exit uart_exit(void) {
    printk(KERN_INFO "Exiting UART driver\n");

    // Clean up resources
    device_destroy(uart_class, MKDEV(major, 0)); // Destroy the device file
    class_destroy(uart_class); // Destroy the class
    unregister_chrdev(major, "uart"); // Unregister the character device
    iounmap(uart_base); // Unmap the UART registers
}

module_init(uart_init);   // Register the module initialization function
module_exit(uart_exit);   // Register the module exit function

MODULE_LICENSE("GPL");    // Module license
MODULE_AUTHOR("TEAM 1 & 7"); // Author of the module
MODULE_DESCRIPTION("UART driver for Raspberry Pi"); // Module description
