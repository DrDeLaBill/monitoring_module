# Monitoring module

Monitoring module is a device that regulates the amount of reagent supply to the liquid by pump, monitors the amount of liquid in the tank, logs pressure and level sensors state and sends recorded values to the server. 
Device can be configurated from server and from UART port by special commands.

### UART commands:

- Standart commands:
    - ```status``` - shows current settings
    - ```saveadcmin``` - saves current liquid sensor value as min value (this value is inverse - the higher the value, the less liquid)
    - ```saveadcmax``` - saves current liquid sensor value as max value (this value is inverse - the lower the value, the more liquid)
    - ```default``` - sets default settings
    - ```clearlog``` - removes old log
    - ```clearpump``` - clears pump work total and work day time 
    - ```pump``` - shows current pump state
    - ```reset``` - removes all log (doesn't work now)
    - ```setid <uint32_t id>``` - sets new module id
    - ```setsleep <uint32_t time>``` - sets log frequency (in seconds)
    - ```seturl <string url>``` - sets server url
    - ```setport <string port>``` - sets server port
    - ```setlitersmin <uint32_t liters>``` - sets tank min level (in milliliters)
    - ```setlitersmax <uint32_t liters>``` - sets tank max level (in milliliters)
    - ```settarget <uint32_t liters>``` - sets target liquid capacity per day (in milliliters)
    - ```setpumpspeed <uint32_t liters>``` - sets pump speed (in milliliters per hour)
    - ```setlogid <uint32_t id>``` - set last log id that must be sended to the server
    - ```delrecord <uint32_t id>``` - removes log record from storage by id
    - ```setpower <bool enabled>``` - allows/forbids pump work
- Debug commands:
    - ```reseteepromerr``` - resets EEPROM status and error bits
    - ```setadcmin <uint32_t adc_val>``` - sets liquid value as min ADC value (this value is inverse - the higher the value, the less liquid)
    - ```setadcmin <uint32_t adc_val>``` - sets liquid value as min ADC value (this value is inverse - the lower the value, the more liquid)
    - ```setconfigver <uint32_t cf_id>``` - sets custom config version that sends to server


### POST request example:

The device sends POST request to server like:
```
POST /api/log/ep HTTP/1.1
Host: (IP/URL)
User-Agent: module
Connection: close
Accept-Charset: utf-8, us-ascii
Content-Type: text/plain
Content-Length: 100

id=123
fw_id=2
cf_id=1
t=2000-01-01T00:00:01
d=id=5;level=0.0;press_1=0.61;pumpw=100;pumpd=100;t=2000-01-01T00:00:01
```
or without log:
```
POST /api/log/ep HTTP/1.1
Host: (IP/URL)
User-Agent: module
Connection: close
Accept-Charset: utf-8, us-ascii
Content-Type: text/plain
Content-Length: 100

id=123
fw_id=2
cf_id=1
t=2000-01-01T00:00:01
```
- ```id``` - curernt device id
- ```fw_id``` - current firmware version
- ```cf_id``` - current configuration vresion
- ```t``` - current device time
- ```d``` - log data
    - ```level``` - log liquid level (liters)
    - ```press_1``` - log first pressure sensor value (MPa)
    - ```pumpw``` - log pump work time (sec)
    - ```pumpd``` - log pump downtime (sec)
    - ```t``` - log time


POST resoponse expected from server:
```
HTTP/1.1 200 OK
server: nginx/1.18.0
date: t
referrer-policy: same-origin
cross-origin-opener-policy: same-origin

t=2023-01-19t12:56:09.386436
d_hwm=0
cf_id=33077616
cf=id=123;ltrmin=10;ltrmax=375;trgt=10000;sleep=900;speed=1600;clr=0;pwr=1;logid=1
```

- ```t``` - current time
- ```d_hwm``` - last log id on server
- ```cf_id``` - current server configuration (updates if module settings changes on server)
- ```cf``` - new config data
    - ```id``` - new module id
    - ```ltrmin``` - tank min level (liters)
    - ```ltrmax``` - tank max level (liters)
    - ```trgt``` - target liguid capacity per day (in milliliters)
    - ```sleep``` - log frequency (seconds)
    - ```speed``` - pump speed (milliliters per hour)
    - ```clr``` - remove old log (doesn't work now)
    - ```pwr``` - allows/forbids pump work (bool)
