/**
  ******************************************************************************
  * @file    usb_bsp.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    19-September-2011
  * @brief   This file is responsible to offer board support package and is
  *          configurable by user.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "rcc.h"
#include "usb_bsp.h"
#include "usbd_conf.h"
#include "core_cm4.h"


void USB_OTG_BSP_ConfigVBUS ( void ) {

}

void USB_OTG_BSP_DriveVBUS  ( uint8_t state ) {

}


/**
* @brief  USB_OTG_BSP_Init
*         Initilizes BSP configurations
* @param  None
* @retval None
*/
#define OTGAF 0x0Au

void USB_OTG_BSP_Init ( void ) {
    //GPIO_InitTypeDef GPIO_InitStructure;


    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOA;

    GPIOA->OSPEEDR |= ( ( 3u << 16 ) | ( 3u << 18 ) | ( 3u << 22 ) | ( 3u << 24 ) );
    GPIOA->MODER   &= ~ ( ( 3u << 16 ) | ( 3u << 18 ) | ( 3u << 22 ) | ( 3u << 24 ) );
    GPIOA->MODER   |= ( ( 2u << 16 ) | ( 2u << 18 ) | ( 2u << 22 ) | ( 2u << 24 ) );

    GPIOA->OTYPER  &= ~ ( ( 1u <<  8 ) | ( 1u <<  9 ) | ( 1u << 11 ) | ( 1u << 12 ) );
    GPIOA->PUPDR   &= ~ ( ( 3u << 16 ) | ( 3u << 18 ) | ( 3u << 22 ) | ( 3u << 24 ) );

    GPIOA->AFR[1] &= ~ ( ( 0xFu  << 0 ) | ( 0xFu  << 4 ) | ( 0xFu  << 12 ) | ( 0xFu  << 16 ) );
    GPIOA->AFR[1] |= ( ( OTGAF << 0 ) | ( OTGAF << 4 ) | ( OTGAF << 12 ) | ( OTGAF << 16 ) );
    /* this for ID line debug */

    GPIOA->OSPEEDR |= ( ( 3u << 20 ) );
    GPIOA->MODER   &= ~ ( ( 3u << 20 ) );
    GPIOA->MODER   |= ( ( 2u << 20 ) );

    GPIOA->OTYPER  |= ( ( 1u << 10 ) );
    GPIOA->PUPDR   &= ~ ( ( 3u << 20 ) );
    GPIOA->PUPDR   |= ~ ( ( 1u << 20 ) );
    //GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_OTG1_FS) ;

    GPIOA->AFR [10u >> 3] &= ~ ( 0xFu << ( ( 10u & 0x7u ) << 2 ) );
    GPIOA->AFR [10u >> 3] |= ( OTGAF << ( ( 10u & 0x7u ) << 2 ) );

    RCC->AHB2ENR |= RCC_APB2Periph_SYSCFG;
    RCC->AHB2ENR |= RCC_AHB2Periph_OTG_FS;
    /* enable the PWR clock */
    //RCC_APB1PeriphResetCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC->AHB1ENR |= RCC_APB1Periph_PWR;

    /* Configure the Key button in EXTI mode */
    //STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_EXTI);

    EXTI->PR = ( 1u << 0 );
}
/**
* @brief  USB_OTG_BSP_EnableInterrupt
*         Enabele USB Global interrupt
* @param  None
* @retval None
*/
void USB_OTG_BSP_EnableInterrupt ( void ) {
    NVIC_SetPriority ( OTG_FS_IRQn, 1 ); // ?
    NVIC_EnableIRQ ( OTG_FS_IRQn );

}
/**
* @brief  USB_OTG_BSP_uDelay
*         This function provides delay time in micro sec
* @param  usec : Value of delay required in micro sec
* @retval None
*/
void USB_OTG_BSP_uDelay ( const uint32_t usec ) {
    register volatile uint32_t count = 0;
    const uint32_t utime = ( ( 120 * usec ) / 7 );
    do {
        if ( ++count > utime ) {
            return ;
        }
    } while ( 1 );
}


/**
* @brief  USB_OTG_BSP_mDelay
*          This function provides delay time in milli sec
* @param  msec : Value of delay required in milli sec
* @retval None
*/
void USB_OTG_BSP_mDelay ( const uint32_t msec ) {
    USB_OTG_BSP_uDelay ( msec * 1000 );
}
