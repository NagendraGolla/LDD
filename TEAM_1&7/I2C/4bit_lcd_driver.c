/*
Pseudo Code:
1. Include necessary headers for I2C communication, file operations, and kernel module development.
2. Define the I2C bus and LCD address.
3. Declare variables to store I2C client, adapter, and file operations.
4. Implement functions to send commands and data to the LCD.
5. Initialize the LCD by sending specific commands via I2C.
6. Implement functions for I2C communication handling (write operations).
7. Define probe and remove functions for the I2C driver.
8. Implement the initialization and exit functions for the module, including driver registration.
9. Provide the necessary file operations to interact with the device (write data to LCD).
10. Register the I2C driver and initialize the module.

*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/i2c-smbus.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/uaccess.h>

#define I2C_BUS 1                // I2C bus number
#define LCD_ADDR 0x27            // I2C address of the LCD
#define CLIENT_NAME "I2C-BASED LCD" // Name of the client device

// Declare variables for I2C communication, device file, and buffers
static struct i2c_adapter *lcd_adapter = NULL;
static struct i2c_client *lcd_client = NULL;
static int major;              // Major number for character device
char dev_buf[256];             // Buffer to hold data to be written to the LCD

// Function to send a nibble of data to the LCD
static int lcd_nibble(unsigned char nibble, bool is_data)
{
    unsigned char data = nibble << 4; // Shift nibble to upper 4 bits

    if (is_data)
    {
        data |= 0x01; // RS = 1 to indicate data
    }

    data |= 0x04; // EN = 1 to enable the LCD for the operation

    // Write the data to the LCD using I2C SMBus write byte operation
    i2c_smbus_write_byte(lcd_client, data);
    udelay(1); // Short delay

    data &= ~0x04; // EN = 0 to disable the LCD after operation

    // Write the data again to complete the operation
    i2c_smbus_write_byte(lcd_client, data);

    return 0; // Return success
}

// Function to send data (character) to the LCD
static void lcd_data(unsigned char data)
{
    lcd_nibble(data >> 4, true);  // Send the higher nibble of data
    lcd_nibble(data & 0x0F, true); // Send the lower nibble of data
}

// Function to send a command to the LCD
static void lcd_command(unsigned char cmd)
{
    lcd_nibble(cmd >> 4, false);  // Send the higher nibble of command
    lcd_nibble(cmd & 0x0F, false); // Send the lower nibble of command
}

// LCD initialization function
static int lcd_init(void)
{
    msleep(40); // Delay for initialization of the LCD

    lcd_command(0x02); // Command to home the cursor
    lcd_command(0x28); // Set 4-bit mode, 2 lines, 5x8 font
    lcd_command(0x0F); // Turn on display, cursor, and blinking
    lcd_command(0x06); // Set increment cursor, no shift
    lcd_command(0x01); // Clear the display

    msleep(2); // Wait for LCD to process the clear command

    pr_info("%s: Initialized\n", CLIENT_NAME); // Log the initialization

    return 0; // Return success
}

// Probe function for detecting and initializing the LCD
static int lcd_probe(struct i2c_client *client)
{
    pr_info("I2C slave has been detected\n");
    pr_info("probe function invoked\n");

    // Initialize the display with necessary commands
    lcd_init();

    return 0; // Return success
}

// Remove function to handle cleanup when the device is removed
static void lcd_remove(struct i2c_client *client)
{
    pr_info("In remove\n");
    lcd_command(0x01); // Clear the display before removing the driver
}

// Define I2C device IDs to match the LCD driver
static const struct i2c_device_id lcd_id[] = {
    {.name = CLIENT_NAME},
    { }
};
MODULE_DEVICE_TABLE(i2c, lcd_id);// it adds the lcd device id to i2c bus.

// Define the I2C driver structure
static struct i2c_driver lcd_driver = {
    .driver = {
        .name = CLIENT_NAME,    // Name of the driver
        .owner = THIS_MODULE,   // Owner of the driver
    },
    .probe = lcd_probe,          // Probe function when device is detected
    .remove = lcd_remove,        // Remove function when device is removed
    .id_table = lcd_id,          // Device IDs supported by this driver
};

// Function to write data from user-space to the LCD
static ssize_t i2c_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    // Copy the data from user-space buffer to device buffer
    if (copy_from_user(dev_buf, buf, len))
    {
        pr_info("Unable to copy from user\n");
        return -EINVAL; // Return error if copy fails
    }
    dev_buf[len] = '\0'; // Null-terminate the device buffer

    pr_info("device_buffer data: %s\n", dev_buf); // Log the received data

    int i = 0;
    static char data[20];
    
    lcd_command(0x01); // Clear the display
    msleep(2);

    // Loop through each character in the buffer and send it to the LCD
    for (i = 0; dev_buf[i]; i++)
    {
        lcd_data(dev_buf[i]); // Send the character as data to the LCD
    }

    return len; // Return the number of bytes written
}

// Define file operations for the character device
struct file_operations fops = {
    .write = i2c_write, // Assign the write function for the device
};

// I2C board information
static struct i2c_board_info lcd_board_info = {
    I2C_BOARD_INFO(CLIENT_NAME, LCD_ADDR), // Set the client name and LCD I2C address
};

// Initialize the LCD driver
static int __init lcd_driver_init(void)
{
    int ret;
    
    // Register the character device with a dynamic major number
    major = register_chrdev(0, "i2c_lcd", &fops);
    pr_info("Registered with major number: %d\n", major);

    // Get the I2C adapter for the defined bus
    lcd_adapter = i2c_get_adapter(I2C_BUS);
    if (!lcd_adapter)
    {
        pr_err("%s: Failed to get the I2C adapter for the I2C%d bus!\n", CLIENT_NAME, I2C_BUS);
        ret = -ENODEV; // Return error code for no device found
        goto err_out;
    }

    // Create a new I2C client for the LCD device
    lcd_client = i2c_new_client_device(lcd_adapter, &lcd_board_info);
    if (!lcd_client)
    {
        pr_err("%s: Failed to get the I2C client for the I2C%d bus!\n", CLIENT_NAME, I2C_BUS);
        ret = -1; // Return error code for I2C client creation failure
        goto err_remove_driver;
    }

    // Register the I2C driver
    i2c_add_driver(&lcd_driver);
    if (ret)
    {
        pr_err("%s: Failed to register the I2C driver!\n", CLIENT_NAME);
        goto err_put_adapter;
    }

    pr_info("Driver registered!\n");
    return 0; // Return success

err_remove_driver:
    i2c_del_driver(&lcd_driver);
err_put_adapter:
    i2c_put_adapter(lcd_adapter);
err_out:
    return ret; // Return failure code
}

// Exit function for cleaning up the LCD driver
static void __exit lcd_driver_exit(void)
{
    i2c_unregister_device(lcd_client); // Unregister the I2C client
    i2c_del_driver(&lcd_driver);       // Delete the I2C driver
    unregister_chrdev(major, "i2c_lcd"); // Unregister the character device
    pr_info("Driver removed!\n"); // Log the driver removal
}

// Register the initialization and exit functions
module_init(lcd_driver_init);
module_exit(lcd_driver_exit);

MODULE_LICENSE("GPL"); // Specify the license of the module
