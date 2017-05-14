#ifndef __CONFIGS_H__
#define __CONFIGS_H__

#define MEASURE_PERIOD 10
#define TEMPTHRES 25
#define DEFAULT_PORT 8998

/*
 * Radiator 1 : openwrt TPLink
 */
#ifdef CONFIG_RADIATOR1
#define SWAP_ENDIAN
#define CONFIG_DS1820
#define GPIO_SW 7

/* led in /sys/class/leds/ */
#define LED2 "tp-link:green:wps"  /* gpio 26 */
#define LED3 "tp-link:green:3g"   /* gpio 27 */
#define LED4 "tp-link:green:wlan" /* gpio 0 */
#define LED5 "tp-link:green:lan"  /* gpio 17 */
#endif

/*
 * Radiator 2 : raspberry pi3
 */
#ifdef CONFIG_RADIATOR2
#define CONFIG_DS1820
//#define CONFIG_SHT1x
#define GPIO_SW  27
#define GPIO_LED 2
#define GPIO_BUTTON 17

#ifdef CONFIG_SHT1x
// GPIO Pins for the SHT1x
#define RPI_GPIO_SHT1x_SCK  3
#define RPI_GPIO_SHT1x_DATA 4
#endif

#endif

#endif
