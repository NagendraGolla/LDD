#include<linux/module.h>
#include<linux/kernel.h>

// module intialization function
static int __init basic_driver_init(void)
{
printk(KERN_INFO "Basic Driver Loaded\n");
return 0;// return 0 indicates sucsessful loading
}

//module cleanup function
static void __exit basic_driver_exit(void)
{
printk(KERN_INFO "Basic Driver Is Unloaded\n");
}

//register modules intialization and cleanup functions
module_init(basic_driver_init);
module_exit(basic_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NAGENDRA");
MODULE_DESCRIPTION("A Basic Linux Devide Driver");
