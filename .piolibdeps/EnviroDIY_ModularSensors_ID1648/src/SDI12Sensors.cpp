/*
 *SDI12Sensors.cpp
 *This file is part of the EnviroDIY modular sensors library for Arduino
 *
 *Initial library developement done by Sara Damiano (sdamiano@stroudcenter.org).
 *
 *This file is for all Decagon Devices that communicate with SDI-12
 *It is dependent on the EnviroDIY SDI-12 library.
*/

#define LIBCALL_ENABLEINTERRUPT  // To prevent compiler/linker crashes
#include <EnableInterrupt.h>  // To handle external and pin change interrupts

#include "SDI12Sensors.h"


// The constructor - need the number of measurements the sensor will return, SDI-12 address, the power pin, and the data pin
SDI12Sensors::SDI12Sensors(char SDI12address, int8_t powerPin, int8_t dataPin, uint8_t measurementsToAverage,
                           const char *sensorName, uint8_t numReturnedVars,
                           uint32_t warmUpTime_ms, uint32_t stabilizationTime_ms, uint32_t measurementTime_ms)
    : Sensor(sensorName, numReturnedVars,
             warmUpTime_ms, stabilizationTime_ms, measurementTime_ms,
             powerPin, dataPin, measurementsToAverage),
    _SDI12Internal(dataPin)
{
    _SDI12address = SDI12address;
}
SDI12Sensors::SDI12Sensors(char *SDI12address, int8_t powerPin, int8_t dataPin, uint8_t measurementsToAverage,
                           const char *sensorName, uint8_t numReturnedVars,
                           uint32_t warmUpTime_ms, uint32_t stabilizationTime_ms, uint32_t measurementTime_ms)
    : Sensor(sensorName, numReturnedVars,
             warmUpTime_ms, stabilizationTime_ms, measurementTime_ms,
             powerPin, dataPin, measurementsToAverage),
    _SDI12Internal(dataPin)
{
    _SDI12address = *SDI12address;
}
SDI12Sensors::SDI12Sensors(int SDI12address, int8_t powerPin, int8_t dataPin, uint8_t measurementsToAverage,
                           const char *sensorName, uint8_t numReturnedVars,
                           uint32_t warmUpTime_ms, uint32_t stabilizationTime_ms, uint32_t measurementTime_ms)
    : Sensor(sensorName, numReturnedVars,
             warmUpTime_ms, stabilizationTime_ms, measurementTime_ms,
             powerPin, dataPin, measurementsToAverage),
    _SDI12Internal(dataPin)
{
    _SDI12address = SDI12address + '0';
}


bool SDI12Sensors::setup(void)
{
    bool retVal = Sensor::setup();  // this will set timestamp and status bit

    // Begin the SDI-12 interface
    _SDI12Internal.begin();

    // Library default timeout should be 150ms, which is 10 times that specified
    // by the SDI-12 protocol for a sensor response.
    // May want to bump it up even further here.
    _SDI12Internal.setTimeout(150);
    // Force the timeout value to be -9999 (This should be library default.)
    _SDI12Internal.setTimeoutValue(-9999);

    // Allow the SDI-12 library access to interrupts
    MS_DBG(F("Enabling interrupts for SDI12 on pin "), _dataPin, '\n');
    enableInterrupt(_dataPin, SDI12::handleInterrupt, CHANGE);

    retVal &= getSensorInfo();

    // Empty the SDI-12 buffer
    _SDI12Internal.clearBuffer();

    // De-activate the SDI-12 Object
    // Use end() instead of just forceHold to un-set the timers
    _SDI12Internal.end();

    return retVal;
}


bool SDI12Sensors::requestSensorAcknowledgement(void)
{
    // Empty the buffer
    _SDI12Internal.clearBuffer();

    MS_DBG(F("   Asking for sensor acknowlegement\n"));
    String myCommand = "";
    myCommand += (char) _SDI12address;
    myCommand += "!"; // sends 'acknowledge active' command [address][!]

    bool didAcknowledge = false;
    uint8_t ntries = 0;
    while (!didAcknowledge && ntries < 5)
    {

        _SDI12Internal.sendCommand(myCommand);
        MS_DBG(F("      >>> "), myCommand, F("\n"));
        delay(30);

        // wait for acknowlegement with format:
        // [address]<CR><LF>
        String sdiResponse = _SDI12Internal.readStringUntil('\n');
        sdiResponse.trim();
        MS_DBG(F("      <<< "), sdiResponse, F("\n"));

        // Empty the buffer again
        _SDI12Internal.clearBuffer();

        if (sdiResponse == String(_SDI12address))
        {
            MS_DBG(F("   "), getSensorName(), F(" at "), getSensorLocation(),
                   F(" replied as expected.\n"));
            didAcknowledge = true;
        }
        else if (sdiResponse.startsWith(String(_SDI12address)))
        {
            MS_DBG(F("   "), getSensorName(), F(" at "), getSensorLocation(),
                   F(" replied, unexpectedly\n"));
            didAcknowledge = true;
        }
        else
        {
            MS_DBG(F("   "), getSensorName(), F(" at "), getSensorLocation(),
                   F(" did not reply!\n"));
            didAcknowledge = false;
        }

        ntries++;
    }

    return didAcknowledge;
}


// A helper function to run the "sensor info" SDI12 command
bool SDI12Sensors::getSensorInfo(void)
{
    MS_DBG(F("   Activating SDI-12 instance for "), getSensorName(),
           F(" at "), getSensorLocation(), '\n');
    // Check if this the currently active SDI-12 Object
    bool wasActive = _SDI12Internal.isActive();
    // If it wasn't active, activate it now.
    // Use begin() instead of just setActive() to ensure timer is set correctly.
    if (wasActive) MS_DBG(F("   SDI-12 instance for "), getSensorName(),
           F(" at "), getSensorLocation(), " was already active!\n");
    if (!wasActive) _SDI12Internal.begin();
    // Empty the buffer
    _SDI12Internal.clearBuffer();

    // Check that the sensor is there and responding
    if (!requestSensorAcknowledgement())
    {
        // if(!wasOn){powerDown();}
        return false;
    }

    MS_DBG(F("   Getting sensor info\n"));
    String myCommand = "";
    myCommand += (char) _SDI12address;
    myCommand += "I!"; // sends 'info' command [address][I][!]
    _SDI12Internal.sendCommand(myCommand);
    MS_DBG(F("      >>> "), myCommand, F("\n"));
    delay(30);

    // wait for acknowlegement with format:
    // [address][SDI12 version supported (2 char)][vendor (8 char)][model (6 char)][version (3 char)][serial number (<14 char)]<CR><LF>
    String sdiResponse = _SDI12Internal.readStringUntil('\n');
    sdiResponse.trim();
    MS_DBG(F("      <<< "), sdiResponse, F("\n"));

    // Empty the buffer again
    _SDI12Internal.clearBuffer();

    // De-activate the SDI-12 Object
    // Use end() instead of just forceHold to un-set the timers
    if (!wasActive) _SDI12Internal.end();

    if (sdiResponse.length() > 1)
    {
        String sdi12Address = sdiResponse.substring(0,1);
        MS_DBG(F("   SDI12 Address:"), sdi12Address);
        float sdi12Version = sdiResponse.substring(1,3).toFloat();
        sdi12Version /= 10;
        MS_DBG(F(", SDI12 Version:"), sdi12Version);
        _sensorVendor = sdiResponse.substring(3,11);
        _sensorVendor.trim();
        MS_DBG(F(", Sensor Vendor:"), _sensorVendor);
        _sensorModel = sdiResponse.substring(11,17);
        _sensorModel.trim();
        MS_DBG(F(", Sensor Model:"), _sensorModel);
        _sensorVersion = sdiResponse.substring(17,20);
        _sensorVersion.trim();
        MS_DBG(F(", Sensor Version:"), _sensorVersion);
        _sensorSerialNumber = sdiResponse.substring(20);
        _sensorSerialNumber.trim();
        MS_DBG(F(", Sensor Serial Number:"), _sensorSerialNumber,'\n');
        return true;
    }
    else return false;
}


// The sensor vendor
String SDI12Sensors::getSensorVendor(void)
{return _sensorVendor;}

// The sensor model
String SDI12Sensors::getSensorModel(void)
{return _sensorModel;}

// The sensor version
String SDI12Sensors::getSensorVersion(void)
{return _sensorVersion;}

// The sensor serial number
String SDI12Sensors::getSensorSerialNumber(void)
{return _sensorSerialNumber;}


// The sensor installation location on the Mayfly
String SDI12Sensors::getSensorLocation(void)
{
    String sensorLocation = F("SDI12-");
    sensorLocation += String(_SDI12address) + F("_Pin") + String(_dataPin);
    return sensorLocation;
}


// Sending the command to get a concurrent measurement
bool SDI12Sensors::startSingleMeasurement(void)
{
    bool retVal = false;
    String startCommand;
    String sdiResponse;


    MS_DBG(F("   Activating SDI-12 instance for "), getSensorName(),
           F(" at "), getSensorLocation(), '\n');
    // Check if this the currently active SDI-12 Object
    bool wasActive = _SDI12Internal.isActive();
    if (wasActive) MS_DBG(F("   SDI-12 instance for "), getSensorName(),
           F(" at "), getSensorLocation(), " was already active!\n");
    // If it wasn't active, activate it now.
    // Use begin() instead of just setActive() to ensure timer is set correctly.
    if (!wasActive) _SDI12Internal.begin();
    // Empty the buffer
    _SDI12Internal.clearBuffer();

    // Check that the sensor is there and responding
    if (!requestSensorAcknowledgement())
    {
        _millisMeasurementRequested = 0;
        retVal = false;
        goto finish;
    }

    MS_DBG(F("   Beginning concurrent measurement on "), getSensorName(),
           F(" at "), getSensorLocation(), '\n');
    startCommand = "";
    startCommand += _SDI12address;
    startCommand += "C!"; // Start concurrent measurement - format  [address]['C'][!]
    _SDI12Internal.sendCommand(startCommand);
    delay(30);  // It just needs this little delay
    MS_DBG(F("      >>> "), startCommand, F("\n"));

    // wait for acknowlegement with format
    // [address][ttt (3 char, seconds)][number of values to be returned, 0-9]<CR><LF>
    sdiResponse = _SDI12Internal.readStringUntil('\n');
    sdiResponse.trim();
    _SDI12Internal.clearBuffer();
    MS_DBG(F("      <<< "), sdiResponse, F("\n"));

    // Empty the buffer again
    _SDI12Internal.clearBuffer();

    // De-activate the SDI-12 Object
    // Use end() instead of just forceHold to un-set the timers
    if (!wasActive) _SDI12Internal.end();

    // Verify the number of results the sensor will send
    // int numVariables = sdiResponse.substring(4,5).toInt();
    // if (numVariables != _numReturnedVars)
    // {
    //     MS_DBG(numVariables, F(" results expected\n"));
    //     MS_DBG(F("This differs from the sensor's standard design of "));
    //     MS_DBG(_numReturnedVars, F(" measurements!!\n"));
    // }

    // Set the times we've activated the sensor and asked for a measurement
    if (sdiResponse.length() > 0)
    {
        MS_DBG(F("   Concurrent measurement started.\n"));
        // Mark the time that a measurement was requested
        _millisMeasurementRequested = millis();
        retVal = true;
        goto finish;
    }
    else
    {
        MS_DBG(F("   "), getSensorName(), F(" at "), getSensorLocation(),
               F(" did not respond to measurement request!\n"));
        _millisMeasurementRequested = 0;
        retVal = false;
        goto finish;
    }

    finish:

    // Even if we failed to start a measurement, we still want to set the status
    // bit to show that we attempted to start the measurement.
    // Set the status bits for measurement requested (bit 5)
    _sensorStatus |= 0b00100000;
    // Verify that the status bit for a single measurement completion is not set (bit 6)
    _sensorStatus &= 0b10111111;

    return retVal;
}


bool SDI12Sensors::addSingleMeasurementResult(void)
{
    if (_millisMeasurementRequested > 0)
    {
        MS_DBG(F("   Activating SDI-12 instance for "), getSensorName(),
               F(" at "), getSensorLocation(), '\n');
        // Check if this the currently active SDI-12 Object
        bool wasActive = _SDI12Internal.isActive();
        if (wasActive) MS_DBG(F("   SDI-12 instance for "), getSensorName(),
               F(" at "), getSensorLocation(), " was already active!\n");
        // If it wasn't active, activate it now.
        // Use begin() instead of just setActive() to ensure timer is set correctly.
        if (!wasActive) _SDI12Internal.begin();
        // Empty the buffer
        _SDI12Internal.clearBuffer();

        MS_DBG(F("   Requesting data from "), getSensorName(),
               F(" at "), getSensorLocation(), '\n');
        String getDataCommand = "";
        getDataCommand += _SDI12address;
        getDataCommand += "D0!";  // SDI-12 command to get data [address][D][dataOption][!]
        _SDI12Internal.sendCommand(getDataCommand);
        delay(30);  // It just needs this little delay
        MS_DBG(F("      >>> "), getDataCommand, F("\n"));

        uint32_t startTime = millis();
        while (_SDI12Internal.available() < 3 && (millis() - startTime) < 1500) {}
        MS_DBG(F("   Receiving results from "), getSensorName(),
               F(" at "), getSensorLocation(), '\n');
        _SDI12Internal.read();  // ignore the repeated SDI12 address
        for (int i = 0; i < _numReturnedVars; i++)
        {
            float result = _SDI12Internal.parseFloat();
            // The SDI-12 library should return -9999 on timeout
            if (result == -9999 or isnan(result)) result = -9999;
            MS_DBG(F("      <<< Result #"), i, F(": "), result, F("\n"));
            verifyAndAddMeasurementResult(i, result);

        }
        // String sdiResponse = _SDI12Internal.readStringUntil('\n');
        // sdiResponse.trim();
        // _SDI12Internal.clearBuffer();
        // MS_DBG(F("      <<< "), sdiResponse, F("\n"));

        // Empty the buffer again
        _SDI12Internal.clearBuffer();

        // De-activate the SDI-12 Object
        // Use end() instead of just forceHold to un-set the timers
        if (!wasActive) _SDI12Internal.end();

        // Unset the time stamp for the beginning of this measurement
        _millisMeasurementRequested = 0;
        // Unset the status bit for a measurement having been requested (bit 5)
        _sensorStatus &= 0b11011111;
        // Set the status bit for measurement completion (bit 6)
        _sensorStatus |= 0b01000000;

        // Return true when finished
        return true;
    }
    else
    {
        MS_DBG(F("   "), getSensorName(), F(" at "), getSensorLocation(),
               F(" is not currently measuring!\n"));
        return false;
    }
}
