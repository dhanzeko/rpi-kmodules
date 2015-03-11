/* led.c */
#include <linux/module.h>   
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/gpio.h>

#define LED1 2

static struct timer_list blink_timer;

MODULE_LICENSE("GPL");

/*
 *  * Timer function called periodically
 *   */
static void blink_timer_func(unsigned long data)
{
    printk(KERN_INFO "%s\n", __func__);

    gpio_set_value(LED1, data); 
                
                /* schedule next execution */
    blink_timer.data = !data;                       // makes the LED toggle 
    blink_timer.expires = jiffies + (1*HZ);         // 1 sec.
    add_timer(&blink_timer);
}


static int __init blinkled_init(void)
{
    int ret = 0;

    printk(KERN_INFO "Blinkled driver init\n");

    
    ret = gpio_request_one(LED1, GPIOF_OUT_INIT_LOW, "led1");

    if (ret) {
        printk(KERN_ERR "Unable to request GPIOs: %d\n", ret);
        return ret;
    }


    init_timer(&blink_timer);

    blink_timer.function = blink_timer_func;
    blink_timer.data = 1L;
    blink_timer.expires = jiffies + (1*HZ);
    add_timer(&blink_timer);

    return 0;
}

static void __exit blinkled_exit(void)
{
    printk(KERN_INFO "Blinkled driver exit\n");

    // deactivate timer if running
    del_timer_sync(&blink_timer);

    // turn the led off
    gpio_set_value(LED1, 0);

    // unregister gpio
    gpio_free(LED1);
}

module_init(blinkled_init);
module_exit(blinkled_exit);
