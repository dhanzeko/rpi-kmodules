/*
 * helloworld.c - Example kmod utilizing ioctl and procfs
 *
 */
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/gpio.h>

#include "../include/linux/led.h"

static ssize_t led_proc_read(struct file *file,
                             char *buffer,
                             size_t buffer_length, loff_t * offset_ptr);

#define LED_ON_STR "on"
#define LED_OFF_STR "off"

/*
 * Required Proc File-system Struct
 *
 * Used to map entry into proc file table upon module insertion
 */
struct proc_dir_entry *led_proc_entry;

const struct file_operations proc_file_fops = {
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

//module_param(gpiopins, unsigned int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
//MODULE_PARM_DESC(gpiopins, "A list of GPIO pins LEDs are attached to");

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
static int led_open(struct inode *inode, struct file *file)
{
    return 0;
}


static int led_close(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t led_read(struct file *filep, char __user *buff, 
        size_t count, loff_t *offp)
{
    pr_info("led_read()\n");
    return 0;
}

static ssize_t led_write(struct file *filep, const char __user *buff, 
                size_t count, loff_t *offp)
{   
    pr_info("led_write()\n");
    return 0;
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
 * ===============================================
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

    /* 
     * We give all of our information in one go, so if the user
     * asks us if we have more information the answer should
     * always be no.
     *
     * This is important because the standard read function from
     * the library would continue to issue the read system call
     * until the kernel replies that it has no more information,
     * or until its buffer is filled.
     * 
     */
    if (offset > 0) {
        /* we have finished to read, return 0 */
        ret = 0;
    } else {
        /* fill the buffer, return the buffeint buffer_lengthr size This
         * assumes that the buffer passed in is big enough to
         * hold what we put in it. More defensive code would
         * check the input buffer_length parameter to check
         * the validity of that assumption.
         *
         * Note that the return value from this call (number
         * of characters written to the buffer) from this will
         * be added to the current offset at the file
         * descriptor level managed by the system code that is
         * called by this routine.
         */

        /* 
         * Make sure we are stinclude/linux/string.harting off with a clean buffer;
         */
        strcpy(buffer, "LED driver\n");

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

dev_t firstdev;

/*
 * This routine is executed when the module is loaded into the
 * kernel. I.E. during the insmod command.
 *
 */
static int __init led_init(void)
{
    int ret = 0;

    alloc_chrdev_region(&firstdev, 0, LED_COUNT, LED_MODULE_NAME);


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
        ret = -ENOMEM;
        goto out;
    }

    // register gpios
    ret = gpio_request_array(leds, ARRAY_SIZE(leds));
    if (ret) {
        pr_err("Unable to request GPIOs: %d\n", ret);
    }   

    pr_info("led module installed\n");

  out:
    return ret;
}

static void __exit led_exit(void)
{
    gpio_free_array(leds, ARRAY_SIZE(leds));

    remove_proc_entry(LED_MODULE_NAME, NULL);

    unregister_chrdev_region(firstdev, LED_COUNT);

    pr_info("led module uninstalled\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple LED module example with procfs and IOCTL");
MODULE_AUTHOR("Darije Hanzekovic");
