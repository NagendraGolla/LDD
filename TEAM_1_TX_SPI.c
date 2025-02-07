/**
 * Pseudo code:
 * 1. Initialize SPI driver for Raspberry Pi 4B
 * 2. Define the basic SPI configuration (speed, chip select, etc.)
 * 3. Define and initialize the device-specific structure
 * 4. Probe function to handle device initialization and setup
 * 5. Remove function to clean up when the device is removed
 * 6. Register the SPI driver and handle device I/O operations like write
 * 7. Implement file operations for interacting with the SPI device via the file system
 * 8. Handle module initialization and exit
 */

#include <linux/module.h>        // Core header for Linux kernel modules
#include <linux/spi/spi.h>       // SPI driver interface header for SPI device handling
#include <linux/kernel.h>        // Provides kernel macros like pr_info, pr_err for logging
#include <linux/init.h>          // Macros for module initialization and exit functions
#include <linux/slab.h>          // For memory allocation using kzalloc, kmalloc
#include <linux/debugfs.h>       // For creating DebugFS entries to expose kernel data to user space
#include <linux/uaccess.h>       // Provides functions to interact with user space memory, like copy_to_user, copy_from_user

#define DRIVER_NAME "rpi4b_spi_driver"   // Driver name for identification in logs and DebugFS
#define SPI_BUS 0                 // SPI bus number (adjust as per your setup)
#define SPI_CS 0                  // Chip Select number (adjust as per your setup)
#define SPI_MAX_SPEED 500000      // Maximum SPI speed in Hz (500 kHz)

static struct spi_device *spi_dev; 

// Structure to hold SPI device-related data
struct rpi4b_spi_dev {
	struct spi_device *spi;       // SPI device handle to interact with the SPI device
};

/**
 * Pseudo code for rpi4b_spi_probe function:
 * 1. Log that the SPI device is being probed
 * 2. Allocate memory for device-specific data using devm_kzalloc
 * 3. Associate the device-specific data with the SPI device
 * 4. Return 0 indicating successful probe or appropriate error code
 */

static int rpi4b_spi_probe(struct spi_device *spi)
{
	struct rpi4b_spi_dev *dev;

	dev_info(&spi->dev, "Probing SPI device\n");    // Log the probing message for the SPI device

	spi_dev=spi;

	// Allocate memory for the SPI device-specific data
	dev = devm_kzalloc(&spi->dev, sizeof(struct rpi4b_spi_dev), GFP_KERNEL);
	if (!dev) {
		dev_err(&spi->dev, "Failed to allocate memory\n");  // Log an error if memory allocation fails
		return -ENOMEM;    // Return error code for memory allocation failure
	}

	// Associate the allocated device-specific data with the SPI device
	spi_set_drvdata(spi, dev);
	dev->spi = spi;

	return 0;    // Return 0 to indicate successful probe
}

/**
 * Pseudo code for rpi4b_spi_remove function:
 * 1. Log the removal of the SPI device
 * 2. Clean up resources and free allocated memory (if necessary)
 */

static void rpi4b_spi_remove(struct spi_device *spi)
{
	struct rpi4b_spi_dev *dev = spi_get_drvdata(spi);    // Retrieve the device-specific data associated with the SPI device
	
	dev_info(&spi->dev, "Removing SPI device\n");    // Log the removal message
}

/* SPI Device ID Table */
static const struct spi_device_id rpi4b_spi_id[] = {
	{ "rpi4b_spi_device", 0 },   // Define the SPI device name for matching with the driver
	{ }
};

MODULE_DEVICE_TABLE(spi, rpi4b_spi_id);    // Register the device ID table with the kernel

/* SPI Driver Structure */
static struct spi_driver rpi4b_spi_driver = {
	.driver = {
		.name = DRIVER_NAME,     // Name of the driver, used for identification
		.owner = THIS_MODULE,    // Module owner to manage references
	},
	.probe = rpi4b_spi_probe,    // Probe function to initialize the device
	.remove = rpi4b_spi_remove,  // Remove function to clean up on device removal
	.id_table = rpi4b_spi_id,    // Device ID table to match supported devices
};

/**
 * Pseudo code for spi_open function:
 * 1. Log the opening of the SPI device
 * 2. Return success to indicate that the device is open
 */

static int spi_open(struct inode *inode, struct file *file) {
	pr_info("SPI Device opened\n");
	return 0;
}

/**
 * Pseudo code for spi_close function:
 * 1. Log the closing of the SPI device
 * 2. Return success to indicate that the device is closed
 */

static int spi_close(struct inode *inode, struct file *file) {
	pr_info("SPI Device closed\n");
	return 0;
}

/**
 * Pseudo code for rpi4b_spi_write function:
 * 1. Define a TX buffer to hold data to send
 * 2. Define an RX buffer to receive data
 * 3. Prepare the SPI transfer structure (tx_buf, rx_buf, etc.)
 * 4. Initialize and add the transfer to the SPI message
 * 5. Perform a synchronous SPI transfer
 * 6. Log the received data or error
 * 7. Return success or failure based on transfer result
 */

static ssize_t rpi4b_spi_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {

	char tx_buf[2];               // Create TX buffer with debug data
	char rx_buf[2] = { 0x00 };    // Create RX buffer to hold received data
	int ret;

	struct spi_transfer t;    // SPI transfer structure to hold individual transfer details
	struct spi_message m;     // SPI message structure to hold multiple transfers

	if (copy_from_user(tx_buf, buf, 2)) {
		pr_err("Failed to receive data from user\n");
		return -EFAULT;
	}

	// Prepare an example SPI transfer
	t.tx_buf = tx_buf;    // Set TX buffer for the SPI transfer
	t.rx_buf = rx_buf;    // Set RX buffer for the SPI transfer
	t.len = 2;                  // Set the transfer length (2 bytes)
	t.speed_hz = SPI_MAX_SPEED; // Set SPI speed to the maximum defined speed
	t.bits_per_word = 8;         // Set SPI bits per word (8 bits per transfer)


	// Initialize SPI message and add the transfer
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	// Execute the SPI transfer synchronously
	ret = spi_sync(spi_dev, &m);
	if (ret) {
		dev_err(&spi_dev->dev, "SPI transfer failed: %d\n", ret);    // Log an error if SPI transfer fails
		return ret;    // Return the error code from the transfer failure
	}

	// Log RX data received during SPI transfer
	dev_info(&spi_dev->dev, "SPI transfer successful. RX Data: 0x%02x 0x%02x\n", rx_buf[0], rx_buf[1]);

	return 0;
}

static struct file_operations spi_fops = {
	.owner = THIS_MODULE,
	.open = spi_open,
	.release = spi_close,
	.write = rpi4b_spi_write,
};

/**
 * Pseudo code for rpi4b_spi_init function:
 * 1. Log that the driver is being initialized
 * 2. Register the character device with the kernel
 * 3. Register the SPI driver with the SPI subsystem
 * 4. Return success or error based on registration result
 */

static int __init rpi4b_spi_init(void)
{
	int major_number;
	pr_info("Initializing %s\n", DRIVER_NAME);    // Log that the module is being initialized
	major_number = register_chrdev(0, DRIVER_NAME, &spi_fops);
	if (major_number < 0) {
		printk(KERN_ALERT "simple_device: Failed to register device\n");
		return major_number;
	}
	printk(KERN_INFO "simple_device: Registered with major number %d\n", major_number);

	return spi_register_driver(&rpi4b_spi_driver);    // Register the SPI driver with the subsystem
}

/**
 * Pseudo code for rpi4b_spi_exit function:
 * 1. Log that the driver is being unloaded
 * 2. Unregister the SPI driver
 * 3. Clean up resources
 */

static void __exit rpi4b_spi_exit(void)
{
	pr_info("Exiting %s\n", DRIVER_NAME);    // Log that the module is being unloaded
	spi_unregister_driver(&rpi4b_spi_driver);    // Unregister the SPI driver from the subsystem
}

// Register module initialization and exit functions
module_init(rpi4b_spi_init);    // Register the module init function
module_exit(rpi4b_spi_exit);    // Register the module exit function

MODULE_AUTHOR("TEAM1 && TEAM7");    // Author of the driver
MODULE_DESCRIPTION("SPI Device Driver for Raspberry Pi 4B with DebugFS");    // Driver description
MODULE_LICENSE("GPL");    // License type for the module
MODULE_VERSION("1.0");    // Module version
