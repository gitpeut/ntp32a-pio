#ifndef CONFIG_ETHERNET_H

#include <Arduino.h>
#include <WiFi.h>
#include "pins.h"


#ifndef ETH_PHY_TYPE
#define ETH_PHY_TYPE  ETH_PHY_LAN8720
#define ETH_PHY_ADDR  1
#define ETH_PHY_MDC   ETH_MDC_PIN
#define ETH_PHY_MDIO  ETH_MDIO_PIN
#define ETH_PHY_POWER ETH_POWER_PIN 
#define ETH_CLK_MODE  ETH_CLOCK_GPIO0_IN
#endif

#include <WifiUDP.h>
#include <ETH.h>
#include "globals.h"

extern bool eth_connected;
void StartEthernet();

#endif
