/*GardenLab ESP8266 Wemos D1 Mini
   Responsible for receiving data over serial from the Arduino and pass it on to the
   Raspberry Pi Server.
   Quentin McDonald
   May 2017
*/

//This example will set up a static IP - in this case 192.168.1.99

#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

/* Pin definitions: */
#define RX D1
#define RX D2


#define PUSH_DATA_FREQUENCY 20 // Frequency of pushing data in seconds

SoftwareSerial softSerial = SoftwareSerial(RX, TX);


long int next_push_time = 0;
String esid;
long rssi;
long wifi_strength;
char buff[128];

WiFiServer server(80);
IPAddress ip(192, 168, 1, 99); // where xx is the desired IP Address
IPAddress gateway(192, 168, 1, 1); // set gateway to match your network



void setup() {
  Serial.begin(9600);
  delay(10);
  softSerial.begin(9600);

  // Start the server
  server.begin();
  Serial.println("Server started");

  setupWifi();

  setupATO();





  pinMode(2, OUTPUT);



}



void loop() {

  ArduinoOTA.handle();

  if ( softSerial.available()) {
    char rec = softSerial.read();
  }


  handleWebServer();

  pushData();


}

void setupATO() {


  ArduinoOTA.setHostname("GardenLabESP8266");
  ArduinoOTA.setPassword("admin");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IPess: ");
  Serial.println(WiFi.localIP());
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

void handleWebServer() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  rssi = WiFi.RSSI();  // eg. -63
  wifi_strength = (100 + rssi);


  // Wait until the client sends some data
  Serial.println("new client");
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<meta http-equiv='refresh' content='10'>");

  client.print("<H1>Welcome to The Garden Lab");
  client.print(" </H1>");
  client.print("<h3>Wifi Strength is: ");
  client.print(wifi_strength);
  client.print("% <br>");

  client.print("<canvas id=\"myCanvas\" width=\"300\" height=\"25\" style=\"border:1px solid #000000;\"></canvas><br><br>");

  client.print("<script>");
  client.print("var c = document.getElementById(\"myCanvas\");");
  client.print("var ctx = c.getContext(\"2d\");");
  client.print("ctx.fillStyle = \"#FF0000\";");
  wifi_strength = (int)(wifi_strength / 100.0 * 300);
  snprintf(buff, sizeof(buff), "ctx.fillRect(0,0,%d,25);", wifi_strength);
  client.print(buff);
  client.println("</script></H3>");


  client.println("</html>");

  delay(1);
  Serial.println("Client disconnected");
  Serial.println("");
}


void pushData() {

  if ( next_push_time != 0 && next_push_time > millis() ) {
    return;
  }
  next_push_time = millis() + PUSH_DATA_FREQUENCY * 1000;

  Serial.println("Pushing data");

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

  String content = "temp=5.00&voltage=42.0";
  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Accept: */*\r\n" +
               "Content-Length: " + content.length() + "\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n\r\n");
  client.println(content);

  delay(10);

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");



}



