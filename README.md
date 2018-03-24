<html>
<body>
# control modem chip TCM3105

1. WiFi SSID and password configuration

"make menuconfig" and select "WiFi SSID/pass configuration" menu.
set SSID and password of the access-point.

2. pin connection between ESP-WROOM-32 and TCM3105

# MCPWM
IO12 -> TRS <br>
IO32 <- RXD <br>
IO27 <- CLK

# LEDC
IO15 -> OSC1

# RMT
IO02 -> TXD

# GPIO
IO19 -> LED <br>
IO14 <- CDT

# DAC
IO25 -> RXB <br>
IO26 -> CDL

# TCM3105
TXA -> RXA
</body>
</html>
