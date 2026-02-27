#include "utils.h"
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/printk.h>

void SetGPIOFunction(struct GpioRegisters *s_pGpioRegisters,
                     int GPIO,
                     int functionCode) {
    int registerIndex = GPIO / 10;
    int bit = (GPIO % 10) * 3;

    unsigned oldValue = s_pGpioRegisters->GPFSEL[registerIndex];
    unsigned mask = 0b111 << bit;

    /**< Print alert messages */
    pr_alert("%s: register index is %d\n",
             __FUNCTION__, registerIndex);
  pr_alert("%s: mask is 0x%x\n",
             __FUNCTION__, mask);
    pr_alert("%s: update value is 0x%x\n",
             __FUNCTION__, ((functionCode << bit) & mask));

    /**< Select the function of the GPIO */
    s_pGpioRegisters->GPFSEL[registerIndex] =
        (oldValue & ~mask) | ((functionCode << bit) & mask);
}

int GetGPIOInputValue(struct GpioRegisters *s_pGpioRegisters, int GPIO) {
    int registerIndex = (~(GPIO / 32)) & 1;
    int bit = GPIO % 32;
    uint32_t value = s_pGpioRegisters->GPLEV[registerIndex];
    
    return (value >> bit) & 1;
}

void SetGPIOOutputValue(struct GpioRegisters *s_pGpioRegisters, int GPIO, bool outputValue) {
    int registerIndex = (GPIO / 32);
    int bit = GPIO % 32;
			 
	if (outputValue)
		s_pGpioRegisters->GPSET[registerIndex] = (1 << bit);
	else
		s_pGpioRegisters->GPCLR[registerIndex] = (1 << bit);
}
