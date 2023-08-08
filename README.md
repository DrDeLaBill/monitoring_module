# Monitoring module

Monitoring module is a device that regulates the amount of reagent supply to the liquid by pump, monitors the amount of liquid in the tank, logs pressure and level sensors state and sends recorded values to the server. 
Device can be configurated from server and from UART port by special commands.

## README Translation
- [English](README.md)
- [Русский](README.ru.md)

### Status indication

The device has an external single-color lamp and an internal two-colors LED, that indicates the internal device state.

| State | Internal LED | External lamp |
| --- | --- | --|
| Pump disabled | Red LED: 6 sec - off, 300 msec - on; green LED: off | lamp: 6 sec - off, 300 msec - on |
| Pump enabled | Red LED: on; green LED: off | lamp: on |
| Pump works | Red LED: 1 sec - off, 1 sec - on; green LED: 1 sec - on, 1 sec - off | lamp: 1 sec - off, 1 sec - on |
| Internal error | Yellow color | - |

### Calculation of the pump work time:

The calculation of the working time occurs at a certain interval and the device starts the pump for this time (which should be no more than this interval).
The work time is calculated according to the parameters set by the user.
The device calculate pump work time using the following formulas:

![img](https://github.com/DrDeLaBill/monitoring_module/assets/40359652/799f6bd4-4b38-43ea-82ec-da083f9f2810)
- V<sub>dused</sub>  - pump used day liquid [ml]
- t<sub>wday</sub> - pump work today [sec]
- U<sub>p</sub> - pump speed [ml/hour]

![img](https://github.com/DrDeLaBill/monitoring_module/assets/40359652/affad669-8753-4d57-8f92-6ef8b3f1e6ae)
- N<sub>p</sub> - number of periods until the end of the day
- Δt - number of seconds until the end of the day [sec]
- t<sub>pp</sub> - pump work period (default 900) [sec]

![img](https://github.com/DrDeLaBill/monitoring_module/assets/40359652/ad8cc8b8-e089-4def-940a-c5bdc2c2a946)
- V<sub>p</sub> - needed liquid volume in this period [ml]
- V<sub>dneed</sub> - needed day liquid volume [ml]

![img](https://github.com/DrDeLaBill/monitoring_module/assets/40359652/1188ced4-4314-4b3c-af24-26b66a4621fa)
- t<sub>w</sub> - pump period work time [sec]


If t<sub>w</sub> > t<sub>pp</sub> then t<sub>w</sub> = t<sub>pp</sub>
  
If t<sub>w</sub> < t<sub>pmin</sub> then t<sub>w</sub> = 0
  
t<sub>pmin</sub> - pump work period min (default 30) \[sec\]


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
    - ```settarget <uint32_t liters>``` - sets target liquid capacity per day (in liters)
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
    - ```id``` - log id
    - ```level``` - log liquid level (liters)
    - ```press_1``` - log first pressure sensor value (MPa)
    - ```pumpw``` - log pump work time (sec)
    - ```pumpd``` - log pump downtime (sec)
    - ```t``` - log time


### POST resoponse expects from server example:

If the device has a different ```cf_id``` than the server:
```
HTTP/1.1 200 OK
server: nginx/1.18.0
date: t
referrer-policy: same-origin
cross-origin-opener-policy: same-origin

t=2023-01-19t12:56:09.386436
d_hwm=0
cf_id=33077616
cf=id=123;ltrmin=10;ltrmax=375;trgt=20;sleep=900;speed=1600;clr=0;pwr=1;logid=1
```

If the device has the same ```cf_id``` with the server:
```
HTTP/1.1 200 OK
server: nginx/1.18.0
date: t
referrer-policy: same-origin
cross-origin-opener-policy: same-origin

t=2023-01-19t12:56:09.386436
d_hwm=0
```

- ```t``` - current time
- ```d_hwm``` - last log id on server
- ```cf_id``` - current server configuration (updates if module settings changes on server)
- ```cf``` - new config data
    - ```id``` - new module id
    - ```ltrmin``` - tank min level (liters)
    - ```ltrmax``` - tank max level (liters)
    - ```trgt``` - target liguid capacity per day (in liters)
    - ```sleep``` - log frequency (seconds)
    - ```speed``` - pump speed (milliliters per hour)
    - ```clr``` - remove old log (doesn't work now)
    - ```pwr``` - allows/forbids pump work (bool)
