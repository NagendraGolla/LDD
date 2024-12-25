/*
 * Pseudo code for SPI Slave Emulation
 *
 * 1. Initialize GPIO pins for SPI communication (MOSI, MISO, SCLK, CS, LED).
 * 2. Set up a character device for user space communication.
 * 3. Configure the interrupt for Chip Select (CS) pin to trigger an interrupt on rising and falling edges.
 * 4. Emulate SPI data transfer in response to the CS interrupt:
 *    a. Wait for clock pulses (SCLK) and read the MOSI pin.
 *    b. Write to the MISO pin based on the emulated response data.
 * 5. Handle the received data, and if it’s 0 or 1, toggle the LED accordingly.
 * 6. Provide feedback to user-space through the character device (success/failure).
 * 7. Clean up GPIOs and IRQ on module unload.
 */

#include <linux/init.h>             // For module initialization and cleanup
#include <linux/kernel.h>           // For printk (kernel log) functionality
#include <linux/module.h>           // For defining the module
#include <linux/gpio.h>             // For GPIO manipulation
#include <linux/interrupt.h>        // For handling interrupts
#include <linux/delay.h>            // For introducing delays (udelay)
#include <linux/fs.h>               // For file operations
#include <linux/uaccess.h>          // For copy_to_user and copy_from_user
#include <linux/cdev.h>             // For character device registration

#define DRIVER_NAME "spi_slave_emulation"  // Name of the driver
#define GPIO_MOSI 535                    // GPIO Pin for MOSI (Master Out Slave In)
#define GPIO_MISO 536                    // GPIO Pin for MISO (Master In Slave Out)
#define GPIO_SCLK 537                    // GPIO Pin for SCLK (Serial Clock)
#define GPIO_CS   520                    // GPIO Pin for CS (Chip Select)
#define GPIO_LED  529                    // GPIO Pin for LED (to show success/failure)

static void spi_emulate_transfer(struct tasklet_struct *spi);

// Variables
static int cs_irq;                          // Interrupt number for CS pin
static volatile bool spi_active = false;    // Flag to indicate if SPI is active
int monitoring_flag = 0;                    // Flag for monitoring the communication

DECLARE_TASKLET(spi_tasklet, spi_emulate_transfer);  // Declaring a tasklet for SPI emulation

// SPI buffer for communication (rx and tx buffers)
static char rx_buffer[32];                 // Buffer to store received data
static char tx_buffer[32] = "Response from SPI Slave";  // Response data to send

// Open function to handle device opening (called from user-space)
static int spi_open(struct inode *inode, struct file *file) {
    pr_info("SPI Device opened\n");  // Log message
    return 0;
}

// Close function to handle device closing (called from user-space)
static int spi_close(struct inode *inode, struct file *file) {
    pr_info("SPI Device closed\n");  // Log message
    return 0;
}

// Interrupt Handler for Chip Select (CS) pin
static irqreturn_t cs_irq_handler(int irq, void *dev_id) {
    spi_active = !gpio_get_value(GPIO_CS);  // Toggle the SPI active status based on CS pin value
    if (spi_active) {
        tasklet_schedule(&spi_tasklet);  // Schedule the tasklet to handle SPI communication
        pr_info("SPI communication started\n");
    } else {
        pr_info("SPI communication ended\n");
    }
    return IRQ_HANDLED;  // Acknowledge the interrupt
}

// Function to emulate the SPI transfer
static void spi_emulate_transfer(struct tasklet_struct *spi) {
    int byte_idx = 0;            // Byte index for receiving data
    int bit_idx = 0;             // Bit index for the current byte
    char received_byte = 0;      // Variable to store the received byte

    // Emulate SPI data transfer while SPI is active
    while (spi_active) {
        // Wait for the clock falling edge (polling SCLK)
        while (gpio_get_value(GPIO_SCLK) && spi_active) {
            udelay(1);  // Busy wait (to avoid high CPU usage)
        }

        if (!spi_active)  // Check if SPI communication has ended
            break;

        // Read the MOSI (Master Out Slave In) bit
        int mosi_bit = gpio_get_value(GPIO_MOSI);
        received_byte = (received_byte << 1) | mosi_bit;  // Shift and store the bit

        // Write the MISO (Master In Slave Out) bit from the tx_buffer
        int miso_bit = (tx_buffer[byte_idx] >> (7 - bit_idx)) & 0x01;
        gpio_set_value(GPIO_MISO, miso_bit);

        bit_idx++;  // Move to the next bit

        // If all 8 bits of a byte are received, store the byte and prepare for the next byte
        if (bit_idx == 8) {
            rx_buffer[byte_idx] = received_byte;
            pr_info("Received Byte: 0x%02x\n", received_byte);  // Log received byte
            received_byte = 0;  // Reset for next byte
            bit_idx = 0;        // Reset bit index
            byte_idx++;         // Move to next byte in the buffer

            if (byte_idx >= sizeof(rx_buffer))  // If buffer is full, stop transfer
                break;
        }

        // Wait for clock rising edge (polling SCLK)
        while (!gpio_get_value(GPIO_SCLK) && spi_active) {
            udelay(1);  // Busy wait
        }
    }

    // After communication ends, interpret the received data
    int value = simple_strtol(rx_buffer, NULL, 10);  // Convert received data to an integer

    if (value == 0 || value == 1) {
        monitoring_flag = 1;
        gpio_set_value(GPIO_LED, value);  // Set LED based on received value
        pr_info("GPIO Device write: %d\n", value);  // Log the operation
    } else {
        monitoring_flag = -1;
        pr_err("Invalid value: GPIO accepts 0 or 1\n");  // Log error if invalid data received
    }
}

// Read function to return result to user-space
static ssize_t spi_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    // Handle success or failure based on monitoring_flag
    if (monitoring_flag > 0) {
        if (copy_to_user(buf, "success", 8)) {
            pr_err("Failed to send data to user\n");
            return -EFAULT;
        }
        monitoring_flag = 0;
        return 1;
    } else if (monitoring_flag < 0) {
        if (copy_to_user(buf, "failure", 8)) {
            pr_err("Failed to send data to user\n");
            return -EFAULT;
        }
        monitoring_flag = 0;
        return -1;
    } else {
        return 0;  // No data to return
    }
}

// File operations structure for character device
static struct file_operations spi_fops = {
    .owner = THIS_MODULE,
    .open = spi_open,
    .release = spi_close,
    .read = spi_read,
};

// Module Initialization function
static int __init spi_slave_init(void) {
    int ret;
    dev_t dev;
    struct cdev spi_cdev;

    pr_info("Initializing SPI Slave Emulation\n");

    // Allocate device number for the character device
    if ((alloc_chrdev_region(&dev, 0, 1, DRIVER_NAME)) < 0) {
        pr_err("Cannot allocate major number\n");
        return -1;
    }

    pr_info("Major = %d Minor = %d\n", MAJOR(dev), MINOR(dev));  // Log allocated major and minor numbers

    // Initialize character device structure
    cdev_init(&spi_cdev, &spi_fops);

    // Add the character device to the system
    if ((cdev_add(&spi_cdev, dev, 1)) < 0) {
        pr_err("Cannot add the device to the system\n");
        goto r_cdev;
    }

    // Configure GPIO pins for SPI (MOSI, MISO, SCLK, CS)
    if (gpio_request(GPIO_MOSI, "MOSI") ||
        gpio_request(GPIO_MISO, "MISO") ||
        gpio_request(GPIO_SCLK, "SCLK") ||
        gpio_request(GPIO_CS, "CS")) {
        pr_err("Failed to request GPIOs\n");
        goto r_cdev;
    }

    gpio_direction_input(GPIO_MOSI);
    gpio_direction_output(GPIO_MISO, 0);
    gpio_direction_input(GPIO_SCLK);
    gpio_direction_input(GPIO_CS);

    // Setup IRQ for CS pin (chip select)
    cs_irq = gpio_to_irq(GPIO_CS);
    if (cs_irq < 0) {
        pr_err("Failed to get IRQ for CS\n");
        goto r_gpio;
    }

    // Request IRQ for CS pin
    ret = request_irq(cs_irq, cs_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, DRIVER_NAME, NULL);
    if (ret) {
        pr_err("Failed to request IRQ\n");
        goto r_irq;
    }

    pr_info("SPI Slave Emulation Initialized\n");
    return 0;

r_irq:
    free_irq(cs_irq, NULL);

r_gpio:
    gpio_free(GPIO_MOSI);
    gpio_free(GPIO_MISO);
    gpio_free(GPIO_SCLK);
    gpio_free(GPIO_CS);

r_cdev:
    unregister_chrdev_region(dev, 1);  // Free the allocated device number
    return -EFAULT;
}

// Module Cleanup function
static void __exit spi_slave_exit(void) {
    free_irq(cs_irq, NULL);  // Free IRQ
    gpio_free(GPIO_MOSI);    // Free GPIO pins
    gpio_free(GPIO_MISO);
    gpio_free(GPIO_SCLK);
    gpio_free(GPIO_CS);

    pr_info("SPI Slave Emulation Exited\n");
}

module_init(spi_slave_init);  // Register the init function
module_exit(spi_slave_exit);  // Register the exit function

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Team 1 and Team 7");
MODULE_DESCRIPTION("SPI Slave Emulation using GPIO for Raspberry Pi");
