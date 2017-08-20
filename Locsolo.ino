#include <ESP8266WiFi.h>
//#include <SPI.h>
/*
   Set time: (store current ticks pluss received ticks)
    - Check if sum time is more then uLong
      - If sum is more then uLong, reset system ticks
      - If there were previously stored "pinEndTicks", recalculate those with the reseted system ticks
*/
///////////////////////D1, D2, D3, D4, D5, D6, D7
const byte pinNbrs[] = {5, 4, 0, 2, 14, 12, 13}; //D3(0) and D4(2) are blinking on reset -- 15(D8) is the "is wifi connected indicator"
const String pinTexts[] = {"Ajtó előtt (D1)", "Járólap mellett (D2)", "nemtudni - nagy (D3)", "Tuják alatt (D4)", "nemtudni - nagy (D5)", "Rózsák, parik (D6)", "Csöpögő (D7)"};
unsigned long pinEndTicks[] = {0, 0, 0, 0, 0, 0, 0};

const char* ssid = "OpenWrt_w";      // your network SSID (name)
const char* password = "W2nkl3r.";   // your network password
int keyIndex = 0;                 // your network key Index number (needed only for WEP)
bool wasWifiConnected = false;

// NETWORK: Static IP details...
IPAddress ip(192, 168, 1, 202);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

int status = WL_IDLE_STATUS;

WiFiServer server(80);

typedef struct record_type
{
  unsigned long authStartTimeMillis;
  String ipAddres;
};

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void setPinHigh(int pinNbr) {
  Serial.print("Setting pin HIGH:"); Serial.println(pinNbr);
  digitalWrite(pinNbr, HIGH);
}

void setPinLow(int pinNbr) {
  Serial.print("Setting pin LOW:"); Serial.println(pinNbr);
  digitalWrite(pinNbr, LOW);
}

void resetPinEndTick(int pinNbr) {
  for (int i = 0; i < 7; i++) {
    if (pinNbrs[i] == pinNbr) pinEndTicks[i] = 0;
  }
}

void calculateAndSetEndTick(int pinNbr, unsigned long timeSpan) {
  for (int i = 0; i < 7; i++) {
    if (pinNbrs[i] == pinNbr) pinEndTicks[i] = millis() + timeSpan; //implement variable overflow logic
  }
}

//set pin low and reset pinEndTick for the pin
void abortPin(int pinNbr) {
  setPinLow(pinNbr);
  resetPinEndTick(pinNbr);
}

//set pin high and set pinEndTick
void schedulePin(int pinNbr, unsigned long timeSpan) {
  setPinHigh(pinNbr);
  calculateAndSetEndTick(pinNbr, timeSpan);
}

void serverSetup()
{
  //start UART and the server
  server.begin();
  server.setNoDelay(true);
  //printWifiStatus();
}
bool checkIfWifiConnected() //true if connected
{
  bool isConnected = (WiFi.status() == WL_CONNECTED && 0 > WiFi.RSSI() && WiFi.RSSI() > -93); //below -93dBm the connection is unrelyable

  if (isConnected) {
    //if (!wasWifiConnected)
    {
      Serial.print("Wifi Connected...");
      //UNCOMMENT THIS line!!
      setPinHigh(15); 
      wasWifiConnected = true;
    }
  }
  else {
    Serial.print("Wifi not yet connected... ["); Serial.print(WiFi.RSSI()); Serial.print(" dBm]");
    setPinLow(15);
    wasWifiConnected = false;
  }

  return isConnected;
}

void setup() {
  Serial.begin(115200);
  pinMode(15, OUTPUT);
  setPinLow(15);
  for (byte i = 0; i < 7; i++) {
    pinMode(pinNbrs[i], OUTPUT);
  }
  WiFi.config(ip, gateway, subnet);
  Serial.print("begin connnect");
  WiFi.begin(ssid, password);

  Serial.print("\nConnecting to "); Serial.println(ssid);
  uint8_t i = 0;
  while (!checkIfWifiConnected() && i++ < 120) delay(500);
  if (i == 21) {
    Serial.print("Could not connect to"); Serial.println(ssid);
    while (1) delay(500);
  }

  serverSetup();
}

void sendControlsHtml(WiFiClient client)
{
  // send a standard http response header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");  // the connection will be closed after completion of the response
  //client.println("Refresh: 10");  // refresh the page automatically every 10 sec
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<title>Locsoló</title>");
  client.println("<meta charset=\"utf-8\">");
  client.println("<div style=\"width:400px\">");
  // output the value of each analog input pin
  for (int i = 0; i < 7; i++) {
    bool sensorReading = digitalRead(pinNbrs[i]);

    client.print(pinTexts[i]);
    client.print(" - pin ");
    client.print(pinNbrs[i]);
    client.print(".");

    client.print("<form action=\"\" method=\"POST\">");
    client.print("<button name=\"pinNbr\" value=\"");
    client.print(pinNbrs[i]);
    if (sensorReading)
    {
      client.print("\">Abort</button>");
      /*
         client.print(" Remaining time: ");
         client.print("12");
         client.print(" minute(s).");
      */
    }
    else if (!sensorReading)
    {
      client.print("\">Schedule</button>");
      client.print(" <input type=\"number\" name=\"timeSpan\" min=\"1\" max=\"60\" value=\"15\">");
      client.print(" minute.");
    }
    client.print("</form>");
    client.println("<br />");
  }
  client.println("</div>");
  client.println("</html>");
}

void reactOnClientResponse(String postResp, String clientResp, WiFiClient client) {
  //check if any of the pin status shall change
  if (postResp.indexOf("pinNbr=") >= 0)
  {
    //parse pin number to change
    String substr = postResp.substring(postResp.indexOf("pinNbr=") + 7);
    int pinNumber = substr.toInt();
    Serial.print("Pin number received: ");
    Serial.println(pinNumber);

    //Change pin status
    if (digitalRead(pinNumber))
    {
      abortPin(pinNumber);
    }
    else
    {
      schedulePin(pinNumber, 10000);
    }
  }
}

bool checkTickEnds()
{
  bool tickEndedForPin = false;

  unsigned long now = millis();
  for (int i = 0; i < 7; i++)
  {
    if (pinEndTicks[i] >= now)
    {
      tickEndedForPin = true;
      setPinLow(pinNbrs[i]);
      pinEndTicks[i] = 0;
    }
  }

  return tickEndedForPin;
}

void loop() {
  serverSetup();
  checkIfWifiConnected();

  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    Serial.print("new client [");
    Serial.print(client.remoteIP());
    Serial.println("]");

    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    String clientResp = "";
    String postResp = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        //Serial.write(c);
        clientResp += c;

        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        // or expect a POST or GET
        if (c == '\n' && currentLineIsBlank) {

          //if client sent a POST
          if (clientResp.indexOf("POST") >= 0)
          {
            while (client.connected() && client.available())
            {
              c = client.read();
              //Serial.write(c);
              clientResp += c;
              postResp += c;
            }

            reactOnClientResponse(postResp, clientResp, client);
          }

          sendControlsHtml(client);

          Serial.println("received: ");
          Serial.println(clientResp);
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();
    Serial.println("client disonnected");
  }

  if (checkTickEnds())
  {
    //sendControlsHtml(client);
  }

  server.stop();

}




