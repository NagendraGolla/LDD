// Pseudo-code Overview:
// 1. Initialize platform driver, register V4L2 device, video device and mutex.
// 2. On probe (device detected), allocate memory for camera device, 
//    register V4L2 and video device, and associate operations.
// 3. On remove (device removed), unregister devices and free memory.
// 4. Implement file operations: open, release, and query capabilities.

// Including necessary header files
#include <linux/module.h>                // Provides the kernel module interface (e.g., init/exit functions).
#include <linux/kernel.h>                // Provides functions for kernel logging and other utilities.
#include <linux/init.h>                  // For module initialization and cleanup.
#include <linux/platform_device.h>       // Allows interaction with platform devices.
#include <linux/videodev2.h>             // Defines the Video4Linux2 API.
#include <media/v4l2-device.h>           // Interface for V4L2 device management.
#include <media/v4l2-ioctl.h>            // Defines ioctl operations for V4L2.

#define DRIVER_NAME "simple_camera"       // Macro defining the driver name, used for logging and naming.

struct simple_camera_dev {              // Structure representing the camera device.
    struct v4l2_device v4l2_dev;         // V4L2 device structure for registration with V4L2 subsystem.
    struct video_device video_dev;       // Video device structure for managing video capture.
    struct mutex lock;                  // Mutex lock for synchronization, protecting shared data.
    bool streaming;                      // Flag indicating whether the camera is streaming.
};

static int simple_camera_open(struct file *file) {
    // This function handles opening the device.
    struct simple_camera_dev *dev = video_drvdata(file);  // Retrieve the device structure associated with the file.
    
    if (!mutex_trylock(&dev->lock))      // Attempt to acquire the mutex; if it is locked, return -EBUSY.
        return -EBUSY;

    printk(KERN_INFO "%s: Device opened\n", DRIVER_NAME);  // Log that the device has been opened.
    return 0;   // Return 0 to indicate success.
}

static int simple_camera_release(struct file *file) {
    // This function handles releasing the device.
    struct simple_camera_dev *dev = video_drvdata(file);  // Retrieve the device structure.

    mutex_unlock(&dev->lock);   // Release the lock when the device is released.
    printk(KERN_INFO "%s: Device released\n", DRIVER_NAME);  // Log that the device has been released.
    return 0;  // Return 0 to indicate success.
}

static int simple_camera_querycap(struct file *file, void *priv,
                                  struct v4l2_capability *cap) {
    // Handles the VIDIOC_QUERYCAP ioctl to retrieve device capabilities.
    strlcpy(cap->driver, DRIVER_NAME, sizeof(cap->driver));  // Copy driver name to cap->driver.
    strlcpy(cap->card, "Simple Camera Device", sizeof(cap->card));  // Set device name in cap->card.
    snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s", DRIVER_NAME);  // Set platform bus info.
    return 0;  // Return 0 to indicate success.
}

static const struct v4l2_ioctl_ops simple_camera_ioctl_ops = {
    .vidioc_querycap = simple_camera_querycap,  // Register the VIDIOC_QUERYCAP ioctl handler.
};

static const struct v4l2_file_operations simple_camera_fops = {
    .owner = THIS_MODULE,              // Set ownership of file operations to this module.
    .open = simple_camera_open,        // Register the open function.
    .release = simple_camera_release,  // Register the release function.
    .unlocked_ioctl = video_ioctl2,    // Register ioctl handler.
};

static int simple_camera_probe(struct platform_device *pdev) {
    struct simple_camera_dev *dev;  // Pointer to hold the device structure.
    int ret;                        // Return value for error checking.

    dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);  // Allocate memory for the device structure.
    if (!dev)                      // If memory allocation fails, return -ENOMEM.
        return -ENOMEM;

    mutex_init(&dev->lock);         // Initialize the mutex lock for device synchronization.

    ret = v4l2_device_register(&pdev->dev, &dev->v4l2_dev);  // Register the V4L2 device with the system.
    if (ret)                        // If registration fails, return the error code.
        return ret;

    strlcpy(dev->video_dev.name, DRIVER_NAME, sizeof(dev->video_dev.name));  // Set video device name.
    dev->video_dev.fops = &simple_camera_fops;  // Associate the file operations with the video device.
    dev->video_dev.ioctl_ops = &simple_camera_ioctl_ops;  // Associate ioctl operations with the video device.
    dev->video_dev.v4l2_dev = &dev->v4l2_dev;  // Link the V4L2 device with the video device.
    dev->video_dev.lock = &dev->lock;  // Assign the lock to the video device.
    dev->video_dev.release = video_device_release;  // Set the release function for the video device.

    ret = video_register_device(&dev->video_dev, VFL_TYPE_GRABBER, -1);  // Register the video device.
    if (ret) {                      // If device registration fails, unregister V4L2 device and return error.
        v4l2_device_unregister(&dev->v4l2_dev);
        return ret;
    }

    platform_set_drvdata(pdev, dev);  // Associate the device structure with the platform device.
    printk(KERN_INFO "%s: Driver initialized\n", DRIVER_NAME);  // Log the successful driver initialization.
    return 0;  // Return 0 to indicate success.
}

static int simple_camera_remove(struct platform_device *pdev) {
    struct simple_camera_dev *dev = platform_get_drvdata(pdev);  // Retrieve the device structure associated with the platform device.

    video_unregister_device(&dev->video_dev);  // Unregister the video device.
    v4l2_device_unregister(&dev->v4l2_dev);  // Unregister the V4L2 device.
    printk(KERN_INFO "%s: Driver removed\n", DRIVER_NAME);  // Log the removal of the driver.
    return 0;  // Return 0 to indicate successful removal.
}

static struct platform_driver simple_camera_driver = {
    .driver = {
        .name = DRIVER_NAME,  // Set the name of the driver.
    },
    .probe = simple_camera_probe,  // Register the probe function to handle device detection.
    .remove = simple_camera_remove,  // Register the remove function for clean-up during device removal.
};

module_platform_driver(simple_camera_driver);  // Register the platform driver with the kernel.

MODULE_LICENSE("GPL");  // Specify the license type of the module (GPL).
MODULE_AUTHOR("TEAM1");  // Author of the module.
MODULE_DESCRIPTION("Simple V4L2 Camera Driver");  // Description of the driver functionality.
MODULE_VERSION("1.0");  // Version of the module.

