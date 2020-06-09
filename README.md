# Covid Bracelet

**Contributions Welcome!** Based on Zephyr OS and NRF52 BLE SOCs

## Get Started: 
We use Zepyhr master as we need a newer mbed TLS as the ones that ships with Zephyr 2.2. We are waiting for the Zepyhr 2.3 release. To build, please install Zephyr and compile via west. Note that Platform.io does not support Zephyr 2.3 rc / Zepyhr master at the current time.

## Features
* Code sends out and receives exposure beacons as specified by Google and Apple for Covid Contact Tracing
  * Rolling, encrypted, anonymous beacons 
  * Compatible with Apple iOS and Android phones
  * with rolling proximity identifiers and encrypted metadata
  * proximity identifiers and BLE addresses change every 10 minutes
* stores own keys for 14 days
* stores contacts with more than 5 minute duration
* Upon infection upload keys to a public database
* Retrieve keys of infections from database
  * computes rolling proximity identifiers 
  * compares to stored contacts to check for exposure 
* Note: as we do not use the flash for key storage, this only works on nrf52480 or you can just store a very small number of keys

**Note: this is a proof of concept and not ready for production**

## Open / Possible next steps
* extensive compatibility testing with Apple iOS and Android devices
* contininous integration testing
* set device name in Flash
* fix entropy: keys are always the same on boot up
* set scanning interval to the correct value, for now we just use the default
* set advertisement interval, correct value, for now we just use the default: should be 200-270 milliseconds
* store long-term contacts in flash
* Energy efficiency
* BLE advertisements sets
* Secure GATT services
* More platforms / OS?
* time sync
* firmware of the air updates (signed)

## TODOs Wristband
Possible platforms for real-world deployment many, as many of the cheap fitness trackers base on NRF52 or chips with similar capabilities.
However, many would need the firmware to be shipped to manufactures.
* Watch UI
* Pine Time could be good for testing

## TODOs App and Basestation
* extend this beyond the simple basestation
* read keys form national databases
