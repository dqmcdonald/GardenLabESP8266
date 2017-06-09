/*GardenLab ESP8266 Wemos D1 Mini
   Responsible for receiving data over serial from the Arduino and pass it on to the
   Raspberry Pi Server.
   The Arduino controls the power for this board so to save battery it's only turned on when
   the Arduino is ready to send data
   Quentin McDonald
   May 2017
*/

//This example will set up a static IP - in this case 192.168.1.99

#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>

/* Pin definitions: */
#define RX D1
#define TX D2


SoftwareSerial softSerial = SoftwareSerial(RX, TX);

String data_string = "";
String last_data_string="";

String esid;
long rssi;
long wifi_strength;
char buff[128];

WiFiServer server(80);
IPAddress ip(192, 168, 1, 99); // where xx is the desired IP Address
IPAddress gateway(192, 168, 1, 1); // set gateway to match your network


int num_bytes = 0; // Number of bytes to read
int bytes_read = 0; // Number currently read

void setup() {
  Serial.begin(9600);
  delay(10);
  softSerial.begin(9600);
  softSerial.flush();

  // Start the server
  server.begin();
  Serial.println("Server started");

pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED,LOW);
  setupWifi();


  digitalWrite(BUILTIN_LED,HIGH);
  pinMode(2, OUTPUT);

  
 
}



void loop() {


 
  if ( softSerial.available()) {

    if ( num_bytes == 0 ) {
      num_bytes = (int)softSerial.read();
      Serial.print("Num bytes to read from Arduino =");
      Serial.println(num_bytes);
       digitalWrite(BUILTIN_LED,LOW);
    } else {
      char c = softSerial.read();
      data_string += c;
      bytes_read++;
      // If we have read the whole string post it to server
      if ( num_bytes == bytes_read ) {
        digitalWrite(BUILTIN_LED,HIGH);
        delay(500);
          digitalWrite(BUILTIN_LED,LOW);
        post_data(data_string);
        last_data_string = data_string;
        data_string = "";
        num_bytes = 0;
        bytes_read = 0;
      }
    }


  }
 

}




void setupWifi() {
  EEPROM.begin(512);
  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
  Serial.print(F("Setting static ip to : "));
  Serial.println(ip);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(esid);
  IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your network
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(esid.c_str(), epass.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  // Print the IP address
  Serial.print("Use this URL : ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

}


void post_data(const String& dstring) {


  Serial.println("Posting data string:");
  Serial.println(data_string);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const char* host = "192.168.1.13";
  const int httpPort = 8080;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  //  // We now create a URI for the request
  String url = "/input/";


  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Accept: */*\r\n" +
               "Content-Length: " + dstring.length() + "\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n\r\n");
  client.println(dstring);

  delay(10);

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");



}



