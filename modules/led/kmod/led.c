/*
 * helloworld.c - Example kmod utilizing ioctl and procfs
 *
 */
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/types.h>

#include "../include/linux/led.h"

static ssize_t led_proc_read(struct file *file,
                                char *buffer,
                                size_t buffer_length,
                                loff_t *offset_ptr);

/*
 * Required Proc File-system Struct
 *
 * Used to map entry into proc file table upon module insertion
 */
struct proc_dir_entry *led_proc_entry;

const struct file_operations proc_file_fops = {
    .owner = THIS_MODULE,
    .read  = led_proc_read,
};

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
static int
led_open(struct inode *inode, struct file *file) {
	return 0;
}


static int
led_close(struct inode *inode, struct file *file) {
	return 0;
}

static long
led_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
	int ret = 0;
	led_ioctl_param_union local_param;

	pr_info("led_ioctl()\n");

	if (copy_from_user
	    ((void *)&local_param, (void *)ioctl_param, _IOC_SIZE(ioctl_num)))
		return  -ENOMEM;

	switch (ioctl_num) {

	case LED_TOGGLE:
	{
		pr_info("toggle_led\n");
		
		break;
	}

	default:
	{
		pr_err("ioctl: no such command\n");
		ret = -EINVAL;
	}
	} /* end of switch(ioctl_num) */

	
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
static ssize_t led_proc_read(struct file *file,
                                char *buffer,
				                size_t buffer_length,
				                loff_t *offset_ptr) {
	int ret;
    int offset = *offset_ptr;

	pr_info("reading led proc entry.\n");
	
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
		ret  = 0;
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
		 * Make sure we are starting off with a clean buffer;
		 */
		strcpy(buffer, "LED driver");

		ret = strlen(buffer);

        *offset_ptr += strlen(buffer);
	}

	return ret;
}


/* 
 * The file_operations struct is an instance of the standard character
 * device table entry. We choose to initialize only the open, release,
 * and unlock_ioctl elements since these are the only functions we use
 * in this module.
 */
struct file_operations
led_dev_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = led_ioctl,
	.open           = led_open,
	.release        = led_close,
};

/* 
 * This is an instance of the miscdevice structure which is used in
 * the helloworld_init routine as a part of registering the module
 * when it is loaded.
 * 
 * The device type is "misc" which means that it will be assigned a
 * static major number of 10. We deduced this by doing ls -la /dev and
 * noticed several different entries we knew to be modules with major
 * number 10 but with different minor numbers.
 * 
 */
static struct miscdevice
led_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = LED_MODULE_NAME,
	.fops  = &led_dev_fops,
};


/*
 * This routine is executed when the module is loaded into the
 * kernel. I.E. during the insmod command.
 *
 */
static int
__init led_init(void)
{
	int ret = 0;

	/*
	 * Attempt to register the module as a misc. device with the
	 * kernel.
	 */
	ret = misc_register(&led_misc);

	if (ret < 0) {
		/* Registration failed so give up. */
		goto out;
	}

	/* 
	 * Creating an entry in /proc with the module name as the file
	 * name (/proc/helloworld). The S_IRUGO | S_IWUGO flags set
	 * the permissions to 666 (rw,rw,rw).
	 */
	led_proc_entry = proc_create_data(LED_MODULE_NAME, 
						  S_IRUGO | S_IWUGO, NULL, &proc_file_fops, NULL);

	if (led_proc_entry == NULL) {
		/*
		 * When the create_proc_entry fails we return a No
		 * Memory Error. Whether ENOMEM is accurate or a
		 * approximate is separate issue but we chose to use
		 * an existing error number.
		 */
		ret = -ENOMEM;
		goto out;
	}

	pr_info("led module installed\n");



out:
	return ret;
}

static void
__exit led_exit(void)
{ 
    remove_proc_entry(LED_MODULE_NAME, led_proc_entry);

	misc_deregister(&led_misc);

	printk("led module uninstalled\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple LED module example with procfs and IOCTL");
MODULE_AUTHOR("Darije Hanzekovic");
