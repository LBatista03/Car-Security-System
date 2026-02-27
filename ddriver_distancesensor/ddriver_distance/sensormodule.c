#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include <linux/ioctl.h>
#include <linux/kthread.h>

#include "utils.h"

#define RD_VALUE _IOR('a','a',int32_t*)

#define DEVICE_NAME "sensor_hc-sr04"
#define CLASS_NAME "sensorClass"

MODULE_DESCRIPTION("HC-SR04 Ultrasonic Sensor Driver with Timer");
MODULE_LICENSE("GPL");

static struct class* sensorDevice_class = NULL;
static struct device* sensorDevice_device = NULL;
static dev_t sensorDevice_majorminor;
static struct cdev c_dev;

static struct timer_list sensor_timer;

static const int TRIGGpioPin = 23; /**< TRIG pin */
static const int ECHOGpioPin = 24; /**< ECHO pin */

struct GpioRegisters *s_pGpioRegisters;

unsigned long distance=0;
static int flag = 0;              // Flag to trigger callback
static struct task_struct *sensor_thread; // Kernel thread
DECLARE_WAIT_QUEUE_HEAD(wq);     // Wait queue for thread synchronization

/**
 * @brief Timer callback function for periodic measurements
 */
 
void sensor_timer_callback(struct timer_list *t) {
    flag = 1; 
    wake_up_interruptible(&wq); 
    pr_info("Sensor Timer: Triggered\n");
    mod_timer(&sensor_timer, jiffies + msecs_to_jiffies(50));
}
 
static int sensor_thread_fn(void *data) {
    while (!kthread_should_stop()) {
        wait_event_interruptible(wq, flag == 1);

        if (kthread_should_stop()){
            pr_info("kthread stopped\n");
            break;
	}
        flag = 0;

        ktime_t start_time, end_time, duration;

        // Trigger the sensor
        SetGPIOOutputValue(s_pGpioRegisters, TRIGGpioPin, 1);
        udelay(10);
        SetGPIOOutputValue(s_pGpioRegisters, TRIGGpioPin, 0);

        // Measure start time
        start_time = ktime_get();

        // Wait for ECHO pin to go HIGH
        ktime_t timeout = ktime_add_us(start_time, 10000); // 10 ms timeout
        while (GetGPIOInputValue(s_pGpioRegisters, ECHOGpioPin) == 0) {
            if (ktime_after(ktime_get(), timeout)) {
                pr_alert("Timeout waiting for ECHO HIGH\n");
                break;
            }
        }
        start_time = ktime_get();

        // Wait for ECHO pin to go LOW
        timeout = ktime_add_us(start_time, 10000); // 10 ms timeout
        while (GetGPIOInputValue(s_pGpioRegisters, ECHOGpioPin) == 1) {
            if (ktime_after(ktime_get(), timeout)) {
                pr_alert("Timeout waiting for ECHO LOW\n");
                break;
            }
        }
        end_time = ktime_get();

        // Calculate distance
        duration = ktime_sub(end_time, start_time); // Pulse width
        duration = ktime_to_us(duration);          // Convert to microseconds
        distance = (duration * 34300) / 2000000;   // cm
        pr_info("HC-SR04: Duration = %lu us\n", duration);
        pr_info("HC-SR04: Distance = %lu cm\n", distance);
    }

    pr_info("Sensor Thread: Exiting thread\n");
    return 0;
}

/**
 * @brief Device open function
 */
int sensor_device_open(struct inode* p_inode, struct file *p_file) {
    pr_alert("%s: called\n", __FUNCTION__);
    p_file->private_data = (struct GpioRegisters *)s_pGpioRegisters;
    
    return 0;
}

/**
 * @brief Device close function
 */
int sensor_device_close(struct inode *p_inode, struct file *p_file) {
    pr_alert("%s: called\n", __FUNCTION__);
    p_file->private_data = NULL;
    return 0;
}

/**
 * @brief Device read function
 * @details Returns the last measured distance to user space
 */
ssize_t sensor_device_read(struct file *p_file, char __user *p_buff, size_t len, loff_t *poffset) {
    char buf[2];
    int value = GetGPIOInputValue(s_pGpioRegisters, ECHOGpioPin);
    buf[0] = value ? '1' : '0';
    buf[1] = '\0';

    return sizeof(buf);
}

/**
 * @brief Device write function
 * @details Set or clear Trigger Pin
 */
ssize_t sensor_device_write(struct file *p_file, const char __user *p_buff, size_t len, loff_t *poffset) {
    char buf[2];
    buf[len] = '\0';
    if(buf[0]){
    	SetGPIOOutputValue(s_pGpioRegisters, TRIGGpioPin, 1);
    }else{
    	SetGPIOOutputValue(s_pGpioRegisters, TRIGGpioPin, 0);
    }
    
    return len;
}


static long sensor_device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
         switch(cmd) {
                case RD_VALUE:
                        if( copy_to_user((int32_t*) arg, &distance, sizeof(distance)) )
                        {
                                pr_err("Data Read : Err!\n");
                        }
                        break;
                default:
                        pr_info("Default\n");
                        break;
        }
        return 0;
}

static struct file_operations sensorDevice_fops = {
    .owner = THIS_MODULE,
    .open = sensor_device_open,
    .release = sensor_device_close,
    .read = sensor_device_read,
    .write = sensor_device_write,
    .unlocked_ioctl = sensor_device_ioctl,
};




/**
 * @brief Module initialization
 */
static int __init sensorModule_init(void) {
    int ret;
    struct device *dev_ret;

    pr_alert("%s: called\n", __FUNCTION__);

    // Allocate character device memory
    if ((ret = alloc_chrdev_region(&sensorDevice_majorminor, 0, 1, DEVICE_NAME)) < 0) {
        return ret;
    }

    // Create device class
    if (IS_ERR(sensorDevice_class = class_create(CLASS_NAME))) {
        unregister_chrdev_region(sensorDevice_majorminor, 1);
        return PTR_ERR(sensorDevice_class);
    }

    // Create device
    if (IS_ERR(dev_ret = device_create(sensorDevice_class, NULL, sensorDevice_majorminor, NULL, DEVICE_NAME))) {
        class_destroy(sensorDevice_class);
        unregister_chrdev_region(sensorDevice_majorminor, 1);
        return PTR_ERR(dev_ret);
    }

    cdev_init(&c_dev, &sensorDevice_fops);
    c_dev.owner = THIS_MODULE;

    if ((ret = cdev_add(&c_dev, sensorDevice_majorminor, 1)) < 0) {
        device_destroy(sensorDevice_class, sensorDevice_majorminor);
        class_destroy(sensorDevice_class);
        unregister_chrdev_region(sensorDevice_majorminor, 1);
        return ret;
    }

    s_pGpioRegisters = (struct GpioRegisters *)ioremap(GPIO_BASE, sizeof(struct GpioRegisters));
    pr_alert("GPIO mapped to virtual address: 0x%x\n", (unsigned)s_pGpioRegisters);

    pr_alert("Configuring TRIGGpioPin as output\n");
    SetGPIOFunction(s_pGpioRegisters, TRIGGpioPin, 1);  // Set TRIG as output
    SetGPIOOutputValue(s_pGpioRegisters, TRIGGpioPin, 0);   // Ensure TRIG is low
    pr_alert("Configuring ECHOGpioPin as input\n");
    SetGPIOFunction(s_pGpioRegisters, ECHOGpioPin, 0);  // Set ECHO as input 
  
    timer_setup(&sensor_timer, sensor_timer_callback, 0);
    mod_timer(&sensor_timer, jiffies + msecs_to_jiffies(5000)); // 5 seconds

    // Start the kernel thread
    sensor_thread = kthread_run(sensor_thread_fn, NULL, "sensor_thread");
    if (IS_ERR(sensor_thread)) {
        pr_alert("Failed to create kernel thread\n");
        del_timer(&sensor_timer); // Cleanup timer
        return PTR_ERR(sensor_thread);
    }
    
    pr_alert("HC-SR04: Module initialized and timer started\n");
	
    return 0;
}

/**
 * @brief Module cleanup
 */
static void __exit sensorModule_exit(void) {
    del_timer(&sensor_timer);
    kthread_stop(sensor_thread);

    SetGPIOFunction(s_pGpioRegisters, TRIGGpioPin, 0b000);
    iounmap(s_pGpioRegisters);

    cdev_del(&c_dev);
    device_destroy(sensorDevice_class, sensorDevice_majorminor);
    class_destroy(sensorDevice_class);
    unregister_chrdev_region(sensorDevice_majorminor, 1);

    pr_alert("HC-SR04: Module exited\n");
}

module_init(sensorModule_init);
module_exit(sensorModule_exit);
