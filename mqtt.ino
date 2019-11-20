#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* WiFi Access Point *********************************/

#define WLAN_SSID         ""
#define WLAN_PASS         ""

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER        "192.168.1.101"
#define AIO_SERVERPORT    1883                   // use 8883 for SSL
#define AIO_USERNAME      ""
#define AIO_KEY           ""

#define mqtt_update_rate   5    //Update rate in seconds

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish     mqtt_spa_state_water_temperature    = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/spa/state/water_temperature");
Adafruit_MQTT_Publish     mqtt_spa_state_lights               = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/spa/state/lights");
Adafruit_MQTT_Publish     mqtt_spa_state_jets                 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/spa/state/jets");
Adafruit_MQTT_Publish     mqtt_spa_state_blower               = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/spa/state/blower");
Adafruit_MQTT_Publish     mqtt_spa_state_status               = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/spa/state/status");

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe   mqtt_spa_command_water_temperature  = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/spa/command/water_temperature");
Adafruit_MQTT_Subscribe   mqtt_spa_command_lights             = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/spa/command/lights");
Adafruit_MQTT_Subscribe   mqtt_spa_command_jets               = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/spa/command/jets");
Adafruit_MQTT_Subscribe   mqtt_spa_command_blower             = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/spa/command/blower");

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void mqttconnect();

uint32_t x = 0;

float temperature = 37.5;

void mqtt_init(void)
{
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println(F("MQTT Initialisation"));

  // Connect to WiFi access point.
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&mqtt_spa_command_water_temperature);
  mqtt.subscribe(&mqtt_spa_command_lights);
  mqtt.subscribe(&mqtt_spa_command_jets);
  mqtt.subscribe(&mqtt_spa_command_blower);
}

void mqtt_command_handler(void)
{

}

void mqtt_loop(void)
{
  static unsigned long time_now = 0, time_prv = 0;

  time_now = millis();

  if ((time_now - time_prv) > (mqtt_update_rate * 1000))
  {
    time_prv = millis();

    // Ensure the connection to the MQTT server is alive (this will make the first
    // connection and automatically reconnect when disconnected).  See the MQTT_connect
    // function definition further below.
    mqtt_connect();

    // this is our 'wait for incoming subscription packets' busy subloop
    // try to spend your time here

    /*Adafruit_MQTT_Subscribe *subscription;
      while ((subscription = mqtt.readSubscription())) {
      if (subscription == &mqtt_spa_command_water_temperature) {
        Serial.print(F("MQTT Command received: "));
        Serial.println((char *)mqtt_spa_command_water_temperature.lastread);
      }
      Serial.println();
      }*/

    Adafruit_MQTT_Subscribe *subscription;

    if ((subscription = mqtt.readSubscription()))
    {
      if (subscription == &mqtt_spa_command_water_temperature)
      {
        Serial.print(F("MQTT Spa Water Temperature Command received: "));
        Serial.println((char *)mqtt_spa_command_water_temperature.lastread);
        balboa_set_desired_temperature(atof((char *)mqtt_spa_command_water_temperature.lastread));
      }
      if (subscription == &mqtt_spa_command_lights)
      {
        Serial.print(F("MQTT Spa Lights Command received: "));
        Serial.println((char *)mqtt_spa_command_lights.lastread);
        balboa_set_lights(atoi((char *)mqtt_spa_command_lights.lastread));
      }
      if (subscription == &mqtt_spa_command_jets)
      {
        Serial.print(F("MQTT Spa Jets Command received: "));
        Serial.println((char *)mqtt_spa_command_jets.lastread);
        balboa_set_jets(atoi((char *)mqtt_spa_command_jets.lastread));
      }
      if (subscription == &mqtt_spa_command_blower)
      {
        Serial.print(F("MQTT Spa Blower Command received: "));
        Serial.println((char *)mqtt_spa_command_blower.lastread);
        balboa_set_blower(atoi((char *)mqtt_spa_command_blower.lastread));
      }
      Serial.println();
    }

    // Now we can publish stuff!
    //Serial.print(F("\nSending photocell val "));
    //Serial.print(x);
    //Serial.print("...");
    /*if (! mqtt_spa_state_water_temperature.publish(bbstatus.temperature)) {
      //Serial.println(F("Failed"));
    } else {
      //Serial.println(F("OK!"));
    }*/

    mqtt_spa_state_water_temperature.publish(bbstatus.temperature);
    mqtt_spa_state_lights.publish(bbstatus.lights);
    mqtt_spa_state_jets.publish(bbstatus.jets);
    mqtt_spa_state_blower.publish(bbstatus.blower);
    mqtt_spa_state_status.publish(bbstatus.status);

    // ping the server to keep the mqtt connection alive
    // NOT required if you are publishing once every KEEPALIVE seconds
    /*
      if(! mqtt.ping()) {
      mqtt.disconnect();
      }
    */
  }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void mqtt_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("Connected!");
  Serial.println();
}
