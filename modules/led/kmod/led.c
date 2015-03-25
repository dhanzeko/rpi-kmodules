/*
 * helloworld.c - Example kmod utilizing ioctl and procfs
 *
 */
#include <linux/kernel.h>   /* printk() */
#include <linux/slab.h>     /* kmalloc() */
#include <asm/uaccess.h>    /* copy_*_user */
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/cdev.h>

#include "../include/linux/led.h"


#define MODULE_LICENSE_STR      "GPL"
#define MODULE_DESCRIPTION_STR  "Simple LED module example with procfs and IOCTL"
#define MODULE_AUTHOR_STR       "Darije Hanzekovic <darije.hanzekovic@gmail.com>"
#define MODULE_VERSION_STR      "0.1"


#define BUFFER_SIZE    64

/*
 * Required Proc File-system Struct
 *
 * Used to map entry into proc file table upon module insertion
 */
static ssize_t led_proc_read(struct file *file,
        char *buffer,
        size_t buffer_length, loff_t * offset_ptr);

struct proc_dir_entry *led_proc_entry;

static const struct file_operations proc_file_fops = {
    .owner = THIS_MODULE,
    .read = led_proc_read,
};

/*
 * * Struct defining pins, direction and inital state
 * */
static struct gpio leds[] = {
    { 2, GPIOF_OUT_INIT_LOW, "LED0" },
    { 3, GPIOF_OUT_INIT_LOW, "LED1" },
    { 4, GPIOF_OUT_INIT_LOW, "LED2" },
};

struct led_dev {
    int gpiopin;
    char brightness;
    struct timer_list timer;
    struct semaphore lock;
    struct cdev cdev;     /* Char device structure      */
};

struct led_dev *led_devices;

static dev_t firstdev;

//module_param(gpiopins, unsigned int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
//MODULE_PARM_DESC(gpiopins, "A list of GPIO pins LEDs are attached to");


static void led_brightness_set(struct led_dev *dev, int brightness)
{
    dev->brightness = (char) (brightness <= 255 ? brightness : 255);

    if (dev->brightness > 0) {
        gpio_set_value(dev->gpiopin, 1);
    } else {
        gpio_set_value(dev->gpiopin, 0);
    }
}


/* 
 * ===============================================
 *                Public Interface
 * ===============================================
 */

/* 
 * ===============================================
 *                IOCTL Interface
 * ===============================================
 */

/*
 * Generic open call that will always be successful since there is no
 * extra setup we need to do for this module as far
 */
static int led_open(struct inode *inode, struct file *filp)
{
    struct led_dev *dev;

    dev = container_of(inode->i_cdev, struct led_dev, cdev);
    filp->private_data = dev;

    return 0;
}


static int led_close(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t led_read(struct file *filp, char __user *buff, 
        size_t count, loff_t *offp)
{
    struct led_dev *dev = (struct led_dev *)filp->private_data;
    int len = 0;
    char kbuff[BUFFER_SIZE];  
    int retval;

    if (*offp > 0)
        return 0;

    if (down_interruptible(&dev->lock))
        return -ERESTARTSYS;


    sprintf(kbuff, "%d\n", dev->brightness);
    len = strlen(kbuff);
    if (copy_to_user(buff, kbuff, len)) {
        retval = -EFAULT;
        goto out;
    }

    *offp += len;
    retval = len;

out:
    up(&dev->lock);
    return retval;
}

static ssize_t led_write(struct file *filp, const char __user *buff, 
                size_t count, loff_t *offp)
{   
    struct led_dev *dev = (struct led_dev *)filp->private_data;
    int len = 0;
    char kbuff[BUFFER_SIZE] = {0};
    int retval = 0;
    unsigned long brightness;
    int ret;

    if (down_interruptible(&dev->lock))
        return -ERESTARTSYS;

    len = count < (BUFFER_SIZE-1) ? count : BUFFER_SIZE-1;

    if (copy_from_user(kbuff, buff, len)) {
        retval = -EFAULT;
        goto out;
    } 

    *offp += len;
    retval = len;
    kbuff[len] = '\0';

    if ((ret = kstrtoul(kbuff, 0, &brightness)) == 0) {
        led_brightness_set(dev, brightness);
    } else {
        pr_warn("led_write: invalid data, errno %d\n", ret);
    }


out:
    up(&dev->lock);
    return retval;
}  


static long
led_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
    int ret = 0;
    led_ioctl_param_union local_param;

    pr_info("led_ioctl()\n");

    if (copy_from_user
        ((void *) &local_param, (void *) ioctl_param, _IOC_SIZE(ioctl_num)))
        return -ENOMEM;

    switch (ioctl_num) {
        case LED_ON:
            pr_info("led on\n");
            break;
        
        case LED_OFF:
            pr_info("led off\n");

        case LED_TOGGLE:
            pr_info("led toggle\n");
            break;
        
        default:
            pr_err("ioctl: no such command\n");
            ret = -EINVAL;
            break;
    }                           /* end of switch(ioctl_num) */


    return ret;
}




/* 
 * ==============================================gpio_request_array=
 *            Proc File Table Interface
 * ===============================================
 */

/* 
 * The type and order of the parameters is standard for a /proc/ read
 * routine, at least, and possibly for all read routines but we didn't
 * go and check this assuption yet.
 *
 */
static ssize_t
led_proc_read(struct file *file,
              char *buffer, size_t buffer_length, loff_t * offset_ptr)
{
    int ret;
    int offset = *offset_ptr;

    pr_info("reading led proc entry (offset=%d; buflen=%d).\n", offset,
            buffer_length);

    if (offset > 0) {
        /* we have finished to read, return 0 */
        ret = 0;
    } else {
        sprintf(buffer, "%s: %s\n"
                "revision: %s\n"
                "author: %s\n"
                "licence: %s\n"
                "major: %d\n", 
                LED_MODULE_NAME, 
                MODULE_DESCRIPTION_STR,
                MODULE_VERSION_STR, 
                MODULE_AUTHOR_STR,
                MODULE_LICENSE_STR,
                MAJOR(firstdev));

        ret = strlen(buffer);

        *offset_ptr += ret;
    }

    return ret;
}


/* 
 * The file_operations struct is an instance of the standard character
 * device table entry. We choose to initialize only the open, release,
 * and unlock_ioctl elements since these are the only functions we use
 * in this module.
 */
struct file_operations led_dev_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = led_ioctl,
    .open = led_open,
    .release = led_close,
    .read = led_read,
    .write = led_write,
};


/*
 *  * Set up the char_dev structure for this device.
 *   */
static int led_setup_cdev(struct led_dev *dev, int index)
{
    int err, devno = firstdev + index;
            
    sema_init(&dev->lock, 1);
    dev->brightness = 0;
    dev->gpiopin = leds[index].gpio;
    cdev_init(&dev->cdev, &led_dev_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &led_dev_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    /* Fail gracefully if need be */
    if (err)
        pr_err("Error %d adding led%d", err, index);

    return err;
}


/*
 * This gpio_request_arrayroutine is executed when the module is loaded into the
 * kernel. I.E. during the insmod command.
 *
 */
static int __init led_init(void)
{
    int i, j;
    int res = 0;

    res = alloc_chrdev_region(&firstdev, 0, LED_COUNT, LED_MODULE_NAME);
    if (res < 0) {
        pr_warn("led: failed to alloc major\n");
        goto init_major_alloc_fail;
    }

    led_devices = kmalloc(LED_COUNT * sizeof(struct led_dev), GFP_KERNEL);
    if (led_devices == NULL) {
        res = -ENOMEM;
        goto init_dev_alloc_fail;
    }

    for (i=0; i<LED_COUNT; i++) {
        if (led_setup_cdev(&led_devices[i], i) != 0) {
            for (j=0; j<i; j++)
                cdev_del(&led_devices[j].cdev);
            res = -ENOMEM;
            goto init_dev_add_fail;
        }
    }

    /* 
     * Creating an entry in /proc with the module name as the file
     * name (/proc/helloworld). The S_IRUGO | S_IWUGO flags set
     * the permissions to 666 (rw,rw,rw).
     */
    led_proc_entry = proc_create(LED_MODULE_NAME,
                                 S_IRUGO | S_IWUGO, NULL, &proc_file_fops);

    if (led_proc_entry == NULL) {
        /*
         * When the create_proc_entry fails we return a No
         * Memory Error. Whether ENOMEM is accurate or a
         * approximate is separate issue but we chose to use
         * an existing error number.
        include/linux/string.h */
        res = -ENOMEM;
        goto init_proc_create_fail;
    }

    // register gpios
    res = gpio_request_array(leds, ARRAY_SIZE(leds));
    if (res) {
        pr_err("Unable to request GPIOs: %d\n", res);
        goto init_gpio_alloc_fail;
    }   

    pr_info("led module installed from proc=%s with pid=%d\n",
            current->comm, current->pid);

    return 0;


init_gpio_alloc_fail:
    remove_proc_entry(LED_MODULE_NAME, NULL);
init_proc_create_fail:
    for (i=0; i<LED_COUNT; i++)
        cdev_del(&led_devices[i].cdev);
init_dev_add_fail:
    kfree(led_devices);
init_dev_alloc_fail:
    unregister_chrdev_region(firstdev, LED_COUNT);
init_major_alloc_fail:
    return res;
}

static void __exit led_exit(void)
{
    int i;

    gpio_free_array(leds, ARRAY_SIZE(leds));

    remove_proc_entry(LED_MODULE_NAME, NULL);

    for (i=0; i<LED_COUNT; i++)
        cdev_del(&led_devices[i].cdev);

    unregister_chrdev_region(firstdev, LED_COUNT);

    pr_info("led module uninstalled from proc=%s with pid=%d\n",
            current->comm, current->pid);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE(MODULE_LICENSE_STR);
MODULE_DESCRIPTION(MODULE_DESCRIPTION_STR);
MODULE_AUTHOR(MODULE_AUTHOR_STR);
MODULE_VERSION(MODULE_VERSION_STR);
