#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

// Pseudo Code for the Module Initialization and Setup:
// 1. Define GPIO pin numbers for SPI signals (MOSI, MISO, SCLK, CS)
// 2. Declare tasklet for SPI data transfer simulation
// 3. Define buffers for received and transmitted data
// 4. Implement SPI open, close, and read functions
// 5. Implement CS interrupt handler to start/stop SPI communication
// 6. Implement the SPI emulation logic using GPIO pins
// 7. Implement module initialization (allocating device numbers, configuring GPIOs, registering IRQ)
// 8. Implement module cleanup (free resources like GPIOs, IRQ, and device number)

#define DRIVER_NAME "spi_slave_emulation"
#define GPIO_MOSI 535  // GPIO Pin for Master Out Slave In (MOSI)
#define GPIO_MISO 536  // GPIO Pin for Master In Slave Out (MISO)
#define GPIO_SCLK 537  // GPIO Pin for Serial Clock (SCLK)
#define GPIO_CS   520  // GPIO Pin for Chip Select (CS), GPIO 8
#define GPIO_LED  529  // GPIO Pin for LED, used for indicating activity

// Declare the tasklet for SPI transfer emulation
static void spi_emulate_transfer(struct tasklet_struct *spi);

// IRQ and flags for SPI communication
static int cs_irq;
static volatile bool spi_active = false;

// Monitoring flag to track if data has been received
int monitoring_flag = 0;

// Tasklet declaration for SPI data transfer emulation
DECLARE_TASKLET(spi_tasklet, spi_emulate_transfer);

// Buffers for storing received and transmitted data
static char rx_buffer[32];  // Buffer for received data (max 32 bytes)
static char tx_buffer[32] = "Response from SPI Slave";  // Buffer for data to send back (default response)

// Pseudo Code for SPI open function:
// 1. Log a message when the device is opened
// 2. Return success

static int spi_open(struct inode *inode, struct file *file)
{
    pr_info("SPI Device opened\n");  // Log when device is opened
    return 0;  // Return success
}

// Pseudo Code for SPI close function:
// 1. Log a message when the device is closed
// 2. Return success

static int spi_close(struct inode *inode, struct file *file) 
{
    pr_info("SPI Device closed\n");  // Log when device is closed
    return 0;  // Return success
}

// Pseudo Code for Chip Select (CS) IRQ Handler:
// 1. Check the current state of the Chip Select (CS) pin.
// 2. Toggle the SPI active state based on CS value.
// 3. If CS is active, schedule the SPI data transfer tasklet.

static irqreturn_t cs_irq_handler(int irq, void *dev_id)
{
    // Toggle the SPI activity based on CS pin state
    spi_active = !gpio_get_value(GPIO_CS);

    // If CS is active, schedule the tasklet for data transfer
    if (spi_active) {
        tasklet_schedule(&spi_tasklet);  // Schedule SPI data transfer tasklet
        pr_info("SPI communication started\n");
    } else {
        pr_info("SPI communication ended\n");
    }

    return IRQ_HANDLED;  // Return interrupt handled status
}

// Pseudo Code for SPI Data Transfer Emulation:
// 1. Continuously monitor the SCLK pin for clock edges.
// 2. On the falling edge, read one bit from MOSI and store it in a byte.
// 3. On the rising edge, send one bit from the response buffer to MISO.
// 4. Repeat until all data is received and transmitted.
static void spi_emulate_transfer(struct tasklet_struct *spi)
{
    int byte_idx = 0;
    int bit_idx = 0;
    char received_byte = 0;

    while (spi_active) {
        // Wait for clock falling edge (polling SCLK)
        while (gpio_get_value(GPIO_SCLK) && spi_active)
            udelay(1);  // Small delay to avoid busy-waiting

        // Break if SPI communication is stopped
        if (!spi_active)
            break;

        // Read one bit from MOSI and shift it into the received byte
        int mosi_bit = gpio_get_value(GPIO_MOSI);
        received_byte = (received_byte << 1) | mosi_bit;

        // Send one bit from the response buffer (MISO)
        int miso_bit = (tx_buffer[byte_idx] >> (7 - bit_idx)) & 0x01;
        gpio_set_value(GPIO_MISO, miso_bit);

        bit_idx++;  // Move to the next bit position

        // If all 8 bits are received, store the byte in the receive buffer
        if (bit_idx == 8) {
            rx_buffer[byte_idx] = received_byte;  // Store received byte
            pr_info("Received Byte: 0x%02x\n", received_byte);  // Log received byte
            received_byte = 0;  // Reset byte accumulator
            bit_idx = 0;  // Reset bit index
            byte_idx++;  // Move to the next byte

            if (byte_idx >= sizeof(rx_buffer))  // Stop if buffer is full
                break;
        }

        // Wait for clock rising edge (polling SCLK)
        while (!gpio_get_value(GPIO_SCLK) && spi_active)
            udelay(1);  // Small delay to avoid busy-waiting
    }

    monitoring_flag = 1;  // Set flag indicating data is available
}

// Pseudo Code for SPI read function:
// 1. Check if data has been received (monitoring_flag is set).
// 2. If data is available, copy the received data from kernel space to user space.
// 3. Reset the monitoring flag after reading the data.

static ssize_t spi_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    if (len > 8)  // Limit the number of bytes to 8 for simplicity
        len = 8;

    // If data has been received (indicated by monitoring_flag)
    if (monitoring_flag == 1) {
        if (copy_to_user(buf, rx_buffer, len)) {
            pr_err("Failed to send data to user\n");
            return -EFAULT;  // Return error if data cannot be copied to user
        }
        monitoring_flag = 0;  // Reset monitoring flag after data transfer
        return 0;  // Success
    }

    return -EFAULT;  // Return error if no data available
}

// Pseudo Code for file operations structure:
// 1. Define the file operations: open, release, read.
static struct file_operations spi_fops = {
    .owner = THIS_MODULE,  // Set owner to current module
    .open = spi_open,  // Define open function
    .release = spi_close,  // Define close function
    .read = spi_read,  // Define read function
};

// Pseudo Code for Module Initialization:
// 1. Allocate a device number for the SPI slave device.
// 2. Initialize the character device structure.
// 3. Register the device with the system.
// 4. Request GPIO pins for SPI signals (MOSI, MISO, SCLK, CS).
// 5. Set up the IRQ for CS (Chip Select).
// 6. Log the success or failure of each initialization step.

static int __init spi_slave_init(void)
{
    int ret;
    dev_t dev;
    struct cdev spi_cdev;

    pr_info("Initializing SPI Slave Emulation\n");

    // Allocate a device number (major and minor numbers)
    if ((alloc_chrdev_region(&dev, 0, 1, DRIVER_NAME)) < 0) {
        pr_err("Cannot allocate major number\n");  // Log error if allocation fails
        return -1;  // Return failure
    }

    pr_info("Major = %d Minor = %d\n", MAJOR(dev), MINOR(dev));  // Log the allocated device number

    // Initialize the character device structure with the file operations
    cdev_init(&spi_cdev, &spi_fops);

    // Add the character device to the system
    if ((cdev_add(&spi_cdev, dev, 1)) < 0) {
        pr_err("Cannot add the device to the system\n");  // Log error if adding fails
        goto r_cdev;  // Return failure
    }

    // Configure GPIOs
    if (gpio_request(GPIO_MOSI, "MOSI") ||
        gpio_request(GPIO_MISO, "MISO") ||
        gpio_request(GPIO_SCLK, "SCLK") ||
        gpio_request(GPIO_CS, "CS")) {
        pr_err("Failed to request GPIOs\n");
        goto r_cdev;
    }

    gpio_direction_input(GPIO_MOSI);  // Set MOSI as input (receive data)
    gpio_direction_output(GPIO_MISO, 0);  // Set MISO as output (send data)
    gpio_direction_input(GPIO_SCLK);  // Set SCLK as input (clock signal)
    gpio_direction_input(GPIO_CS);  // Set CS as input (chip select)

    // Setup Chip Select IRQ (interrupt request)
    cs_irq = gpio_to_irq(GPIO_CS);
    if (cs_irq < 0) {
        pr_err("Failed to get IRQ for CS\n");  // Log error if IRQ setup fails
        goto r_gpio;
    }

    ret = request_irq(cs_irq, cs_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, DRIVER_NAME, NULL);
    if (ret) {
        pr_err("Failed to request IRQ\n");  // Log error if IRQ request fails
        goto r_irq;
    }

    pr_info("SPI Slave Emulation Initialized\n");
    return 0;  // Success

r_irq:
    free_irq(cs_irq, NULL);  // Free IRQ if setup fails

r_gpio:
    gpio_free(GPIO_MOSI);  // Free GPIO resources
    gpio_free(GPIO_MISO);
    gpio_free(GPIO_SCLK);
    gpio_free(GPIO_CS);

r_cdev:
    unregister_chrdev_region(dev, 1);  // Free the allocated device number
    return -EFAULT;  // Return error
}

// Pseudo Code for Module Cleanup:
// 1. Free the IRQ and GPIO resources.
// 2. Unregister the device.
// 3. Log the cleanup process.

static void __exit spi_slave_exit(void)
{
    free_irq(cs_irq, NULL);  // Free IRQ resources
    gpio_free(GPIO_MOSI);  // Free GPIO resources
    gpio_free(GPIO_MISO);
    gpio_free(GPIO_SCLK);
    gpio_free(GPIO_CS);

    pr_info("SPI Slave Emulation Exited\n");  // Log exit message
}

module_init(spi_slave_init);  // Initialize the module
module_exit(spi_slave_exit);  // Cleanup on exit

MODULE_LICENSE("GPL");  // Define module license
MODULE_AUTHOR("Team 1 and Team 7");  // Define module author
MODULE_DESCRIPTION("SPI Slave Emulation using GPIO for Raspberry Pi");  // Define module description
