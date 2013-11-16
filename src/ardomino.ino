/////////////////////////////////////////////////////////////////////////////
//
// Arduino Sketch for the Ardomino Project.
//
// NOTE: This is an experimental "new version" that communicates
//       plain text lines over a TCP socket.
//       To be merged with the original sketch in alfcrisci/ardomino.
//
// Biometeorological ArdOmino Skecth to monitor Air Temperature
// and Relative Humidity.
//
// Application Case https://github.com/alfcrisci/Ardomino presented
// OfficinaIbimet IBIMET CNR http://www.fi.ibimet.cnr.it/
//
// Author: Alessandor Matese - Alfonso Crisci - OfficinaIbimet
//
// General scheme and function are done by Mirko Mancini in his work
// thesis DESIGN AND IMPLEMENTATION OF AN AMBIENT INTELLIGENCE SYSTEM BASED
// ON ARDUINO AND ANDROID Universita di Parma FAC. DI INGEGNERIA CORSO DI
// LAUREA IN INGEGNERIA INFORMATICA
//
// Mirko Mancin <mirkomancin90@gmail.com>
//   website:  www.mancio90.it
//   forum http://forum.arduino.cc/index.php?topic=157524.0;wap2
//
// Samuele Santi <redshadow@hackzine.org>
//   Author of this experimental version, trying to solve some problems
//   (mostly with wifi connectivity).
//   website: http://hackzine.org - http://samuelesanti.com
//   github: https://github.com/rshk
//
// Library reference
//   https://github.com/adafruit/DHT-sensor-library
//   https://github.com/harlequin-tech/WiFlyHQ
//
/////////////////////////////////////////////////////////////////////////////


#include "settings.h"  // See the README file for more information

const int MAX_CONNECTION_TIME = 0; // In milliseconds


// Easy streaming
//------------------------------------------------------------

template<class T> inline Print &operator<<(Print &obj, T arg) {
  obj.print(arg);
  return obj;
}
const char endl[] = "\r\n";  // CR+LN linefeed for serial port


// Communication with the DHT
//------------------------------------------------------------

#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT22

// Initialize the interface w/DHT
DHT dht(DHTPIN, DHTTYPE);

// Struct to hold sensor data
struct SensorData {
  float humidity;
  float temperature;
};

// Used to serialize data, for transmission
// over the wire.
// Note: we should send BIG endian data, but
// right now we are using LITTLE endian, which
// seems to be the default in x86_64..
union PackedSensorData {
  struct SensorData values;
  char packed[sizeof(struct SensorData)];
};

union PackedSensorData data;


/**
 * Setup the communication with DHT sensor
 */
void setup_dht() {
  Serial.println("[DHT] Setting up DHT module");
  dht.begin();
}


/**
 * Read sensor data from the DHT
 */
void loop_dht() {
  Serial.println("[DHT] Reading sensors data");
  data.values.humidity = dht.readHumidity();
  data.values.temperature = dht.readTemperature();
}



// Serial-port communication
//------------------------------------------------------------

void setup_serial() {
  Serial.begin(SERIAL_SPEED);
  Serial << endl << endl;
  Serial.println("----------------------------------------");
  Serial.println("        Ardomino Serial Console         ");
  Serial.println("----------------------------------------");
  Serial << "INFO: Starting initialization sequence" << endl;
}


/**
 * Send read data through the serial port
 */
void loop_serial() {
  if (isnan(data.values.humidity) || isnan(data.values.temperature)) {
    Serial << "ERROR: Failed reading values from DHT" << endl;
  }
  else {
    Serial << "DATA:"
           << " RH=" << data.values.humidity
           << " T=" << data.values.temperature
           << endl;
  }
}


// Wifi communication (via WiFly module)
//------------------------------------------------------------

#include <WiFlyHQ.h>
#include <SoftwareSerial.h>

SoftwareSerial wifiSerial(8,9);
WiFly wifly;

// Todo: read these from some configuration file!
const char mySSID[] = WIFI_SSID;
const char myPassword[] = WIFI_PASSWORD;

// The server to which to POST the data.values..
const char site[] = SERVER_ADDR;
const unsigned int site_port = SERVER_PORT;

void terminal();

// Total connection time
uint32_t connectTime = 0;


/**
 * Setup function for the WiFly module
 */
void setup_wifly() {
    char buf[32];

    Serial << "[wifly] INFO: WiFly Module initialization" << endl;
    Serial << "[wifly] INFO: Free memory: " << wifly.getFreeMemory() << endl;

    wifiSerial.begin(9600);

    if (!wifly.begin(&wifiSerial, &Serial)) {
      Serial << "[wifly] ERROR: Failed to start wifly" << endl;
      terminal();
    }

    // Join wifi network if not already associated
    if (!wifly.isAssociated()) {

	// Setup the WiFly to connect to a wifi network
      Serial << "[wifly] INFO: Joining network" << endl;

      wifly.setSSID(mySSID);
      wifly.setPassphrase(myPassword);
      wifly.enableDHCP();

      if (wifly.join()) {
	Serial << "[wifly] INFO: Joined wifi network" << endl;
      }
      else {
	Serial << "[wifly] ERROR: Failed to join wifi network" << endl;
	terminal(); // Open wifi <-> serial communication
      }
    }
    else {
      Serial << "[wifly] INFO: Already joined network" << endl;
    }

    Serial << "[wifly]    MAC: " << wifly.getMAC(buf, sizeof(buf)) << endl;
    Serial << "[wifly]    IP: " << wifly.getIP(buf, sizeof(buf)) << endl;
    Serial << "[wifly]    Netmask: " << wifly.getNetmask(buf, sizeof(buf)) << endl;
    Serial << "[wifly]    Gateway: " << wifly.getGateway(buf, sizeof(buf)) << endl;

    wifly.setDeviceID(DEVICE_ID);
    Serial << "[wifly]    DeviceID: " << wifly.getDeviceID(buf, sizeof(buf)) << endl;

    if (wifly.isConnected()) {
      Serial << "[wifly] INFO: Old connection active. Closing" << endl;
      wifly.close();
    }

    // Try to make a HTTP request..
    if (wifly.open(site, site_port)) {
      Serial << "[wifly] INFO: Connected to " << site << endl;
      wifly.println("HELLO");
    }
    else {
      Serial << "[wifly] ERROR: Failed to connect" << endl;
    }
}


void loop_wifly() {

  int available;

  // If not already connected, we need to establish
  // a TCP connection to the server

  if (wifly.isConnected() == false) {
    Serial << "[wifly] INFO: Connecting to server" << endl;

    if (wifly.open(site, site_port)) {
      Serial << "[wifly] INFO: Connected to" << site << ":"
	     << site_port << endl;
      connectTime = millis();
    }
    else {
      Serial << "[wifly] ERROR: Failed connecting to "
	     << site << ":" << site_port << endl;
    }
  }
  else {
    available = wifly.available();

    if (available < 0) {
      Serial << "[wifly] WARNING: Disconnected";
    }

    else {
      // Print data from WiFly to serial port
      if (available > 0) {
	String recvdata = "";
        while (wifly.available() > 0) {
	  recvdata += wifly.read();
	}
	Serial << "[wifly] INFO: Received data: " << recvdata
	       << " %%end%%" << endl;
      }

      // Send sensor data
      Serial << "INFO: WiFly: Sending sensors data" << endl;

      // wifly.print("DATA:");
      // wifly.print(" T=");
      // wifly.print(data.values.temperature);
      // wifly.print(" RH=");
      // wifly.print(data.values.humidity);
      // wifly.println();

      // Send packed data over wifi
      wifly.print("ARDOMINO");
      wifly.print(data.packed);

      // Disconnect after reaching MAX_CONNECTION_TIME
      if (MAX_CONNECTION_TIME &&
	  ((millis() - connectTime) > MAX_CONNECTION_TIME)) {
	Serial << "[wifly] INFO: Max connection time reached."
	       "Disconecting." << endl;
	wifly.close();
      }
    }
  }
}


// Connect the WiFly serial to the serial monitor.
void terminal() {
  Serial << "[wifly] Opening WiFly <-> Serial communication." << endl;
  while (1) {
    if (wifly.available() > 0) {
      Serial.write(wifly.read());
    }
    if (Serial.available() > 0) {
      wifly.write(Serial.read());
    }
  }
}





// Standard setup/loop functions
//------------------------------------------------------------

void setup() {
  setup_serial();
  setup_dht();
  setup_wifly();
  Serial << "*** ARDOMINO Initialization done" << endl;
}


void loop() {
  loop_dht();
  loop_serial();
  loop_wifly();
  delay(500);
}
