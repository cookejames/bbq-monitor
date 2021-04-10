#include <NimBLEDevice.h>
#include <Arduino.h>
#include <ArduinoLog.h>

NimBLEUUID PRIMARY_UUID(uint16_t(0xFFF0));
NimBLEUUID ACCOUNT_AND_VERIFY_CHARACTERISTIC(uint16_t(0xFFF2));
NimBLEUUID SETTINGS_RESULT_CHARACTERISTIC(uint16_t(0xFFF1));
NimBLEUUID SETTINGS_WRITE_CHARACTERISTIC(uint16_t(0xFFF5));
NimBLEUUID REAL_TIME_DATA_CHARACTERISTIC(uint16_t(0xFFF4));
NimBLEUUID HISTORIC_DATA_CHARACTERISTIC(uint16_t(0xFFF3));

uint8_t CREDENTIALS_MSG[] = {0x21, 0x07, 0x06,
                             0x05, 0x04, 0x03, 0x02, 0x01, 0xb8, 0x22,
                             0x00, 0x00, 0x00, 0x00, 0x00};                //14
uint8_t REALTIME_DATA_ENABLE_MSG[] = {0x0B, 0x01, 0x00, 0x00, 0x00, 0x00}; //6
// _UNITS_FAHRENHEIT_MSG = b"\x02\x01\x00\x00\x00\x00"
// _UNITS_CELSIUS_MSG = b"\x02\x00\x00\x00\x00\x00"
// _REQUEST_BATTERY_LEVEL_MSG = b"\x08\x24\x00\x00\x00\x00"

void scanEndedCB(NimBLEScanResults results);

static NimBLEAdvertisedDevice *advDevice;

static bool doConnect = false;
static uint32_t scanTime = 10; /** 0 = scan forever */

class ClientCallbacks : public NimBLEClientCallbacks
{
  void onConnect(NimBLEClient *pClient)
  {
    Log.trace("Client connected");
  };

  void onDisconnect(NimBLEClient *pClient)
  {
    Log.notice("%s Disconnected - Starting scan", pClient->getPeerAddress().toString().c_str());
    NimBLEDevice::getScan()->start(scanTime, scanEndedCB);
  };
};
/** Create a single global instance of the callback class to be used by all clients */
static ClientCallbacks clientCB;

/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks
{

  void onResult(NimBLEAdvertisedDevice *advertisedDevice)
  {
    Log.verbose("Advertised Device found: %s", advertisedDevice->toString().c_str());
    if (String("iBBQ").equals(advertisedDevice->getName().c_str()) && advertisedDevice->isAdvertisingService(PRIMARY_UUID))
    {
      Log.notice("Found iBBQ Device");
      /** stop scan before connecting */
      NimBLEDevice::getScan()->stop();
      /** Save the device reference in a global for the client to use*/
      advDevice = advertisedDevice;
      /** Ready to connect now */
      doConnect = true;
    }
  };
};

/** Notification / Indication receiving handler callback */
void notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  std::string str = (isNotify == true) ? "Notification" : "Indication";
  str += " from ";
  /** NimBLEAddress and NimBLEUUID have std::string operators */
  str += std::string(pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress());
  str += ": Service = " + std::string(pRemoteCharacteristic->getRemoteService()->getUUID());
  str += ", Characteristic = " + std::string(pRemoteCharacteristic->getUUID());
  str += ", Length = " + (uint8_t)length;
  str += ", Value = " + std::string((char *)pData, length);
  Log.verbose(str.c_str());

  if (length > 0 && pRemoteCharacteristic->getUUID().equals(REAL_TIME_DATA_CHARACTERISTIC))
  {
    uint8_t numProbes = length / 2;
    uint16_t result[numProbes];
    for (uint8_t i = 0; i < length; i++)
    {
      if (i % 2 == 0 && i < length - 1)
      {
        uint16_t value = (((uint16_t)pData[i + 1] << 8) + pData[i]) / 10;
        result[i / 2] = value;
      }
    }
    //TODO replace with return
    Log.notice("Reading: %d:%d:%d:%d", result[0], result[1], result[2], result[3]);
  }
}

/** Callback to process the results of the last scan or restart it */
void scanEndedCB(NimBLEScanResults results)
{
  Log.notice("Scan Ended");
}

/** Handles the provisioning of clients and connects / interfaces with the server */
bool connectToServer()
{
  NimBLEClient *pClient = nullptr;

  /** Check if we have a client we should reuse first **/
  if (NimBLEDevice::getClientListSize())
  {
    /** Special case when we already know this device, we send false as the
         *  second argument in connect() to prevent refreshing the service database.
         *  This saves considerable time and power.
         */
    pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
    if (pClient)
    {
      if (!pClient->connect(advDevice, false))
      {
        Log.notice("Reconnect failed");
        return false;
      }
      Log.notice("Reconnected client");
    }
    /** We don't already have a client that knows this device,
         *  we will check for a client that is disconnected that we can use.
         */
    else
    {
      pClient = NimBLEDevice::getDisconnectedClient();
    }
  }

  /** No client to reuse? Create a new one. */
  if (!pClient)
  {
    if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS)
    {
      Log.error("Max clients reached - no more connections available");
      return false;
    }

    pClient = NimBLEDevice::createClient();

    Log.verbose("New client created");

    pClient->setClientCallbacks(&clientCB, false);

    if (!pClient->connect(advDevice))
    {
      /** Created a client but failed to connect, don't need to keep it as it has no data */
      NimBLEDevice::deleteClient(pClient);
      Log.notice("Failed to connect, deleted client");
      return false;
    }
  }

  if (!pClient->isConnected())
  {
    if (!pClient->connect(advDevice))
    {
      Log.warning("Failed to connect");
      return false;
    }
  }

  Log.trace("Connected to: %s", pClient->getPeerAddress().toString().c_str());
  Log.trace("RSSI: %d", pClient->getRssi());

  /** Now we can read/write/subscribe the charateristics of the services we are interested in */
  NimBLERemoteService *pSvc = nullptr;
  NimBLERemoteCharacteristic *pChr = nullptr;

  pSvc = pClient->getService(PRIMARY_UUID);
  if (pSvc)
  { /** make sure it's not null */
    pChr = pSvc->getCharacteristic(ACCOUNT_AND_VERIFY_CHARACTERISTIC);

    if (pChr)
    { /** make sure it's not null */
      if (pChr->canWrite())
      {
        if (pChr->writeValue(CREDENTIALS_MSG, 14))
        {
          Log.verbose("Wrote new value to: %s", pChr->getUUID().toString().c_str());
        }
        else
        {
          /** Disconnect if write failed */
          pClient->disconnect();
          return false;
        }
      }
    }
  }
  else
  {
    Log.warning("Primary service not found.");
  }

  pSvc = pClient->getService(PRIMARY_UUID);
  if (pSvc)
  { /** make sure it's not null */
    pChr = pSvc->getCharacteristic(SETTINGS_RESULT_CHARACTERISTIC);

    if (pChr)
    { /** make sure it's not null */
      if (pChr->canNotify())
      {
        if (!pChr->subscribe(true, notifyCB))
        {
          /** Disconnect if subscribe failed */
          pClient->disconnect();
          return false;
        }
      }
    }
  }
  else
  {
    Log.warning("Primary service not found.");
  }

  pSvc = pClient->getService(PRIMARY_UUID);
  if (pSvc)
  { /** make sure it's not null */
    pChr = pSvc->getCharacteristic(REAL_TIME_DATA_CHARACTERISTIC);

    if (pChr)
    { /** make sure it's not null */
      if (pChr->canNotify())
      {
        if (!pChr->subscribe(true, notifyCB))
        {
          /** Disconnect if subscribe failed */
          pClient->disconnect();
          return false;
        }
      }
    }
  }
  else
  {
    Log.warning("Primary service not found.");
  }

  pSvc = pClient->getService(PRIMARY_UUID);
  if (pSvc)
  { /** make sure it's not null */
    pChr = pSvc->getCharacteristic(SETTINGS_WRITE_CHARACTERISTIC);

    if (pChr)
    { /** make sure it's not null */
      if (pChr->canWrite())
      {
        if (pChr->writeValue(REALTIME_DATA_ENABLE_MSG, 6))
        {
          Log.verbose("Wrote new value to: %s", pChr->getUUID().toString().c_str());
        }
        else
        {
          /** Disconnect if write failed */
          pClient->disconnect();
          return false;
        }
      }
    }
  }
  else
  {
    Log.warning("Primary service not found.");
  }
  Log.trace("Done connecting to the device!");
  return true;
}

void printNewline(Print *_logOutput)
{
  _logOutput->print('\n');
}

void setup()
{
  Log.begin(LOG_LEVEL_TRACE, &Serial, true);
  Log.setSuffix(printNewline);

  Log.verbose("Starting NimBLE Client");
  /** Initialize NimBLE, no device name spcified as we are not advertising */
  NimBLEDevice::init("");

  /** Optional: set the transmit power, default is 3db */
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */

  /** create new scan */
  NimBLEScan *pScan = NimBLEDevice::getScan();

  /** create a callback that gets called when advertisers are found */
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());

  /** Set scan interval (how often) and window (how long) in milliseconds */
  pScan->setInterval(45);
  pScan->setWindow(15);

  /** Active scan will gather scan response data from advertisers
     *  but will use more energy from both devices
     */
  pScan->setActiveScan(true);
  /** Start scanning for advertisers for the scan time specified (in seconds) 0 = forever
     *  Optional callback for when scanning stops.
     */
  pScan->start(scanTime, scanEndedCB);
}

void loop()
{
  /** Loop here until we find a device we want to connect to */
  while (!doConnect)
  {
    delay(1);
  }

  doConnect = false;

  /** Found a device we want to connect to, do it now */
  if (connectToServer())
  {
    Log.verbose("Success! we should now be getting notifications");
  }
  else
  {
    Log.warning("Failed to connect!");
  }

  // NimBLEDevice::getScan()->start(scanTime, scanEndedCB);
}