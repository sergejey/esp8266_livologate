# esp8266_livologate
Control livolo switch with ESP8266+rf433 sender

LivoloTX library: https://github.com/bitlinker/LivoloTx

433RF module data connected to D0 pin.

Default access point password: 1111

MQTT can be configured to report status.

Usage: HTTP GET requests to send Livolo buttons commands

http://IP_ADDRESS/rfkey?button=BUTTON_NUM_1-10&remote=REMOTE_ID

Example:
http://IP_ADDRESS/rfkey?button=1&remote=8500
http://IP_ADDRESS/rfkey?button=2&remote=8500

All off command:
http://IP_ADDRESS/rfkey?button=off&remote=8500