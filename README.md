# esp8266_livologate
Control livolo switch with ESP8266+rf433 sender

LivoloTX library: https://github.com/bitlinker/LivoloTx

433RF module data connected to D0 pin.

Default access point password: 1111

MQTT can be configured to report status.

Usage: HTTP GET requests to send Livolo buttons commands

http://<IP>/rfkey?button=<BUTTON_NUM 1-10>&remote=<REMOTE_ID>

Example:
http://<IP>/rfkey?button=1&remote=8500
http://<IP>/rfkey?button=2&remote=8500

All off command:
http://<IP>/rfkey?button=off&remote=8500