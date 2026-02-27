/**
 * @file utils.h
 * @author Jose Pires, Silviane Correia, Sofia Alves
 * @date 2022-10-04
 *
 * @brief Provides a set of defs and utility functions for the DD
 *
 * GPIO pins are controlled by:
 *  - GPFSELx function select registers
 *  - GPFSETx/GPFCLRx registers to set/clear pins
 */
#include <linux/types.h>

#define BCM2708_PERI_BASE       0xfe000000
#define GPIO_BASE (BCM2708_PERI_BASE + 0x200000) // GPIO controller


/**
 * @brief GPIO registers struct matching Rasp specs 
 * <a href="https://datasheets.raspberrypi.com/bcm2711/bcm2711-peripherals.pdf"> Rasp4 Datasheet</a> 
 */
struct GpioRegisters
{
    uint32_t GPFSEL[6]; /**< Select registers */
    uint32_t Reserved1;
    uint32_t GPSET[2]; /**< Set registers */
    uint32_t Reserved2;
    uint32_t GPCLR[2]; /**< Clear registers */
    uint32_t GPLEV[2];  /**< Level registers to read pin state */
};

/**
 * @brief Sets the function of the GPIO
 * @param s_pGpioRegisters: ptr to a GpioRegisters struct [in]
 * @param GPIO: pin nr [in]
 * @param funcionCode: code of the function to set [in]
 *
 * Can be used to define pin as input/output
 */
void SetGPIOFunction(struct GpioRegisters *s_pGpioRegisters, int GPIO, int functionCode);

/**
 * @brief Sets the GPIO's output value
 * @param s_pGpioRegisters: ptr to a GpioRegisters struct [in]
 * @param GPIO: pin nr [in]
 */
int GetGPIOInputValue(struct GpioRegisters *s_pGpioRegisters, int GPIO);

/**
 * @brief Sets the GPIO's output value
 * @param s_pGpioRegisters: ptr to a GpioRegisters struct [in]
 * @param GPIO: pin nr [in]
 * @param outputValue: logical value of the pin's output[in]
 */
void SetGPIOOutputValue(struct GpioRegisters *s_pGpioRegisters, int GPIO, bool outputValue);

