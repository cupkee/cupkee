/* Linker script for an STM32F103RC board (256KB flash, 48KB RAM). */

/* Define memory regions. */
MEMORY
{
	rom (rx) : ORIGIN = 0x0801E000, LENGTH = 120K
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 48K
}

/* Include the common ld script. */
INCLUDE libopencm3_stm32f1.ld

