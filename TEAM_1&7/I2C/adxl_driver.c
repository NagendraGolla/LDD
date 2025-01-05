#include <linux/module.h>      // Required for module definitions
#include <linux/init.h>        // Required for init and exit macros
#include <linux/i2c.h>         // Required for I2C functions
#include <linux/kernel.h>      // Required for kernel functions (e.g., pr_info)
#include <linux/fs.h>          // Required for file system operations
#include <linux/cdev.h>        // Required for character device operations
#include <linux/uaccess.h>     // Required for user space access functions (copy_to_user)

// Define constants for the driver
#define AVAILABLE_RPI_I2C_BUS   1       // I2C bus number on the Raspberry Pi
#define I2C_SLAVE_ADR           0x53    // I2C address of the ADXL345 accelerometer
#define CLIENT_NAME             "adxl_client_pi4" // Name of the I2C client device
#define DEVICE_NAME             "my_i2c_dev"     // Name of the driver

// Declare global variables
static struct i2c_adapter *pi_i2c_adap = NULL;      //abstraction of the device connected to the i2c bus
static struct i2c_client *adxl_i2c_client = NULL;   // Pointer to the I2C client
static int major_number;                            // Major number for the character device
static struct cdev my_cdev;                         // Character device structure






// Pseudo-code for File Operations:
// OPEN: Do nothing.
// RELEASE: Do nothing.
// READ:
// 1. Resume runtime PM.
// 2. Send register address to read from ADXL345.
// 3. Receive data from ADXL345.
// 4. Copy data to user space.
// 5. Mark device as last busy and allow autosuspend.

// File Operations
static int my_open(struct inode *inode, struct file *file)
{
    return 0; // Always successful
}

static int my_release(struct inode *inode, struct file *file)
{
    return 0; // Always successful
}

static ssize_t my_read(struct file *file, char __user *user_buf, size_t count, loff_t *off)
{
    int ret;
    char tx_buf = 0x32;  // Address of data registers
    char rx_buf[6];      // Buffer for received data


    ret = i2c_master_send(adxl_i2c_client, &tx_buf, 1); // Send register address
    if (ret < 0) {
        pr_err("Setting register address failed: %d\n", ret); // Print error if failed
    	return ret;
    }

    ret = i2c_master_recv(adxl_i2c_client, rx_buf, count); // Receive data
    if (ret < 0) {
        pr_err("Receiving failed: %d\n", ret); // Print error if failed
    	return ret;
    }

    if (copy_to_user(user_buf, rx_buf, count)) { // Copy to user space
        pr_err("Copying to user space failed\n"); // Print error if failed
        ret = -EFAULT; // Set error code
    	return ret;
    }

    return count; // Return number of bytes read
}

// File Operations Structure
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
};







// Pseudo-code for Probe Function:
// 1. Configure ADXL345 (set data format).
// 2. Activate and enable runtime PM.
// 3. Set autosuspend delay and enable autosuspend.

// I2C Probe Function
static int adxl_probe(struct i2c_client *client)
{
    int ret;
    char data[2] = {0x31, 0x04}; // DATA_FORMAT register, left justified

    if (i2c_master_send(adxl_i2c_client, data, 2) < 0) { // Configure ADXL345
        pr_err("Failed to set DATA_FORMAT register\n"); // Print error if failed
        return -EFAULT; // Return error code
    }
    
    data[0] = 0x2D; // POWER_CTL register 
    data[1] = 0x08; // Measure Mode 
    ret = i2c_master_send(adxl_i2c_client, data, 2); // Send I2C command
    if (ret < 0) {
        pr_err("Failed to set POWER_CTL for resume: %d\n", ret); // Print error if failed
        return ret; // Return error code
    }

    pr_info("Probe function called and ADXL345 initialized\n"); // Print message
    return 0; // Return success
}

// I2C Remove Function
static void adxl_remove(struct i2c_client *client)
{
    pr_info("%s: removed!\n", CLIENT_NAME); // Print message
}






// I2C Device ID Table
static const struct i2c_device_id pi_id[] = {
    {CLIENT_NAME, 0}, // Match any I2C address for this client name
    {} // Null terminator
};
MODULE_DEVICE_TABLE(i2c, pi_id); // Declare device table


// Define the driver structure with name, owner, PM operations, probe, remove, and ID table.

// I2C Driver Structure
static struct i2c_driver adxl_driver = {
    .driver = {
        .name = CLIENT_NAME,      // Name of the driver
        .owner = THIS_MODULE,     // Owner of the driver (this module)
    },
    .probe = adxl_probe,          // Probe function (called when a matching device is found)
    .remove = adxl_remove,        // Remove function (called when the driver is unloaded or the device is removed)
    .id_table = pi_id,            // I2C device ID table (used for device matching)
};


// Pseudo-code for I2C Board Info:
// Define the board info structure to describe the I2C device to the kernel.

// I2C Board Info (used for static device declaration)
static struct i2c_board_info pi_i2c_board_info = {
    I2C_BOARD_INFO(CLIENT_NAME, I2C_SLAVE_ADR), //macro used to list an i2c device and its address also
                                                //initializes essential fields of i2c_board_info structure
};

// Pseudo-code for Driver Initialization:
// 1. Get the I2C adapter.
// 2. Create a new I2C client device.
// 3. Allocate a character device region.
// 4. Initialize and add the character device.
// 5. Add the I2C driver.
// 6. Print a message to the kernel log on success.
// ERROR HANDLING: Use goto statements to unwind and cleanup if any step fails.

// Driver Initialization Function
static int __init adxl_driver_init(void)
{
    int ret = 0;

    pi_i2c_adap = i2c_get_adapter(AVAILABLE_RPI_I2C_BUS); // Get the I2C adapter for the specified bus
    if (!pi_i2c_adap) {
        pr_err("%s: Failed to get the i2c_adapter for the i2c%d bus!\n", CLIENT_NAME, AVAILABLE_RPI_I2C_BUS); // Print error
        return -ENODEV; // Return error code (No such device)
    }

    adxl_i2c_client = i2c_new_client_device(pi_i2c_adap, &pi_i2c_board_info); // Create a new I2C client device
    if (!adxl_i2c_client) {
        pr_err("%s: Failed to register the i2c_client!\n", CLIENT_NAME); // Print error
        ret = -ENODEV; // Set error code
        goto err_adapter; // Jump to error handling
    }

    ret = alloc_chrdev_region(&major_number, 0, 1, DEVICE_NAME); // Allocate a range of character device numbers
    if (ret < 0) {
        pr_err("Failed to allocate char device region\n"); // Print error
        goto err_client; // Jump to error handling
    }

    cdev_init(&my_cdev, &fops); // Initialize the character device structure
    ret = cdev_add(&my_cdev, major_number, 1); // Add the character device to the system
    if (ret < 0) {
        pr_err("Failed to add char device\n"); // Print error
        goto err_region; // Jump to error handling
    }

    ret = i2c_add_driver(&adxl_driver); // Add the I2C driver to the I2C subsystem
    if (ret) {
        pr_err("Failed to add i2c driver.\n"); // Print error
        goto err_cdev; // Jump to error handling
    }

    pr_info("Device registered with major number %d\n", MAJOR(major_number)); // Print success message with major number
    return 0; // Return success

// Error Handling (Unwinding and Cleanup)
err_cdev:
    cdev_del(&my_cdev); // Delete the character device
err_region:
    unregister_chrdev_region(major_number, 1); // Unregister the character device region
err_client:
    i2c_unregister_device(adxl_i2c_client); // Unregister the I2C client device
err_adapter:
    i2c_put_adapter(pi_i2c_adap); // Put the i2c adapter (decrement refcount)

    return ret; // Return the error code
}

// Pseudo-code for Driver Exit:
// 1. Delete the character device.
// 2. Unregister the character device region.
// 3. Unregister the I2C client device.
// 4. Unregister the I2C driver.
// 5. Print a message to the kernel log.

// Driver Exit Function
static void __exit adxl_driver_exit(void)
{
    cdev_del(&my_cdev); // Delete the character device
    unregister_chrdev(MAJOR(major_number), DEVICE_NAME); // Unregister the char device region
    i2c_unregister_device(adxl_i2c_client); // Unregister the I2C client
    i2c_del_driver(&adxl_driver); // Remove the I2C driver
    i2c_put_adapter(pi_i2c_adap); // Put the i2c adapter (decrement refcount)
    pr_info("I2c Driver Removed!\n"); // Print a message to the kernel log
}

module_init(adxl_driver_init); // Register the initialization function
module_exit(adxl_driver_exit); // Register the exit function

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Team7");
MODULE_DESCRIPTION("ADXL345 device driver");
MODULE_VERSION("1.0")
