#ifndef LED_KM_H
#define LED_KM_H

/* 
 * ===============================================
 *             DDAL LED Data Structures
 * ===============================================
 */

/* Convienient constant name */
#define LED_MODULE_NAME "led"

/* 
 * This header file is used by both kernel code and user code. The
 * portion of the header used by kernel code is concealed from user
 * code by the __KERNEL__ ifdef.
 */
#ifdef __KERNEL__


/*
 * Required Proc File-system Struct
 *
 * Used to map entry into proc file table upon module insertion
 */
extern struct proc_dir_entry *led_proc_entry;

#endif /* __KERNEL__*/

/* 
 * ===============================================
 *             Public API Functions
 * ===============================================
 */

#define LED_COUNT 3


/*
 * There typically needs to be a struct definition for each flavor of
 * IOCTL call.
 */
typedef struct led_ioctl_inc_s {
	int placeholder;
} led_ioctl_inc_t;

/* 
 * This generic union allows us to make a more generic IOCTRL call
 * interface. Each per-IOCTL-flavor struct should be a member of this
 * union.
 */
typedef union led_ioctl_param_u {
	led_ioctl_inc_t      set;
} led_ioctl_param_union;

#define LED_ON     1
#define LED_OFF    2
#define LED_TOGGLE 3


#endif /* LED_KM_H */
