#ifndef __CONFIGS_H__
#define __CONFIGS_H__

#define MEASURE_PERIOD 10 /* sec between measurement */
#define STORE_PERIOD (60*5) /* sec between db store */
#define TEMPTHRES 25
#define DEFAULT_PORT 8998

/*
 * Radiator 1 : openwrt TPLink
 */
#ifdef CONFIG_RADIATOR1
#define MODULE_ID 1
#define SWAP_ENDIAN
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
#define MODULE_ID 2
#define GPIO_SW 4
#define GPIO_LED 11
#define GPIO_BUTTON 17

#ifdef CONFIG_SHT1x
// GPIO Pins for the SHT1x
#define RPI_GPIO_SHT1x_SCK  3
#define RPI_GPIO_SHT1x_DATA 4
#endif

#endif /* RADIATOR2 */

#ifndef MODULE_ID
#define MODULE_ID 0 /* default */
#endif

#endif
