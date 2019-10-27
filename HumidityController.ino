/***************************************
   Name    :  Humidity Controller
   Version :  v1.0
   Make    :  2019/10/11
****************************************/

#include <stdint.h>
#include <TouchScreen.h>
#include <Adafruit_TFTLCD.h>
#include <DHT.h>

#define LCD_CS A3    // Chip Select goes to Analog 3
#define LCD_CD A2    // Command/Data goes to Analog 2
#define LCD_WR A1    // LCD Write goes to Analog 1
#define LCD_RD A0    // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 8   // can be a digital pin
#define XP 9   // can be a digital pin

#define TS_MINX 122 // X Min Value (Raw)
#define TS_MAXX 948 // X Max Value (Raw)
#define TS_MINY 122 // Y Min Value (Raw)
#define TS_MAXY 908 // Y Max Value (Raw)

//カラーマップ
#define BLACK         0x0000
#define BLUE          0x001F
#define LIGHTBLUE     0xaebc
#define GREY          0xCE79
#define LIGHTGREY     0xDEDB
#define RED           0xF800
#define GREEN         0x07E0
#define LIGHTGREEN    0x8f71
#define CYAN          0x07FF
#define MAGENTA       0xF81F
#define YELLOW        0xFFE0
#define LIGHTYELLOW   0xfffb
#define WHITE         0xFFFF

const int PIN_DHT = 10;
const int PIN_RELAY = 11;
DHT dht( PIN_DHT, DHT11 );

int8_t arryHumy[] = {0,0};
int8_t arryTemp[] = {0,0};
int8_t arrySetting[] = {0,0};
int settingValue = 35;
int titleverMode = 1;
int gvPos = 30 + 5;  //注)30の部分はgPos_xと同じ値を入れる
int tft_X = 0;
int tft_Y = 0;
int tft_Z = 0;

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET); 
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

void setup() {

  //シリアル通信設定
  Serial.begin(9600);

  //電磁弁制御ピンの設定
  pinMode(PIN_RELAY,OUTPUT);
  
  //TFTライブラリの初期化
  tft.reset();
  uint16_t identifier = tft.readID();
  tft.begin(identifier);

  //DHTライブラリの初期化
  dht.begin();

  //画面描画
  createWindow();

}

void loop() {

  static bool isFahrenheit = true;
  static int percentHumidity = 0;
  static int Temperature = 0;
  static int timeCounter = 100;

  //DHTセンサーを1s間隔で取得するための処理[15]
  if (timeCounter >= 15){
    percentHumidity = dht.readHumidity();
    Temperature = (dht.readTemperature(isFahrenheit)-32)/1.8;
    timeCounter = 0;

    //グラフの更新
    updataGraph(percentHumidity);

    //値の取得タイミング表示用ランプ
    tft.fillCircle(15,85,7,BLUE);

  }else{

    timeCounter += 1;

    //値の取得タイミング表示用ランプ
    if (timeCounter >= 4){
      tft.fillCircle(15,85,7,WHITE);
    }

  }

  //タッチ動作を取得
  retrieveTouch();

  //タッチパネル操作
  if (tft_Z > ts.pressureThreshhold) {

    if(tft_Y >= 26 && tft_Y <= 37){
      if(tft_X >= 28 && tft_X <= 41){
        //SETTINGの「＋」をタッチした
        if(settingValue < 95){
          settingValue +=5;
          //値をリセット
          tft.fillRect(35,110,320,220,BLACK);
          gvPos = 35;
          //グラフ生成
          createGraph();
          //設定ラインを引く
          createSettingLine();
        }
      }
      if(tft_X >= 9 && tft_X <= 20){
        //SETTINGの「－」をタッチした
        if(settingValue > 5){
          settingValue -=5;
          //値をリセット
          tft.fillRect(35,110,320,220,BLACK);
          gvPos = 35;
          //グラフ生成
          createGraph();
          //設定ラインを引く
          createSettingLine();
        }
      }
    }
  }

  //配列へ置き換え
  valSplit(percentHumidity,arryHumy);
  valSplit(Temperature,arryTemp);
  valSplit(settingValue,arrySetting);

  //値の更新
  insertValue();
  
  //電磁弁の解閉判断
  if (percentHumidity > settingValue){
    createTitlever(1);
    //digitalWrite(PIN_RELAY,HIGH);
  }else{
    createTitlever(0);
    //digitalWrite(PIN_RELAY,LOW);
  }

  delay(5);
}

int adjust_x(int x) {
  int TFT_MAX = tft.width() - 1;
  return constrain(map(x, TS_MINX, TS_MAXX, 0, TFT_MAX), 0, TFT_MAX);

}

int adjust_y(int y) {
  int TFT_MAX = tft.height() - 1;
  return constrain(map(y, TS_MINY, TS_MAXY, 0, TFT_MAX), 0, TFT_MAX);

}

void valSplit(int a ,int8_t *arybuf){
  int buf = 0;
  arybuf[0] = a / 10;
  buf = a % 10;
  arybuf[1] = buf / 1;
}

void createTitlever(int tMode) {

  if(titleverMode != tMode){

    switch (tMode){
      case 0: //ノーマル
        tft.fillRect(0, 0, TS_MAXX, 20, CYAN);
        titleverMode = 0;
        break;
      case 1: //リレーON
        tft.fillRect(0, 0, TS_MAXX, 20, GREEN);
        titleverMode = 1;
        break;
    }

    tft.setCursor(2, 2);
    tft.setTextColor(BLACK);
    tft.setTextSize(2);
    tft.println("HUMIDITY CONTROLLER V1.0");
  }

}

void createWindow() {

  //縦横表示設定
  tft.setRotation(3);
  tft.fillScreen(BLACK);

  //タイトル
  createTitlever(0);

  //現在値・設定値表示Window
  tft.fillRoundRect(5, 25, 152, 70, 3, WHITE);
  tft.fillRoundRect(162, 25, 152, 70, 3, LIGHTYELLOW);
  
  //現在値タイトル
  tft.setCursor(6,26);
  tft.setTextColor(BLACK);
  tft.setTextSize(1);
  tft.println("CURRENT");

  //設定値タイトル
  tft.setCursor(163,26);
  tft.println("SETTING");

  //単位
  tft.setCursor(135,70);
  tft.setTextColor(BLACK);
  tft.setTextSize(3);
  tft.println("%");
  tft.setCursor(290,70);
  tft.println("%");
  tft.setCursor(105,27);
  tft.setTextSize(1);
  tft.println("TEMP:");

  //設定変更用ボタン
  tft.fillRoundRect(275, 26, 18, 18, 3,MAGENTA);
  tft.fillRoundRect(295, 26, 18, 18, 3,MAGENTA);
  tft.setTextSize(2);
  tft.setCursor(279,27); tft.println("+");
  tft.setCursor(299,27); tft.println("-");
  
  //グラフ生成
  createGraph();

  //設定ラインを引く
  createSettingLine();

}

//タッチ動作の取得
void retrieveTouch(){

  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);

  //共有PINのポート設定
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  // on my tft the numbers are reversed so this is used instead of the above
  tft_X = tft.width() - map(p.x, TS_MAXX, TS_MINX, 0, tft.width());
  tft_Y = map(p.y, TS_MAXY, TS_MINY, 0, tft.height());
  tft_Z = p.z;

}

void createSettingLine(){
  //設定ライン
  tft.drawFastHLine(35,120+(100-settingValue),280,BLUE);
  tft.setTextSize(1);
  tft.setTextColor(BLUE);
  tft.setCursor(220,120+(100-settingValue)-10);
  tft.println("<SETTING LINE>");
}

void updataGraph(int humy){
  
  //グラフ値へ置き換え 注)120の部分はgPos_yと同じ値を入れる
  humy = 120 + (100 - humy);

  //グラフ横軸の加算処理
  if (gvPos >= 310){
    //右端までいったら初期化 , 30はvPos_xと同じ値を入れる
    gvPos = 30 + 5;
    //値をリセット
    tft.fillRect(35,120,320,220,BLACK);
    //設定ラインを引く
    createSettingLine();
  }else{
    gvPos += 1;
  }

  //湿度のグラフ化
  tft.drawPixel(gvPos,humy,RED);

}

void createGraph() {
  //グラフ作成位置用変数
  static int gPos_y = 120;
  static int gPos_x = 30;

  //グラフ(タイトル)
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.setCursor(gPos_x-25,gPos_y-18);
  tft.println("<HUMIDITY GRAPH>");
  //グラフ(縦軸)
  tft.drawLine(gPos_x,gPos_y,gPos_x,gPos_y + 100,WHITE);
  //グラフ(目盛)
  tft.drawLine(gPos_x,gPos_y,gPos_x+5,gPos_y,WHITE);             //100% y=120
  tft.drawLine(gPos_x,gPos_y+50,gPos_x+5,gPos_y+50,WHITE);       // 50%
  tft.drawLine(gPos_x,gPos_y+100,gPos_x+5,gPos_y+100,WHITE);     //  0% y=220
  //グラフ(目盛‐数値)
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.setCursor(gPos_x-25,gPos_y-3); tft.println("100%");
  tft.setCursor(gPos_x-25,gPos_y+47); tft.println(" 50%");
  tft.setCursor(gPos_x-25,gPos_y+97); tft.println("  0%");
}

void insertValue() {

  //アスキー変換
  arryHumy[0] += 48;
  arryHumy[1] += 48;
  arryTemp[0] += 48;
  arryTemp[1] += 48;
  arrySetting[0] += 48;
  arrySetting[1] += 48;
  
  //画面の数字更新
  tft.drawChar(50,50,arryHumy[0],RED,WHITE,5);
  tft.drawChar(80,50,arryHumy[1],RED,WHITE,5);
  tft.drawChar(135,27,arryTemp[0],BLACK,WHITE,1);
  tft.drawChar(141,27,arryTemp[1],BLACK,WHITE,1);
  tft.drawChar(205,50,arrySetting[0],RED,LIGHTYELLOW,5);
  tft.drawChar(235,50,arrySetting[1],RED,LIGHTYELLOW,5);

}