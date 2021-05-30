// Program Ver. IAS004

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include "AMG8833.h"
#include "SG90.h"

// I2C通信のピンID
#define SDA_PIN_ID  (21)
#define SCL_PIN_ID  (22)

// 赤外線アレイセンサのセル数
#define AMG_CELL_NUM  (8)
// 赤外線アレイセンサの視野角
#define AMG_VIEW_ANGLE  (60)

// サーボモータ(SG90)制御(PWM)用GPIOピンID
#define PAN_PIN_ID  (4)
#define TILT_PIN_ID (16)

// サーボモータ(SG90)制御範囲
#define PAN_MIN_ANGLE   (-90)
#define PAN_MAX_ANGLE   (90)
#define TILT_MIN_ANGLE  (-40)
#define TILT_MAX_ANGLE  (40)

// 赤外線アレイセンサのインスタンス生成
AMG8833 amg8833(AMG88_ADDR);

// サーボモータ(SG90)のインスタンス
SG90 pan;       // 水平方向
SG90 tilt;      // 上下方向

// WiFiのSSID, Password
const char *ssid = "your_ssid";
const char *password = "your_password";

// サーバーのインスタンス生成
WebServer server(80);

// heat関数(heat.cpp)の宣言
uint32_t heat(float x);

// グローバル変数
float temp[AMG_CELL_NUM*AMG_CELL_NUM];
unsigned long lastT = 0;

const float Pi = atan(1) * 4;
#define d2r(d) ((d) / 180.0 * Pi)
#define r2d(r) ((r) * 180.0 / Pi)
const float l = AMG_CELL_NUM / 2.0 / tan(d2r(AMG_VIEW_ANGLE/2.0));

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
    for (int y = 0; y < AMG_CELL_NUM; y++) {
        for (int x = 0; x < AMG_CELL_NUM; x++) {
            float t = temp[(AMG_CELL_NUM - y - 1) * AMG_CELL_NUM + AMG_CELL_NUM - x - 1];
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

    pan.begin(PAN_PIN_ID, 15, PAN_MIN_ANGLE, PAN_MAX_ANGLE);      // チャネル15で初期化
    tilt.begin(TILT_PIN_ID, 14, TILT_MIN_ANGLE, TILT_MIN_ANGLE);  // チャネル14で初期化

    pan.write(0);      // 水平方向、正面を向く
    tilt.write(0);     // 上下方向も正面を向く
    delay(500);
}

void loop() {
    float maxT = 0.0;       // 最高温度
    int Tx[AMG_CELL_NUM*AMG_CELL_NUM];     // 最高温度のx座標
    int Ty[AMG_CELL_NUM*AMG_CELL_NUM];     // 最高温度のy座標
    int Tn = 0;             // 最高温度の個数
    float Cx, Cy;           // 最高温度の重心座標

    server.handleClient();

    if ((millis() - lastT) > 500) {  // 500m秒に1回処理する
        lastT = millis();

        amg8833.read(temp);  // AMG8833から温度データを取得

        for (int y = 0; y < AMG_CELL_NUM; y++) {
            for (int x = 0; x < AMG_CELL_NUM; x++) {
                float t = temp[(AMG_CELL_NUM - y - 1) * AMG_CELL_NUM + AMG_CELL_NUM - x - 1];  // 画素(x, y)の温度を取り出す
                if (maxT < t) {          // 最高温度が更新されたら
                    maxT = t;
                    Tn = 1;              // 最高温度の個数を1にして、
                    Tx[Tn - 1] = x;      // 最高温度の座標を記録
                    Ty[Tn - 1] = y;
                } else if (maxT == t) {  // 最高温度と同じ温度が見つかったら
                    Tn++;                // 最高温度の個数を1増やして、
                    Tx[Tn - 1] = x;      // 最高温度の座標を記録
                    Ty[Tn - 1] = y;
                }
            }
        }
        Cx = Cy = 0.0;
        for (int i = 0; i < Tn; i++) {
            Cx += Tx[i];
            Cy += Ty[i];
        }

        Cx = Cx / Tn - (AMG_CELL_NUM / 2.0);  // 中心座標が(0, 0)になるようにして重心座標を計算
        Cy = Cy / Tn - (AMG_CELL_NUM / 2.0);

        Serial.printf("center: (%.1f, %.1f, %.1f, %.1f)\r\n", Cx, Cy, r2d(atan(Cx / l)), r2d(atan(Cy / l)));
        if (Cx) {
            pan.move(-1 * r2d(atan(Cx / l)));
        }
        if (Cy) {
            tilt.move(r2d(atan(Cy / l)));
        }
    }
}
