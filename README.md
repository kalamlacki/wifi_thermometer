Wifi digital thermometer

This project allows on reading temperature over http protocol.
It assumes that you have NODEmcu based on esp8266 chip and to the pin number 2 
you have connected the DQ pin of DS18B20 chip, power pin connected to 3.3V PIN
and GND to GND.
After uploading to board it runs at open access point mode with SSID equal to lk_e8266_thermo
You can connect using putty/minicom by USB cable to the board and read acess point ip. Later
you can use this ip to connect to the board and configure it in client mode in which the device will
be working under controll of your local access point.

To configure board to use wifi client mode use this link (works in AP and client mode):
http://<borad.ip>
You will see simple website with 3 inputs (SSID name, password, and alias of the device)
and 2 buttons: one to save config and the second to read existing setup.
If you make a mistake during configuration you can always connect to the board using putty or minicom
and press "R" button which will put back the board back to WiFi AP mode.

To read temperature:
http://<board.ip>/temp

on the screen you will get plain text response like this:

DS18B20 Temperature=20.06C

Which later can be used in automatic script which controll the temperature


If you like the project please donate over paypal to lukasz@pm.kalamlacki.eu.

