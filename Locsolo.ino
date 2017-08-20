#include <ESP8266WiFi.h>
//#include <SPI.h>

///////////////////////D1, D2, D3, D4, D5, D6, D7, D8
const byte pinNbrs[] = {5, 4, 0, 2, 14, 12, 13, 15}; //D3(0) and D4(2) are blinking on reset

const char* ssid = "OpenWrt_w";      // your network SSID (name)
const char* password = "W2nkl3r.";   // your network password
int keyIndex = 0;                 // your network key Index number (needed only for WEP)

// NETWORK: Static IP details...
IPAddress ip(192, 168, 1, 201);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

int status = WL_IDLE_STATUS;

WiFiServer server(80);

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

void setup() {
  Serial.begin(115200);

  for (byte i = 0; i < 8; i++) {
    pinMode(pinNbrs[i], OUTPUT);
  }

  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);

  Serial.print("\nConnecting to "); Serial.println(ssid);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if (i == 21) {
    Serial.print("Could not connect to"); Serial.println(ssid);
    while (1) delay(500);
  }
  //start UART and the server
  server.begin();
  server.setNoDelay(true);
  printWifiStatus();
}

void reactOnClientResponse(String clientResp) {
  //check if any of the pin status shall change
  if (clientResp.indexOf("pinNbr=") >= 0)
  {
    //parse pin number to change
    String substr = clientResp.substring(clientResp.indexOf("pinNbr=") + 7);
    int pinNumber = substr.toInt();
    Serial.print("Pin number received: ");
    Serial.println(pinNumber);

    //Change pin status
    if (digitalRead(pinNumber))
    {
      setPinLow(pinNumber);
    }
    else
    {
      setPinHigh(pinNumber);
    }
  }
}

void sendHttpContentToClient(WiFiClient client)
{
  // send a standard http response header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");  // the connection will be closed after completion of the response
  //client.println("Refresh: 5");  // refresh the page automatically every 5 sec
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  // output the value of each analog input pin
  for (int i = 0; i < 8; i++) {
    bool sensorReading = digitalRead(pinNbrs[i]);

    client.print("pin ");
    client.print(pinNbrs[i]);
    client.print(" ");

    client.print("<form action=\"\" method=\"POST\">");
    client.print("<input type =\"checkbox\" name=\"categories1\" value = \"2\" onchange=\"this.form.submit();\"");
    if (sensorReading) client.print(" checked ");
    client.print(">3</input>");
    //client.print(pinNbrs[i]);
    client.print("</form>");
    client.println("<br />");
  }
  client.println("</html>");
}

void loop() {
  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    Serial.print("new client [");
    Serial.print(client.remoteIP());
    Serial.println("]");

    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    String clientResp = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);

        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          while (client.available())
          {
            c = client.read();
            Serial.write(c);
            clientResp += c;
          }
          reactOnClientResponse(clientResp);

          sendHttpContentToClient(client);

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
}



