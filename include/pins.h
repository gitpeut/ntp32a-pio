#ifndef PINS_H
#define PINS_H

//i2c bus pins
#define SDApin     4
#define SCLpin     5
#define SWITCH_PIN 2

//GPS pins
#define PULSE_PIN 36 // SENSE_VP, Input for PPS clock pulse on pin 36
#define RXPin     32 // GPS serial
#define TXPin     33

/* Obligatory ethernet pins used:

GPIO22 - EMAC_TXD1   : TX1
GPIO19 - EMAC_TXD0   : TX0
GPIO21 - EMAC_TX_EN  : TX_EN
GPIO26 - EMAC_RXD1   : RX1
GPIO25 - EMAC_RXD0   : RX0
GPIO27 - EMAC_RX_DV  : CRS
GPIO00 - EMAC_TX_CLK : nINT/REFCLK (50MHz) - 4k7 Pullup
*GPIO23 - SMI_MDC     : MDC
*GPIO18 - SMI_MDIO    : MDIO
*GPIO13 - PHY_POWER   : NC - Osc. Enable - 4k7 Pulldown
pins with * are freely configurable


*/

// Pin# of the enable signal for the external crystal oscillator (-1 to disable for internal APLL source)
#define ETH_POWER_PIN   13
// Pin# of the I²C clock signal for the Ethernet PHY
#define ETH_MDC_PIN     23
// Pin# of the I²C IO signal for the Ethernet PHY
#define ETH_MDIO_PIN    18

#endif
