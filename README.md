# Covid Bracelet, compatibile with Covid Apps on iOS and Android [![Actions Status: test](https://github.com/CovidBraceletPrj/CovidBracelet/workflows/test/badge.svg)](https://github.com/CovidBraceletPrj/CovidBracelet/actions) [![Actions Status: build](https://github.com/CovidBraceletPrj/CovidBracelet/workflows/build/badge.svg)](https://github.com/CovidBraceletPrj/CovidBracelet/actions)


**Contributions Welcome!** 



## Features
*  Sends and receives exposure beacons as specified by Google and Apple for Covid Contact Tracing
  * Rolling, encrypted, anonymous beacons 
  * Compatible with Apple iOS and Android phones
  * With rolling proximity identifiers and encrypted metadata
  * Proximity identifiers and BLE addresses change every 10 minutes
* Stores own keys for 14 days
* Stores contacts of more than 5 minute duration
* Upon infection upload keys to a public database
* Retrieve keys of infections from database
  * computes rolling proximity identifiers 
  * compares to stored contacts to check for exposure 

Builds on on Zephyr OS and NRF52 BLE SOCs. Note: as we for now do not use the flash for key storage, this currently only works on nrf52480 or you can just store a very small number of keys. Moving the keys to flash is on the TODO list and will fix this. 

## Get Started:
This project is based on platformIO, see:  [https://platformio.org/platformio-ide](https://platformio.org/platformio-ide)

## Testing
To run the tests for the desktop environment select the test task for desktop or run:
```
platformio test -e desktop
```

**Note: this is a proof of concept and not ready for production**

## Demo Video

[![Video Demo](https://img.youtube.com/vi/tYGsFJC3LtE/0.jpg)](https://youtu.be/tYGsFJC3LtE)

## Open / Possible next steps
* firmware of the air updates (signed): done in testing
* fix entropy: keys are always the same on boot up
* time sync
* set device name, user id or so in Flash
* set scanning interval to the correct value, for now we just use the default
* set advertisement interval, correct value, for now we just use the default: should be 200-270 milliseconds
* store long-term contacts in flash
* set transmit power and store this in the corona beacons
* get infections from DB, check their signatures
* extensive logging: crash, reboot, battery level, charging state, contacts, memory useage, flash usage, 
* extensive compatibility testing with Apple iOS and Android devices
* contininous integration testing
* Energy efficiency
* BLE advertisements sets
* Secure GATT services and authentication of base statation in general
* More platforms: with display etc.

## TODOs Wristband
Possible platforms for real-world deployment many, as many of the cheap fitness trackers base on NRF52 or chips with similar capabilities.
However, many would need the firmware to be shipped to manufactures.
* Watch UI
* Pine Time could be good for testing

## TODOs App and Basestation
* extend this beyond the simple basestation
* read keys form national databases
