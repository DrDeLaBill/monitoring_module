# MONITORING MODULE

---

### UART COMMANDS

- status - get current settings
- time - get module time
- setid (uint id) - set new module id
- setsleep (uint time) - set log frequency (in seconds)
- seturl (string url) - set server url
- setport (string port) - set server port
- setlitersmin (uint liters) - set tank min level (in milliliters)
- setlitersmax (uint liters) - set tank max level (in milliliters)
- saveadcmin - save current liquid sensor value as min value
- saveadcmax - save current liquid sensor value as max value
- settarget (uint liters) - set target liguid capacity per day (in milliliters)
- setpumpspeed (uint liters) - pump speed (in milliliters per hour)
- setlogid (uint id) - set last log id in server DB
- default - set default settings
- clearlog - remove old log

### POST RESPONSE PARAMS

Module send POST request to server like:
```
POST /api/v1/send HTTP/1.1
Host: (IP/URL):(port)
User-Agent: module
Connection: close
Accept-Charset: utf-8, us-ascii
Content-Type: text/plain
Content-Length: 100

id=123
fw_id=1
cf_id=1
t=2000-01-01T00:00:12
d=id=5;level=0.0;press_1=0.61;press_2=0.0;pump=9750060
```

Needed response from server:

```
HTTP/1.1 200 OK
server: nginx/1.18.0
date: t
referrer-policy: same-origin
cross-origin-opener-policy: same-origin

t=2023-01-19t12:56:09.386436
d_hwm=473830
cf_id=33077616
cf=id=123;ltrmin=100;ltrmax=1;trgt=4000;sleep=900;speed=1000;logid=1;clr=0
```

- t - current time
- cf_id - current configuration, updates if module settings changes
- cf - new config array
    - id - new module id
    - ltrmin - tank min level (in milliliters)
    - ltrmax - tank max level (in milliliters)
    - trgt - target liguid capacity per day (in milliliters)
    - sleep - log frequency (in seconds)
    - speed - pump speed (in milliliters per hour)
    - logid - last log id in server DB
    - clr - remove old log
