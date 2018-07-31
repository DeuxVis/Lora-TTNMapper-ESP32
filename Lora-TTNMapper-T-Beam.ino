#include <HardwareSerial.h>
#include <TinyGPS++.h>
#include <lmic.h>
#include <hal/hal.h>
#include <WiFi.h>

// UPDATE the config.h file in the same folder WITH YOUR TTN KEYS AND ADDR.
#include "config.h"

// T-Beam specific hardware
#define BUILTIN_LED 21
#define GPS_TX 12
#define GPS_RX 15

// The TinyGPS++ object
TinyGPSPlus gps;
HardwareSerial GPSSerial(1);

char s[32]; // used to sprintf for Serial output

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;
// Schedule TX every this many seconds (might become longer due to duty cycle limitations).
const unsigned TX_INTERVAL = 30;

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 18,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = LMIC_UNUSED_PIN, // was "14,"
  .dio = {26, 33, 32},
};

unsigned long last_update = 0;
String toLog;
uint8_t txBuffer[9];
uint32_t LatitudeBinary, LongitudeBinary;
uint16_t altitudeGps;
uint8_t hdopGps;

#define PMTK_SET_NMEA_UPDATE_05HZ  "$PMTK220,2000*1C"
#define PMTK_SET_NMEA_UPDATE_1HZ  "$PMTK220,1000*1F"
#define PMTK_SET_NMEA_OUTPUT_RMCGGA "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"

void build_packet()
{
  LatitudeBinary = ((gps.location.lat() + 90) / 180.0) * 16777215;
  LongitudeBinary = ((gps.location.lng() + 180) / 360.0) * 16777215;
  
  sprintf(s, "Lat: %f", gps.location.lat());
  Serial.println(s);
  
  sprintf(s, "Lng: %f", gps.location.lng());
  Serial.println(s);
  
  txBuffer[0] = ( LatitudeBinary >> 16 ) & 0xFF;
  txBuffer[1] = ( LatitudeBinary >> 8 ) & 0xFF;
  txBuffer[2] = LatitudeBinary & 0xFF;

  txBuffer[3] = ( LongitudeBinary >> 16 ) & 0xFF;
  txBuffer[4] = ( LongitudeBinary >> 8 ) & 0xFF;
  txBuffer[5] = LongitudeBinary & 0xFF;

  altitudeGps = gps.altitude.meters();
  txBuffer[6] = ( altitudeGps >> 8 ) & 0xFF;
  txBuffer[7] = altitudeGps & 0xFF;

  hdopGps = gps.hdop.value()/10;
  txBuffer[8] = hdopGps & 0xFF;

  toLog = "";
  for(size_t i = 0; i<sizeof(txBuffer); i++)
  {
    char buffer[3];
    sprintf(buffer, "%02x", txBuffer[i]);
    toLog = toLog + String(buffer);
  }
  Serial.println(toLog);
}

void onEvent (ev_t ev) {
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      // Disable link check validation (automatically enabled
      // during join, but not supported by TTN at this time).
      LMIC_setLinkCheckMode(0);
      break;
    case EV_RFU1:
      Serial.println(F("EV_RFU1"));
      break;
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      digitalWrite(BUILTIN_LED, LOW);
      if (LMIC.txrxFlags & TXRX_ACK) {
        Serial.println(F("Received Ack"));
      }
      if (LMIC.dataLen) {
        sprintf(s, "Received %i bytes of payload", LMIC.dataLen);
        Serial.println(s);
        sprintf(s, "RSSI %d SNR %.1d", LMIC.rssi, LMIC.snr);
        Serial.println(s);
      }
      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    default:
      Serial.println(F("Unknown event"));
      break;
  }
}

void do_send(osjob_t* j) {  
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND)
  {
    Serial.println(F("OP_TXRXPEND, not sending"));
  }
  else
  {
    // Prepare upstream data transmission at the next possible time.
    build_packet();
    LMIC_setTxData2(1, txBuffer, sizeof(txBuffer), 0);
    Serial.println(F("Packet queued"));
    digitalWrite(BUILTIN_LED, HIGH);
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("TTN Mapper"));
  
  //Turn off WiFi and Bluetooth
  WiFi.mode(WIFI_OFF);
  btStop();
  
  GPSSerial.begin(9600, SERIAL_8N1, GPS_TX, GPS_RX);
  GPSSerial.setTimeout(2);

  GPSSerial.println(F(PMTK_SET_NMEA_OUTPUT_RMCGGA));
  GPSSerial.println(F(PMTK_SET_NMEA_UPDATE_1HZ));   // 1 Hz update rate
  
  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  
  LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
  
  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band

  // Disable link check validation
  LMIC_setLinkCheckMode(0);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF7,14); 
  
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);
  
}

void loop() {
  
  // Update GPS
  while (GPSSerial.available())
  {
    gps.encode(GPSSerial.read());
  }
  
  delay(1000);
  
  if (gps.location.isValid())
  {
    Serial.println("GPS has fixed.");
    do_send(&sendjob);
    os_runloop_once();
  }
  else
  {
    Serial.println("No fix yet...");
  }
}
