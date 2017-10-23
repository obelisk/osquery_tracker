#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* ssid     = "fbguest";
const char* password = "m0vefast";
const char* pr_url = "/search/issues?q=type:pr+state:open+repo:facebook/osquery";
const char* issue_url = "/search/issues?q=type:issue+state:open+repo:facebook/osquery"; 
const char* host = "api.github.com";

// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "87 08 95 D2 0E A2 B8 68 CF 44 DE 3D 04 22 F1";

static const unsigned char osquerylogo [] = {
0x7F, 0x00, 0x80, 0x00, 0x3F, 0x80, 0xC0, 0x01, 0x1F, 0xC0, 0xE0, 0x03, 0x0F, 0xE0, 0xF0, 0x07,
0x07, 0xF0, 0xF8, 0x0F, 0x03, 0xF8, 0xFC, 0x1F, 0x01, 0xFC, 0xFE, 0x3F, 0x00, 0xFE, 0xFF, 0x7F,
0x01, 0xFF, 0xFF, 0xFE, 0x03, 0xFC, 0x7F, 0xFC, 0x07, 0xF8, 0x3F, 0xF8, 0x0F, 0xF0, 0x1F, 0xF0,
0x1F, 0xE0, 0x0F, 0xE0, 0x3F, 0xC0, 0x07, 0xC0, 0x7F, 0x80, 0x03, 0x80, 0xFF, 0x00, 0x01, 0x00,
0x00, 0x80, 0x01, 0xFF, 0x01, 0xC0, 0x03, 0xFE, 0x03, 0xE0, 0x07, 0xFC, 0x07, 0xF0, 0x0F, 0xF8,
0x0F, 0xF8, 0x1F, 0xF0, 0x1F, 0xFC, 0x3F, 0xE0, 0x3F, 0xFE, 0x7F, 0xC0, 0x7F, 0xFF, 0xFF, 0x80,
0xFE, 0xFF, 0x7F, 0x00, 0xFC, 0x7F, 0x3F, 0x80, 0xF8, 0x3F, 0x1F, 0xC0, 0xF0, 0x1F, 0x0F, 0xE0,
0xE0, 0x0F, 0x07, 0xF0, 0xC0, 0x07, 0x03, 0xF8, 0x80, 0x03, 0x01, 0xFC, 0x00, 0x01, 0x00, 0xFE
};

Adafruit_SSD1306 display = Adafruit_SSD1306();
uint16_t lastPull;
uint16_t lastIssue;

void setup() {
  Serial.begin(115200);
 
  Serial.println();
  Serial.println("osquery dev board v0.1");
  Serial.println("Connecting to wifi");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lastPull = 9999;
  lastIssue = 9999;

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay();
  
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.drawBitmap(48, 0, osquerylogo, 32, 32, WHITE);
  display.display();
  display.setTextSize(2);
 
  Serial.println();
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

String getCount(const char* url) {
  WiFiClientSecure client;
  if (!client.connect(host, 443)) {
    Serial.println("connection failed");
    return String();
  }

  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: osqueryIssueTracker-obelisk\r\n" +
               "Connection: close\r\n\r\n");
 
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      //Serial.println("headers received");
      break;
    }
  }
  String json = client.readStringUntil('}');
  size_t comma, colon;
  for (size_t i = 0; i < json.length(); ++i) {
    if (json[i] == ':') {
      colon = i;
      continue;
    }
    if (json[i] == ',') {
      comma = i;
      break;
    }
  }
  client.readStringUntil('\n');
  if (comma - colon > 5) {
    return String();
  }
  return json.substring(colon+1, comma);
}
 
void loop() {
  delay(5000);

  String prCount = getCount(pr_url);
  String isCount = getCount(issue_url);

  // Probably hit a rate limit try again next time
  if (prCount.length() == 0 || isCount.length() == 0) {
    return;
  }
  Serial.println("Pull requests: " + prCount);
  Serial.println("Issues: " + isCount);
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("P: " + prCount);
  display.print("\nI: " + isCount);
  display.drawBitmap(90, 0, osquerylogo, 32, 32, WHITE);
  display.display();
}