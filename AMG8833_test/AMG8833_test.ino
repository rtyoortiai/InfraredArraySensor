// Program Ver. IAS001

#include <Wire.h>

// レジスタアドレス(https://docid81hrs3j1.cloudfront.net/medialibrary/2017/11/PANA-S-A0002141979-1.pdf)
#define PCLT  (0x00)  // パワーコントロールレジスタ(R/W)[7:0] (初期値0x00(ノーマルモード))
#define RST   (0x01)  // リセットレジスタ(W)[7:0]
#define FPSC  (0x02)  // フレームレートレジスタ(R/W)[0:0] (0x00: 10fps, 0x01: 1fps) (初期値0x00)
#define INTC  (0x03)  // 割り込みコントロールレジスタ(R/W)[1:0]
#define STAT  (0x04)  // ステータスレジスタ(R)[2:1]
#define SCLR  (0x05)  // ステータスクリアレジスタ(W)[2:1]
#define AVE   (0x07)  // アベレージレジスタ(R/W)[5:5]
                      // 2回移動平均出力モード有効にする場合       2回移動平均出力モードを無効にする場合
                      // Address  R/W   Value                 Address   R/W   Value
                      // 0x1F     W     0x50                  0x1F      W     0x50
                      // 0x1F     W     0x45                  0x1F      W     0x45
                      // 0x1F     W     0x57                  0x1F      W     0x57
                      // 0x07     W     0x20                  0x1F      W     0x00
                      // 0x1F     W     0x00                  0x1F      W     0x00
#define T01L  (0x80)  // 低位ビット側の温度レジスタ(R)[7:0]  (符号データ含め12 bit)

#define AMG88_ADDR  (0x68) // I2Cスレーブアドレス in 7bit

// I2C通信のピンID
#define SDA_PIN_ID  (21)
#define SCL_PIN_ID  (22)

#define DEG_PER_BIT (0.25)  // 0000_0000_0001 = 0.25 deg

void write8(int id, int reg, int data) {  // idで示されるデバイスにregとdataを書く
    Wire.beginTransmission(id);  // 送信先のI2Cアドレスを指定して送信の準備をする
    Wire.write(reg);  // regをキューイングする
    Wire.write(data);  // dataをキューイングする
    uint8_t result = Wire.endTransmission();  // キューイングしたデータを送信する
}

void dataread(int id, int reg, int *data, int datasize) {
    Wire.beginTransmission(id);  // 送信先のI2Cアドレスを指定して送信の準備をする
    Wire.write(reg);  // regをキューイングする
    Wire.endTransmission();  // キューイングしたデータを送信する

    Wire.requestFrom(id, datasize);  // データを受信する先のI2Cアドレスとバイト数を指定する
    int i = 0;
    while (Wire.available() && i < datasize) {
        data[i++] = Wire.read();  // 指定したバイト数分、データを読む
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) ;

    pinMode(SDA_PIN_ID, INPUT_PULLUP);  // SDAをプルアップする
    pinMode(SCL_PIN_ID, INPUT_PULLUP);  // SCLをプルアップする
    Wire.begin();
    
    write8(AMG88_ADDR, FPSC, 0x00);   // 10fps
    write8(AMG88_ADDR, INTC, 0x00);   // INT出力無効
    write8(AMG88_ADDR, 0x1F, 0x50);   // 移動平均出力モード有効(ここから)
    write8(AMG88_ADDR, 0x1F, 0x45);
    write8(AMG88_ADDR, 0x1F, 0x57);
    write8(AMG88_ADDR, AVE,  0x20);   // 2回移動平均出力を有効に設定
    write8(AMG88_ADDR, 0x1F, 0x00);   // 移動平均出力モード有効(ここまで)
}

void loop() {
    float temp[64];  // 8 x 8 の温度データ

    int sensorData[128];
    dataread(AMG88_ADDR, T01L, sensorData, 128);  // 128バイト(= 2byte/data * 8 * 8)のピクセルデータを読む
    for (int i = 0 ; i < 64 ; i++) {
        int16_t temporaryData = (sensorData[i * 2 + 1] << 8) + sensorData[i * 2];  // 上位バイトと下位バイトから温度データを作る
        if(temporaryData > 0x200) {  // マイナスの場合
            temp[i] = (-temporaryData +  0xfff) * -DEG_PER_BIT;
        } else {                     // プラスの場合 
            temp[i] = temporaryData * DEG_PER_BIT;
        }
    }

    for (int y = 0; y < 8; y++) {  // 8 x 8の温度データをシリアルモニタに出力する
        for (int x = 0; x < 8; x++) {
            Serial.printf("%2.1f ", temp[(8 - y - 1) * 8 + 8 - x - 1]);
        }
        Serial.println();
    }
    Serial.println("--------------------------------");
    delay(5000);
}
