/*
                                    PSEUDO-CODE
1. Define constants and structures for I2C operations.
2.Declare global variables for I2C adapter, I2C client, and the character device.
3. Define file operations (open, read, release) for the character device.
4. Implement the probe function to initialize and configure the I2C device.
5. Implement the remove function to clean up the I2C device when removed.
6. Implement initialization function to set up I2C adapter, client, and character device.
7. Implement exit function to clean up resources during module unload.
8. Register the I2C driver and character device.*/


#include <linux/module.h>       // For defining kernel modules
#include <linux/init.h>         // For module initialization and exit
#include <linux/slab.h>         // For memory allocation
#include <linux/i2c.h>          // For I2C functions and structures
#include <linux/i2c-smbus.h>    // For I2C SMBus communication
#include <linux/kernel.h>       // For printk and kernel-level functions
#include <linux/fs.h>           // For character device operations
#include <linux/cdev.h>         // For character device structure and management
#include <linux/uaccess.h>      // For user-space access functions

// Define constants for the I2C bus and slave address
#define AVAILABLE_RPI_I2C_BUS  1            // Raspberry Pi I2C bus number (usually 1)
#define I2C_SLAVE_ADR          0x53         // Slave I2C address (example: 0x53 for ADXL345)
#define CLIENT_NAME            "i2c_client_pi4"  // Name of the I2C client device
#define DEVICE_NAME            "my_i2c_dev"  // Name for the character device

// Declare global variables for the I2C adapter and client, character device, etc.
static struct i2c_adapter *pi_i2c_adap = NULL;  // Pointer to I2C adapter
static struct i2c_client *pi_i2c_client = NULL; // Pointer to I2C client device
static int major_number;  // Major number for the character device
static struct cdev my_cdev;  // Structure for the character device
static char data[6];  // Data buffer to hold the data to/from the device

// The open function for the character device (called when the device is opened)
static int my_open(struct inode *inode, struct file *file)
{
    // This function is typically used for setup, but here it's left empty.
    return 0;  // Success
}

// The release function for the character device (called when the device is closed)
static int my_release(struct inode *inode, struct file *file)
{
    // This function is typically used for cleanup, but here it's left empty.
    return 0;  // Success
}

// The read function for the character device
// This will be called when user space tries to read from the device
static ssize_t my_read(struct file *file, char __user *user_buf, size_t count, loff_t *off)
{
    // Set the register address to read from (0x32 is example for ADXL345)
    data[0] = 0x32;

    // Send the register address to the I2C device
    if ((i2c_master_send(pi_i2c_client, data, 1)) < 0) {
        pr_err("setting register address failed\n");
        return -EFAULT;  // Return error if sending the register address fails
    }
    pr_info("register address sent\n");

    // Set up data buffer for receiving 6 bytes of data
    data[0] = 0x00;  // Clear the register address in the buffer

    // Receive 6 bytes of data from the I2C device
    if ((i2c_master_recv(pi_i2c_client, data, 6)) < 0) {
        pr_err("receiving failed\n");
        return -EFAULT;  // Return error if receiving data fails
    }

    pr_info("data received\n");

    // Copy the data from kernel space to user space
    if (copy_to_user(user_buf, data, 6)) {
        pr_err("copying to user space failed\n");
        return -EFAULT;  // Return error if copying fails
    }

    return count;  // Return the number of bytes read
}

// File operations structure (function pointers for device operations)
static const struct file_operations fops = {
    .owner = THIS_MODULE,      // This module owns the file operations
    .open = my_open,           // Open function
    .release = my_release,     // Release function
    .read = my_read,           // Read function
};

// Probe function: Called when the I2C device is detected and registered
static int pi_probe(struct i2c_client *client)
{
    // Set the data format register to a specific value (example for ADXL345)
    data[0] = 0x31;
    data[1] = 0x04;
    if ((i2c_master_send(pi_i2c_client, data, 2)) < 0) {
        pr_err("setting DATA FORMAT register failed\n");
        return -EFAULT;  // Return error if setting data format fails
    }

    // Set the power control register (example for ADXL345)
    data[0] = 0x2D;
    data[1] = 0x08;
    if ((i2c_master_send(pi_i2c_client, data, 2)) < 0) {
        pr_err("setting POWER CONTROL register failed\n");
        return -EFAULT;  // Return error if setting power control fails
    }

    pr_info("Probe function is called and ADXL345 is initialized\n");
    return 0;  // Success
}

// Remove function: Called when the I2C device is removed
static void pi_remove(struct i2c_client *client)
{
    pr_info("%s: removed!\n", CLIENT_NAME);
}

// Define the I2C device ID table (mapping device names to device IDs)
static const struct i2c_device_id pi_id[] = {
    {CLIENT_NAME, 0},    // Register the device name and ID
    {}                   // Null entry to terminate the list
};
MODULE_DEVICE_TABLE(i2c, pi_id);  // Inform the I2C subsystem of the device IDs

// Define the I2C driver structure
static struct i2c_driver pi_driver = {
    .driver = {
        .name = CLIENT_NAME,  // The name of the driver
        .owner = THIS_MODULE, // Owner of the driver
    },
    .probe = pi_probe,       // Probe function
    .remove = pi_remove,     // Remove function
    .id_table = pi_id,       // Device ID table
};

// Define the I2C board information for the client device
static struct i2c_board_info pi_i2c_board_info = {
    I2C_BOARD_INFO(CLIENT_NAME, I2C_SLAVE_ADR), // Set the board info (name, address)
};

// Initialization function for the driver module
static int __init pi_driver_init(void)
{
    int ret = 0;

    // Get the I2C adapter for the specified I2C bus (bus 1)
    pi_i2c_adap = i2c_get_adapter(AVAILABLE_RPI_I2C_BUS);
    if (!pi_i2c_adap) {
        pr_err("%s: Failed to get the i2c_adapter for the i2c%d bus!\n", CLIENT_NAME, AVAILABLE_RPI_I2C_BUS);
        return -ENODEV;  // Return error if I2C adapter is not found
    }

    // Register the I2C client device
    pi_i2c_client = i2c_new_client_device(pi_i2c_adap, &pi_i2c_board_info);
    if (!pi_i2c_client) {
        pr_err("%s: Failed to register the i2c_client!\n", CLIENT_NAME);
        ret = -ENODEV;
        goto err_adapter;
    }

    // Allocate a character device region (dynamic major number allocation)
    ret = alloc_chrdev_region(&major_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("Failed to allocate char device region\n");
        goto err_client;
    }

    // Initialize and add the character device
    cdev_init(&my_cdev, &fops);
    ret = cdev_add(&my_cdev, major_number, 1);
    if (ret < 0) {
        pr_err("Failed to add char device\n");
        goto err_region;
    }

    // Add the I2C driver
    ret = i2c_add_driver(&pi_driver);
    if (ret) {
        pr_err("Failed to add i2c driver.\n");
        goto err_cdev;
    }

    pr_info("Device registered with major number %d\n", MAJOR(major_number));
    return 0;

err_cdev:
    cdev_del(&my_cdev);
err_region:
    unregister_chrdev_region(major_number, 1);

err_client:
    i2c_unregister_device(pi_i2c_client);
err_adapter:
    return ret;
}

// Exit function for the driver module
static void __exit pi_driver_exit(void)
{
    // Cleanup the character device
    cdev_del(&my_cdev);
    unregister_chrdev(MAJOR(major_number), DEVICE_NAME);

    // Unregister the I2C client and driver
    i2c_unregister_device(pi_i2c_client);
    i2c_del_driver(&pi_driver);

    pr_info("I2c Driver Removed!\n");
}

// Register the module initialization and exit functions
module_init(pi_driver_init);
module_exit(pi_driver_exit);

// Module metadata
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Team7");
MODULE_DESCRIPTION("Basic Implementation of I2C Linux Device Driver");
MODULE_VERSION("1.0");

