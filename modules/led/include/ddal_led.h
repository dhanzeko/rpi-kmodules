#ifndef DDAL_LED_H
#define DDAL_LED_H

#define DDAL_LED_MAGIC 0xd34db33f

extern int ddal_led_open(void);

extern int ddal_led_close(void);

extern int ddal_led_on(int fd);

extern int ddal_led_off(int fd);

extern int ddal_led_toggle(int fd);

#endif
