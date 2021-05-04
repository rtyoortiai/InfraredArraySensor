// AMG8833.h
// 赤外線アレイセンサモジュール関連の定義ヘッダ

#ifndef _AMG8833_H_
#define _AMG8833_H_

#include <Arduino.h>

// レジスタアドレス(https://docid81hrs3j1.cloudfront.net/medialibrary/2017/11/PANA-S-A0002141979-1.pdf)
#define PCLT    (0x00)      // パワーコントロールレジスタ(R/W)[7:0] (初期値0x00(ノーマルモード))
#define RST     (0x01)      // リセットレジスタ(W)[7:0]
#define FPSC    (0x02)      // フレームレートレジスタ(R/W)[0:0] (0x00: 10fps, 0x01: 1fps) (初期値0x00)
#define INTC    (0x03)      // 割り込みコントロールレジスタ(R/W)[1:0]
#define STAT    (0x04)      // ステータスレジスタ(R)[2:1]
#define SCLR    (0x05)      // ステータスクリアレジスタ(W)[2:1]
#define AVE     (0x07)      // アベレージレジスタ(R/W)[5:5]
                            // 2回移動平均出力モード有効にする場合       2回移動平均出力モードを無効にする場合
                            // Address  R/W   Value                 Address   R/W   Value
                            // 0x1F     W     0x50                  0x1F      W     0x50
                            // 0x1F     W     0x45                  0x1F      W     0x45
                            // 0x1F     W     0x57                  0x1F      W     0x57
                            // 0x07     W     0x20                  0x1F      W     0x00
                            // 0x1F     W     0x00                  0x1F      W     0x00
#define T01L  (0x80)        // 低位ビット側の温度レジスタ(R)[7:0]  (符号データ含め12 bit)

#define AMG88_ADDR  (0x68)  // I2Cスレーブアドレス in 7bit

#define DEG_PER_BIT (0.25)  // 0000_0000_0001 = 0.25 deg

class AMG8833 {             // AMG8833クラス
public:
    AMG8833(uint8_t addr);
    virtual ~AMG8833();

    void begin(void);
    void read(float *temp);
private:
    uint8_t _addr;             // AMG8833のスレーブアドレス
    void _write8(int id, int reg, int data);
    void _dataread(int id, int reg, int *data, int datasize);
};

#endif      // _AMG8833_H_
