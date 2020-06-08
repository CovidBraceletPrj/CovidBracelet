#
# Copyright (c) 2020 Olaf Landsiedel, based on a code example from Tony DiCola
#
# SPDX-License-Identifier: Apache-2.0
#

#NOTE: seems not to work well on some iMACs

import logging
import time
import uuid

import struct
import binascii

import Adafruit_BluefruitLE

# Enable debug output.
#logging.basicConfig(level=logging.DEBUG)

# Define service and characteristic UUIDs used by the Covid service.
COVID_SERVICE_UUID = uuid.UUID('F2110D79-699F-6A98-EA42-A7AD9EC75106')
NEXT_KEY_UUID      = uuid.UUID('F3110D79-699F-6A98-EA42-A7AD9EC75106')
NEW_KEY_UUID      = uuid.UUID('F4110D79-699F-6A98-EA42-A7AD9EC75106')

INFECTED_KEY_CNT_UUID      = uuid.UUID('F5110D79-699F-6A98-EA42-A7AD9EC75106')

PERIOD_KEY_0_UUID      = uuid.UUID('00110D79-699F-6A98-EA42-A7AD9EC75106')
PERIOD_KEY_1_UUID      = uuid.UUID('01110D79-699F-6A98-EA42-A7AD9EC75106')
PERIOD_KEY_2_UUID      = uuid.UUID('02110D79-699F-6A98-EA42-A7AD9EC75106')
PERIOD_KEY_3_UUID      = uuid.UUID('03110D79-699F-6A98-EA42-A7AD9EC75106')
PERIOD_KEY_4_UUID      = uuid.UUID('04110D79-699F-6A98-EA42-A7AD9EC75106')
PERIOD_KEY_5_UUID      = uuid.UUID('05110D79-699F-6A98-EA42-A7AD9EC75106')
PERIOD_KEY_6_UUID      = uuid.UUID('06110D79-699F-6A98-EA42-A7AD9EC75106')
PERIOD_KEY_7_UUID      = uuid.UUID('07110D79-699F-6A98-EA42-A7AD9EC75106')
PERIOD_KEY_8_UUID      = uuid.UUID('08110D79-699F-6A98-EA42-A7AD9EC75106')
PERIOD_KEY_9_UUID      = uuid.UUID('09110D79-699F-6A98-EA42-A7AD9EC75106')
PERIOD_KEY_10_UUID      = uuid.UUID('0A110D79-699F-6A98-EA42-A7AD9EC75106')
PERIOD_KEY_11_UUID      = uuid.UUID('0B110D79-699F-6A98-EA42-A7AD9EC75106')
PERIOD_KEY_12_UUID      = uuid.UUID('0C110D79-699F-6A98-EA42-A7AD9EC75106')
PERIOD_KEY_13_UUID      = uuid.UUID('0D110D79-699F-6A98-EA42-A7AD9EC75106')

infected_period_keys = []

# Get the BLE provider for the current platform.
ble = Adafruit_BluefruitLE.get_provider()

def check_infection(covid):
    infected_key_cnt_reader = covid.find_characteristic(INFECTED_KEY_CNT_UUID)

    period_key_readers = (
        covid.find_characteristic(PERIOD_KEY_0_UUID), 
        covid.find_characteristic(PERIOD_KEY_1_UUID),
        covid.find_characteristic(PERIOD_KEY_2_UUID),
        covid.find_characteristic(PERIOD_KEY_3_UUID),
        covid.find_characteristic(PERIOD_KEY_4_UUID),
        covid.find_characteristic(PERIOD_KEY_5_UUID),
        covid.find_characteristic(PERIOD_KEY_6_UUID),
        covid.find_characteristic(PERIOD_KEY_7_UUID),
        covid.find_characteristic(PERIOD_KEY_8_UUID),
        covid.find_characteristic(PERIOD_KEY_9_UUID),
        covid.find_characteristic(PERIOD_KEY_10_UUID),
        covid.find_characteristic(PERIOD_KEY_11_UUID),
        covid.find_characteristic(PERIOD_KEY_12_UUID),
        covid.find_characteristic(PERIOD_KEY_13_UUID)
    )

    #check for infection
    ret = bytearray(infected_key_cnt_reader.read_value())
    infected_key_cnt = int.from_bytes(ret, byteorder='little', signed=False)
    print('device has: infected key cnt ', infected_key_cnt)
    for i in range(infected_key_cnt):
        ret = bytearray(period_key_readers[i].read_value())
        if ret not in infected_period_keys:
            print('adding to DB new infected key ', i,  ': ', binascii.hexlify(bytearray(ret)))
            infected_period_keys.append(ret)
        else:
            print('infected key ', i , 'already known in DB: ', binascii.hexlify(bytearray(ret)))


def upload_keys(covid):
    next_key = covid.find_characteristic(NEXT_KEY_UUID)
    new_key = covid.find_characteristic(NEW_KEY_UUID)
    
    go_on = True

    while go_on:    
        print("read key")
        ret = bytearray(next_key.read_value())
        expected_index = int.from_bytes(ret, byteorder='little', signed=False)
        print('expected key index of device is ', expected_index, ', we have upto id ', len(infected_period_keys))
        #TODO: check for maximum storage on device
        if( expected_index >= 0 and expected_index < len(infected_period_keys) ):
            print('Sending key ', expected_index, ' from DB to device...')
            new_key.write_value(infected_period_keys[expected_index])
            #TODO do we need this?
            time.sleep(1)
        else:
            go_on = False


# Main function implements the program logic so it can run in a background
# thread.  Most platforms require the main thread to handle GUI events and other
# asyncronous events like BLE actions.  All of the threading logic is taken care
# of automatically though and you just need to provide a main function that uses
# the BLE provider.
def main():
    # Clear any cached data because both bluez and CoreBluetooth have issues with
    # caching data and it going stale.
    ble.clear_cached_data()

    # Get the first available BLE network adapter and make sure it's powered on.
    adapter = ble.get_default_adapter()
    adapter.power_on()
    print('Using adapter: {0}'.format(adapter.name))

    # Disconnect any currently connected UART devices.  Good for cleaning up and
    # starting from a fresh state.
    print('Disconnecting any connected Covid devices...')
    ble.disconnect_devices([COVID_SERVICE_UUID])

    while True:
        # Scan for Covid devices.
        print('Searching for Covid device...')
        try:
            adapter.start_scan()
            # Search for the first Covid device found (will time out after 60 seconds
            # but you can specify an optional timeout_sec parameter to change it).
            device = ble.find_device(service_uuids=[COVID_SERVICE_UUID])
            if device is None:
                raise RuntimeError('Failed to find Covid device!')
        finally:
            # Make sure scanning is stopped before exiting.
            adapter.stop_scan()

        if device is not None:
            print('Connecting to device...')
            device.connect()  # Will time out after 60 seconds, specify timeout_sec parameter
                        # to change the timeout.

            # Once connected do everything else in a try/finally to make sure the device
            # is disconnected when done.
            try:
                # Wait for service discovery to complete for at least the specified
                # service and characteristic UUID lists.  Will time out after 60 seconds
                # (specify timeout_sec parameter to override).
                print('Discovering services...')
                device.discover([COVID_SERVICE_UUID], [NEXT_KEY_UUID, NEW_KEY_UUID, 
                    INFECTED_KEY_CNT_UUID,
                    PERIOD_KEY_0_UUID,
                    PERIOD_KEY_1_UUID,
                    PERIOD_KEY_2_UUID,
                    PERIOD_KEY_3_UUID,
                    PERIOD_KEY_4_UUID,
                    PERIOD_KEY_5_UUID,
                    PERIOD_KEY_6_UUID,
                    PERIOD_KEY_7_UUID,
                    PERIOD_KEY_8_UUID,
                    PERIOD_KEY_9_UUID,
                    PERIOD_KEY_10_UUID,
                    PERIOD_KEY_11_UUID,
                    PERIOD_KEY_12_UUID,
                    PERIOD_KEY_13_UUID
                    ])

                # Find the Covid service and its characteristics.
                covid = device.find_service(COVID_SERVICE_UUID)

                check_infection(covid)
                upload_keys(covid)
            finally:
                # Make sure device is disconnected on exit.
                device.disconnect()

# Initialize the BLE system.  MUST be called before other BLE calls!
ble.initialize()

# Start the mainloop to process BLE events, and run the provided function in
# a background thread.  When the provided main function stops running, returns
# an integer status code, or throws an error the program will exit.
ble.run_mainloop_with(main)