#MONITORING MODULE

---

###UART COMMANDS

- settings - get current settings
- time - get module time
- setid <uint id> - set new module id
- setsleep <uint time> - set log frequency (in seconds)
- seturl <string url> - set server url
- setport <string port> - set server port
- setlitersmin <uint liters> - set tank min level (in milliliters)
- setlitersmax <uint liters> - set tank max level (in milliliters)
- saveadcmin - save current liquid sensor value as min value
- saveadcmax - save current liquid sensor value as max value
- settsrget <uint liters> - set target liguid capacity per day (in milliliters)
- setpumpspeed <uint liters> - pump speed (in milliliters per hour)
- setlogid <uint id> - set last log id in server DB
- default - set default settings
- clearlog - remove old log

###POST RESPONSE PARAMS

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
