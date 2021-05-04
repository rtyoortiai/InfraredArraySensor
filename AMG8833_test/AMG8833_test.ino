// Program Ver. IAS003

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include "AMG8833.h"

// I2C通信のピンID
#define SDA_PIN_ID  (21)
#define SCL_PIN_ID  (22)

// 赤外線アレイセンサのインスタンス生成
AMG8833 amg8833(AMG88_ADDR);

// WiFiのSSID, Password
const char *ssid = "your_ssid";
const char *password = "your_password";

// サーバーのインスタンス生成
WebServer server(80);

// heat関数(heat.cpp)の宣言
uint32_t heat(float x);

// グローバル変数
float temp[64];
unsigned long lastT = 0;

void handleRoot() {
    String msg = "hello";  // レスポンスメッセージを用意
    Serial.println(msg);
    server.send(200, "text/plain", msg);  // レスポンスを返信する
}

void handleCapture() {  // /captureをアクセスされたときの処理
    char buf[400];

    snprintf(buf, 400,  // thermal.svgというファイルをアクセスするHTMLを作る
   "<html>\
    <body>\
        <div align=\"center\">\
            <img src=\"/thermal.svg\" width=\"400\" height=\"400\" />\
        </div>\
    </body>\
    </html>"
          );
    server.send(200, "text/html", buf);  // HTMLを返信する
}

void handleStream() {  // /streamをアクセスされたときの処理
    char buf[400];

    snprintf(buf, 400,  // thermal.svgというファイルをアクセスするHTMLを作る
   "<html>\
    <head>\
        <meta http-equiv='refresh' content='0.5'/>\
    </head>\
    <body>\
        <div align=\"center\">\
            <img src=\"/thermal.svg\" width=\"400\" height=\"400\" />\
        </div>\
    </body>\
    </html>"
          );
    server.send(200, "text/html", buf);  // HTMLを返信する
}

void handleThermal() {  // /thermal.svgの処理関数
    String out = "";
    char buf[100];  // AMG8833の画素データを
    out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n";
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            float t = temp[(8 - y - 1) * 8 + 8 - x - 1];
            uint32_t color = heat(map(constrain((int)t, 0, 40), 0, 40, 0, 100) / 100.0);
            Serial.printf("%2.1f ", t);
            sprintf(buf, "<rect x=\"%d\" y=\"%d\" width=\"50\" height=\"50\" fill=\"#%06x\" />\n",
                x * 50, y * 50, color);
            out += buf;
        }
        Serial.println();
    }
    Serial.println("--------------------------------");
    out += "</svg>\n";

    server.send(200, "image/svg+xml", out);
}

void handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";

    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }

    server.send(404, "text/plain", message);
}

void setup() {
    Serial.begin(115200);
    while (!Serial) ;

    pinMode(SDA_PIN_ID, INPUT_PULLUP);  // SDAをプルアップする
    pinMode(SCL_PIN_ID, INPUT_PULLUP);  // SCLをプルアップする
    Wire.begin();
    amg8833.begin();
    
    WiFi.mode(WIFI_STA);
    Serial.println("");
    
    // Wait for connection
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("thermoCam")) {
        Serial.println("MDNS responder started");
    }

    server.on("/", handleRoot);
    server.on("/capture", handleCapture);  // /captureの処理関数を登録  ----④
    server.on("/stream", handleStream);  // /streamの処理関数を登録
    server.on("/thermal.svg", handleThermal);  // /thermal.svgの処理関数を登録
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("access: http://thermoCam.local/capture for still image");
    Serial.println("access: http://thermoCam.local/stream for stream image");
}

void loop() {
    server.handleClient();

    if ((millis() - lastT) > 500) {
        lastT = millis();

        amg8833.read(temp);  // AMG8833から温度データを取得
    }
}
