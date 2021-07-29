#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <Firebase.h>
#include <FirebaseHttpClient.h>
#include <FirebaseArduino.h>
#include <ESP8266WiFi.h>
#include "DHT.h"


// 파이어 베이스 및 와이파이, 아두이노 설정값
#define FIREBASE_HOST "파이어베이스호스트"
#define FIREBASE_AUTH "파이어베이스인증코드"
#define WIFI_SSID "와이파이이름"
#define WIFI_PASSWORD "와이파이비밀번호"
#define DHTPIN 16      // DHT핀을 2번으로 정의한다(DATA핀)
#define DHTTYPE DHT11  // DHT타입을 DHT11로 정의한다
DHT DHT(DHTPIN, DHTTYPE);  // DHT설정 - dht (디지털4, dht11)

int UpperThreshold = 518;
int LowerThreshold = 490;
int reading = 0;
float BPM = 0.0;
float purse=0;                
bool IgnoreReading = false;
bool FirstPulseDetected = false;
unsigned long FirstPulseTime = 0;
unsigned long SecondPulseTime = 0;
unsigned long PulseInterval = 0;

int TXPin = 0;
int RXPin = 2;

//GPS
SoftwareSerial ss(TXPin, RXPin); //tx,rx
TinyGPS gps;// GPS 객체생성


static void smartdelay(unsigned long ms);
static void print_float(float val, float invalid, int len, int prec);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_date(TinyGPS &gps);
char FireTime[32];        // 시간넣을 문자

void setup() 
{
  Serial.begin(9600);
 
    // 와이파이 연결확인
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);       // 와이파이 id, pass 연결
    Serial.print("connecting");
    while (WiFi.status() != WL_CONNECTED)       // 비연결시 0.5초마다 .
    {
        Serial.print(".");
        delay(500);
    }
   Serial.println();
   Serial.print("connected: ");
   Serial.println(WiFi.localIP());
   Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);    // 와이파이 연결시 파이어베이스에 접속
   DHT.begin();           // 온도센서
   ss.begin(9600);        // gps센서
}


void loop() 
{

  float flat=0.0, flon=0.0;
  float pulse = random(30)+75;                                  // bpm 계산 오류로 인한 랜덤함수
  unsigned long age, date, time, chars = 0;
  unsigned short sentences = 0, failed = 0;

   gps.f_get_position(&flat, &flon, &age);
   print_float(flat, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6);      // 위도 소수점 6자리까지 표현
   print_float(flon, TinyGPS::GPS_INVALID_F_ANGLE, 11, 6);      // 경도 소수점 6자리까지 표현
   print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
   print_date(gps);       // 시간
     
  purseBPM();     // 심박수 계산 함수
  

float tempC = DHT.readTemperature();      // 섭씨 온도
float temperatureC = tempC + 11.5;

Serial.print("Temperature (C) = ");
Serial.println(temperatureC);

    if(flat != NULL && flon != NULL && flat != 1000 && flon != 1000)
    {
    Firebase.setString("time", FireTime);
    
    Firebase.setFloat("latitude", flat);    // 파이어베이스에 lattitude 항목에 실수값 위도 삽입
    
    Firebase.setFloat("longitude", flon);    // 파이어베이스에 longitude 항목에 실수값 경도 삽입
    }
    
    Firebase.setFloat("purse", purse);    // 파이어베이스에 purse 항목에 심박값 위도 삽입
    //Firebase.setFloat("purse", pulse);
    
    Firebase.setFloat("temperature", temperatureC);    // 파이어베이스에 temperature 항목에 온도값 위도 삽입

    
    
    if (Firebase.failed())
    {
      Serial.print("setting /location failed:");
      Serial.println(Firebase.error());
      return;      
    }
  smartdelay(500);

}
static void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static void print_float(float val, float invalid, int len, int prec)
{
  if (val == invalid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(' ');
  }
  smartdelay(0);
}

static void print_int(unsigned long val, unsigned long invalid, int len)
{
  char sz[32];
  if (val == invalid)
    strcpy(sz, "*******");
  else
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  Serial.print(sz);
  smartdelay(0);
}

static void print_date(TinyGPS &gps)
{
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  if (age == TinyGPS::GPS_INVALID_AGE)
    Serial.print("********** ******** ");
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d ",
        month, day, year, hour, minute, second);
        
        sprintf(FireTime, "%02d/%02d/%02d %02d:%02d:%02d ",
        month, day, year, hour, minute, second);
        
    Serial.print(sz);
  }
  print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
  Serial.println();
  smartdelay(0);
}
 
static void purseBPM(){

  reading = analogRead(A0);
  
      // Heart beat leading edge detected.
      if(reading > UpperThreshold && IgnoreReading == false){
        if(FirstPulseDetected == false){
          FirstPulseTime = millis();
          FirstPulseDetected = true;
        }
        else{
          SecondPulseTime = millis();
          PulseInterval = SecondPulseTime - FirstPulseTime;
          FirstPulseTime = SecondPulseTime;
        }
        IgnoreReading = true;
      }

      // Heart beat trailing edge detected.
      if(reading < LowerThreshold){
        IgnoreReading = false;
      }  
  
      BPM = (1.0/PulseInterval) * 60.0 * 1000;

      Serial.print(reading);
      Serial.print("\t");
      Serial.print(PulseInterval);
      Serial.print("\t");
      Serial.print(BPM);
      Serial.println(" BPM");
      Serial.flush();

    purse = round(BPM);
}




/*------------------------------------------------------------------------------------------------------------------------------------------*/
/*    아두이노 버전 1.6.9
 *    사용 보드 Arducam esp8266
 *    필요 보드매니져 https://www.arducam.com/downloads/ESP8266_UNO/package_ArduCAM_index.json
 *    필요 zip 라이브러리 https://github.com/mikalhart/TinyGPSPlus
 *                        https://github.com/FirebaseExtended/firebase-arduino
 *    필요 라이브러리 arduinoJson 5.13.2
 *
#include <ESP8266.h>
#include <Firebase.h>
#include <FirebaseArduino.h>
#include <ESP8266WiFi.h>
 *
 *
#define FIREBASE_HOST "arduino-gps-3774c.firebaseio.com"
#define FIREBASE_AUTH "aKJbvoMVeu38iKZ9PaR735PNctweKNK3zbja9zGL"
#define WIFI_SSID "Softbanan"
#define WIFI_PASSWORD "dks81466"
 *
 *
 */

      
