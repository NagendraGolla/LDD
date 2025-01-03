/*
 * Pseudo Code:
 * 1. Define UART register base address and memory size.
 * 2. Define UART register offsets and flags.
 * 3. Declare a pointer for memory-mapped I/O of UART registers.
 * 4. Define file operations: open, write, read, and close.
 * 5. Define module initialization and cleanup functions.
 * 6. Register and configure UART device, mapping memory and handling device creation and cleanup.
 */

// Include necessary Linux kernel headers
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>

// Define base address and size for UART0 (for Raspberry Pi 3/4, adjust for RPi1/2)
#define UART0_BASE   0x3F201000 // Base address for UART0
#define UART_REG_SIZE 0x1000    // Memory size to map for UART registers

// Define UART register offsets (based on the datasheet for UART)
#define UART_DR      0x00 // Data Register (holds data for transmission/reception)
#define UART_FR      0x18 // Flag Register (indicates UART status flags)
#define UART_IBRD    0x24 // Integer Baud Rate Divisor
#define UART_FBRD    0x28 // Fractional Baud Rate Divisor
#define UART_LCRH    0x2C // Line Control Register (for configuring UART line settings)
#define UART_CR      0x30 // Control Register (enables/disables UART)
#define UART_IMSC    0x38 // Interrupt Mask Set/Clear Register

// UART Flags (used for status checking)
#define UART_FR_TXFF 0x20 // Transmit FIFO Full (if set, UART is busy transmitting)
#define UART_FR_RXFE 0x10 // Receive FIFO Empty (if set, no data is available to read)

// Pointer to map UART register space
static void __iomem *uart_base; // I/O memory base pointer for UART registers

// File operation for opening the UART device
static int uart_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "UART device opened\n"); // Log a message when device is opened
    return 0; // Return 0 for success
}

// File operation for writing to the UART device
static ssize_t uart_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    char c; // Temporary variable to hold a character to be transmitted
    int i=0;
    // Loop through the data to write
    while (count--) {
        // Copy one byte of data from user space to kernel space
        if (copy_from_user(&c, buf+i, 1))
            return -EFAULT; // Return error if copying fails

        // Wait until the Transmit FIFO is not full
        while (readl(uart_base + UART_FR) & UART_FR_TXFF)
            cpu_relax(); // Relax CPU to allow other tasks to run while waiting

        // Write the character to the UART Data Register
        writel(c, uart_base + UART_DR);
	i++;
    }

    return count; // Return the number of bytes written
}

// File operation for reading from the UART device
static ssize_t uart_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    char rx_buffer[20]; // Temporary variable to store received data
    int i=0;

    while(1)
    {
    // Wait until the Receive FIFO has data
    while (readl(uart_base + UART_FR) & UART_FR_RXFE)
        cpu_relax(); // Relax CPU to allow other tasks while waiting

    // Read one byte of data from UART Data Register
    rx_buffer[i] = readl(uart_base + UART_DR) & 0xFF;
    if(rx_buffer[i]=='\0')
	    break;
    i++;

    }
    // Copy the received byte to user space
    if (copy_to_user(buf, rx_buffer, i))
        return -EFAULT; // Return error if copying fails

    return 1; // Return 1 byte read
}

// File operation for closing the UART device
static int uart_close(struct inode *inode, struct file *file) {
    printk(KERN_INFO "UART device closed\n"); // Log a message when device is closed
    return 0; // Return 0 for success
}

// File operations structure (open, write, read, close)
static const struct file_operations uart_fops = {
    .owner = THIS_MODULE,        // Set module as the owner of the operations
    .open = uart_open,           // Assign the open function
    .write = uart_write,         // Assign the write function
    .read = uart_read,           // Assign the read function
    .release = uart_close,       // Assign the close function
};

// Major number for the character device
static int major;

// Class structure for creating the device
static struct class *uart_class;

// Module initialization function
static int __init uart_init(void) {
    printk(KERN_INFO "Initializing UART driver\n");

    // Map UART registers into kernel virtual address space
    uart_base = ioremap(UART0_BASE, UART_REG_SIZE);
    if (!uart_base) {
        printk(KERN_ERR "Failed to map UART registers\n");
        return -ENOMEM; // Return error if mapping fails
    }

    // Disable UART by clearing the UART Control Register
    writel(0, uart_base + UART_CR);

    // Configure UART (Baud Rate: 115200, 8N1 format)
    writel(1, uart_base + UART_IBRD); // Set Integer Baud Rate Divisor (1 for 115200)
    writel(40, uart_base + UART_FBRD); // Set Fractional Baud Rate Divisor (40 for 115200)
    writel((3 << 5) | (1 << 4), uart_base + UART_LCRH); // Set Line Control (8-bit, no parity, 1 stop bit)
    writel((1 << 9) | (1 << 8) | 1, uart_base + UART_CR); // Enable UART (TX/RX, UART Enable)

    // Register the character device with a dynamically assigned major number
    major = register_chrdev(0, "uart", &uart_fops);
    if (major < 0) {
        printk(KERN_ERR "Failed to register UART device\n");
        iounmap(uart_base); // Unmap the UART registers on failure
        return major; // Return the error code from registering the device
    }

    // Create a class for the UART device to make it available in /dev
    uart_class = class_create(THIS_MODULE, "uart");
    if (IS_ERR(uart_class)) {
        unregister_chrdev(major, "uart"); // Unregister device on failure
        iounmap(uart_base); // Unmap UART registers on failure
        return PTR_ERR(uart_class); // Return error code for class creation
    }

    // Create the device file in /dev
    device_create(uart_class, NULL, MKDEV(major, 0), NULL, "uart");
    printk(KERN_INFO "UART driver initialized successfully\n");

    return 0; // Return 0 for successful initialization
}

// Module cleanup function
static void __exit uart_exit(void) {
    printk(KERN_INFO "Exiting UART driver\n");

    // Clean up and unregister resources
    device_destroy(uart_class, MKDEV(major, 0)); // Destroy the device file
    class_destroy(uart_class); // Destroy the class
    unregister_chrdev(major, "uart"); // Unregister the character device
    iounmap(uart_base); // Unmap UART registers from memory
}

// Specify the initialization and cleanup functions
module_init(uart_init);
module_exit(uart_exit);

// Define module metadata
MODULE_LICENSE("GPL");            // Set the module license to GPL
MODULE_AUTHOR("TEAM 1 & 7");      // Author of the module
MODULE_DESCRIPTION("UART driver for Raspberry Pi"); // Short description of the module
