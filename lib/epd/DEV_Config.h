/***************************************************
//Web: http://www.buydisplay.com
EastRising Technology Co.,LTD
****************************************************/
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "Debug.h"


#ifdef RPI
    #ifdef USE_BCM2835_LIB
        #include <bcm2835.h>
    #elif USE_WIRINGPI_LIB
        #include <wiringPi.h>
        #include <wiringPiSPI.h>
    #elif USE_DEV_LIB
        #include "RPI_sysfs_gpio.h"
        #include "dev_hardware_SPI.h"
    #endif
#endif

#ifdef JETSON
    #ifdef USE_DEV_LIB
        #include "sysfs_gpio.h"    
        #include "sysfs_software_spi.h"
    #elif USE_HARDWARE_LIB
        
    #endif

#endif

/**
 * data
**/
#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

/**
 * GPIOI config
**/
extern int EPD_RST_PIN;
extern int EPD_DC_PIN;
extern int EPD_CS_PIN;
extern int EPD_BUSY_PIN;

/*------------------------------------------------------------------------------------------------------*/
void DEV_Digital_Write(UWORD Pin, UBYTE Value);
UBYTE DEV_Digital_Read(UWORD Pin);

void DEV_SPI_WriteByte(UBYTE Value);
void DEV_SPI_Write_nByte(uint8_t *pData, uint32_t Len);
void DEV_Delay_ms(UDOUBLE xms);

UBYTE DEV_Module_Init(void);
void DEV_Module_Exit(void);


#endif
