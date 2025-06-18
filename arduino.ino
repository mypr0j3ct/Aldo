#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <LiquidCrystal_I2C.h>
#include <FIRFilter.h>
#include <IIRFilter.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define FORMAT_LITTLEFS_IF_FAILED true
#define API_KEY "AIzaSyCqVC1VxSA0q3crk_dX-tvqWWjCXDPm_TA"
#define DATABASE_URL "https://aldo-e1e88-default-rtdb.asia-southeast1.firebasedatabase.app/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
MAX30105 particleSensor;
LiquidCrystal_I2C lcd(0x27, 16, 2);
AsyncWebServer server(80);
IPAddress localIP;
IPAddress localGateway;
IPAddress subnet(255, 255, 0, 0);
WiFiUDP ntpUDP;

const char* var2Path = "/data/value.txt";
String agePath = "/Age";
String bloodPath = "/Blood_Pressure";
String heartPath = "/Max_HR";
String predPath = "/Prediction";
String slopePath = "/ST_Slope";
String sexPath = "/Sex";
String idPath = "/idmicro";
const long intervalWifi = 10000;
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";
String waktu = "";
String ssid;
String pass;
String ip;
String gateway;
unsigned long startTime;
unsigned long previousMillis = 0;
unsigned long interval;
bool initialized = false;

//ValueID, ValueAge, ValueBP, ValueHR, ValueSex, ValueST, ValuePred
const int buttonUp = 32;
const int buttonDown = 33;
const int buttonRight = 25;
const int buttonLeft = 26;

int idRatusan = 0;
int idPuluhan = 0;
int idSatuan = 0;
bool editIdRatusan = true;
bool editIdPuluhan = false;
bool editIdSatuan = false;
int ValueID = 0;
int usiaPuluhan = 0;
int usiaSatuan = 0;
bool editUsiaPuluhan = true;
bool editUsiaSatuan = false;
int ValueAge = 0;
bool isMale = true;
String ValueSex = "M";
const unsigned long updateInterval = 1000; 
int motor = 15;
int solenoid = 2;
int mark = 0;
int ValueBP = 0;
const double ECG_samplefreq = 360;
const unsigned long ECG_interval = round(1e6 / ECG_samplefreq);
const uint8_t ECG_pin = 39;
const int16_t DC_offset = 511;
const double b_notch[] = {0.98963618, -1.60126497, 0.98963618};
const double a_notch[] = {1, -1.60126497, 0.97927235};
const double b_lp[] = {
  1.02703880e-05, 7.18927160e-05, 2.15678148e-04, 3.59463580e-04,
  3.59463580e-04, 2.15678148e-04, 7.18927160e-05, 1.02703880e-05
};
const double a_lp[] = {
  1.0, -5.0256015, 11.03572646, -13.68299448, 10.32280236, -4.73071438,
  1.21779242, -0.13569627
};
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);
const double b_hp[] = {0.99630444, -1.99260889, 0.99630444};
const double a_hp[] = {1.0, -1.99259523, 0.99262254};
FIRFilter notch(b_notch);
IIRFilter lp(b_lp, a_lp);
IIRFilter hp(b_hp, a_hp);
struct ECGSample {
  unsigned long timestamp;
  int value;
};
const int maxBufferSize = 400;
ECGSample ecgBuffer[maxBufferSize];
int bufferIndex = 0;
bool bufferActive = false;
bool r1Detected = false;
bool r2Detected = false;
unsigned long millisR1 = 0, millisR2 = 0;
int valueR1 = 0, valueR2 = 0;
int valueS1 = 0, valueS2 = 0;
const int rThreshold = 650;
const int dropThreshold = 300;
const int rsWindow = 150;
const int maxRRInterval = 1200;
char temporarySTs[7][6];
int temporaryIndex = 0;
String ValueST = "Unknown";
const unsigned long bpmMeasurementTime = 30000;
String ValueConvSex, ValueConvST, ValueConvPred, ValueConvID;
int ValueHR = 0;
int avg = 0;
String ValuePred;

// ===================================================================================
// == KODE HTML & CSS UNTUK WI-FI MANAGER DITANAMKAN DI SINI (EMBEDDED)
// ===================================================================================
const char wifiManager_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP Wi-Fi Manager</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }
    
    html { 
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
    }
    
    body { 
      margin: 0;
      padding: 20px;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
    }
    
    .topnav { 
      background: linear-gradient(135deg, #1abc9c 0%, #16a085 100%);
      padding: 30px 20px;
      border-radius: 20px 20px 0 0;
      box-shadow: 0 8px 32px rgba(0,0,0,0.1);
      position: relative;
      overflow: hidden;
    }
    
    .topnav::before {
      content: '';
      position: absolute;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background: url('data:image/svg+xml,<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100"><defs><pattern id="wifi" patternUnits="userSpaceOnUse" width="20" height="20"><path d="M10 2c4.4 0 8 3.6 8 8s-3.6 8-8 8-8-3.6-8-8 3.6-8 8-8zm0 2c-3.3 0-6 2.7-6 6s2.7 6 6 6 6-2.7 6-6-2.7-6-6-6z" fill="rgba(255,255,255,0.1)"/></pattern></defs><rect width="100" height="100" fill="url(%23wifi)"/></svg>') repeat;
      opacity: 0.1;
    }
    
    h1 { 
      color: white;
      font-size: 2.5rem;
      font-weight: 700;
      text-shadow: 0 2px 4px rgba(0,0,0,0.2);
      position: relative;
      z-index: 1;
    }
    
    .content { 
      width: 100%;
      max-width: 500px;
    }
    
    .card-grid { 
      display: grid;
      grid-template-columns: 1fr;
      gap: 0;
    }
    
    .card { 
      background: rgba(255, 255, 255, 0.95);
      backdrop-filter: blur(10px);
      border-radius: 0 0 20px 20px;
      padding: 40px;
      box-shadow: 0 20px 40px rgba(0,0,0,0.1);
      border: 1px solid rgba(255,255,255,0.2);
    }
    
    form {
      width: 100%;
    }
    
    label { 
      font-weight: 600;
      color: #2c3e50;
      display: block;
      margin-bottom: 8px;
      font-size: 1.1rem;
      position: relative;
    }
    
    label::before {
      content: '';
      position: absolute;
      left: -25px;
      top: 50%;
      transform: translateY(-50%);
      width: 4px;
      height: 20px;
      background: linear-gradient(135deg, #1abc9c, #16a085);
      border-radius: 2px;
    }
    
    input[type=text], input[type=password] { 
      width: 100%;
      padding: 16px 20px;
      margin: 8px 0 24px 0;
      border: 2px solid #e0e6ed;
      border-radius: 12px;
      font-size: 1rem;
      transition: all 0.3s ease;
      background: rgba(255,255,255,0.8);
      backdrop-filter: blur(5px);
    }
    
    input[type=text]:focus, input[type=password]:focus {
      outline: none;
      border-color: #1abc9c;
      box-shadow: 0 0 0 3px rgba(26, 188, 156, 0.1);
      transform: translateY(-2px);
      background: white;
    }
    
    input[type=text]::placeholder, input[type=password]::placeholder {
      color: #95a5a6;
      font-style: italic;
    }
    
    input[type=submit] { 
      background: linear-gradient(135deg, #1abc9c 0%, #16a085 100%);
      color: white;
      padding: 18px 20px;
      margin: 20px 0 0 0;
      border: none;
      border-radius: 12px;
      cursor: pointer;
      width: 100%;
      font-size: 1.2rem;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 1px;
      transition: all 0.3s ease;
      box-shadow: 0 8px 25px rgba(26, 188, 156, 0.3);
      position: relative;
      overflow: hidden;
    }
    
    input[type=submit]:hover { 
      background: linear-gradient(135deg, #16a085 0%, #1abc9c 100%);
      transform: translateY(-3px);
      box-shadow: 0 12px 35px rgba(26, 188, 156, 0.4);
    }
    
    input[type=submit]:active {
      transform: translateY(-1px);
      box-shadow: 0 6px 20px rgba(26, 188, 156, 0.3);
    }
    
    input[type=submit]::before {
      content: '🌐';
      margin-right: 10px;
      font-size: 1.1rem;
    }
    
    @media (max-width: 600px) {
      body {
        padding: 10px;
      }
      .card {
        padding: 30px 20px;
      }
      h1 {
        font-size: 2rem;
      }
      .topnav {
        padding: 20px 15px;
      }
    }
    
    @keyframes fadeInUp {
      from { opacity: 0; transform: translateY(30px); }
      to { opacity: 1; transform: translateY(0); }
    }
    .card { animation: fadeInUp 0.6s ease-out; }
    .topnav { animation: fadeInUp 0.4s ease-out; }
    
    .card-grid { animation: float 6s ease-in-out infinite; }
    @keyframes float {
      0%, 100% { transform: translateY(0px); }
      50% { transform: translateY(-5px); }
    }
  </style>
</head>
<body>
  <div class="topnav">
    <h1>ESP Wi-Fi Manager</h1>
  </div>
  <div class="content">
    <div class="card-grid">
      <div class="card">
        <form action="/" method="POST">
          <p>
            <label for="ssid">SSID</label>
            <input type="text" id="ssid" name="ssid"><br>
            <label for="pass">Password</label>
            <input type="password" id="pass" name="pass"><br>
            <input type="submit" value="Ubah Wifi">
          </p>
        </form>
      </div>
    </div>
  </div>
</body>
</html>
)rawliteral";
// ===================================================================================

float probYes = 0.5533769;
float probNo = 0.4466231;

void sex();
void usia();
void bpSensor();
void melihat();
void initWiFi();
void idUser();
void STslope();
void heartRate();
void hrSensor();
void NaiveBayes();
void processECG();
void resetDetection();
void welcomeScreen();
void bloodPressure();
void updateSexDisplay();
void updateUsiaDisplay();
void processSTSegment();
void konfirmasiUlang();
void initLittleFS();
void setupFirebase();
void updateIdDisplay();
void startWiFiSetup();
void printSavedData();
String getTimestamp();
void menuWiFiAtauLanjut();
void sendDataToFirebase();
void menu1WiFiAtauLanjut();
bool isButtonPressed(int pin);
void menuKirimFirebaseAtauLanjut();
void createDir(fs::FS &fs, const char *path);
String readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendToFile(fs::FS &fs, const char *path, const char *message);
void deleteFile(fs::FS &fs, const char *path);
float getProbabilityAge(int age, String condition);
float getProbabilitySex(String sex, String condition);
float getProbabilityRestingBP(int bp, String condition);
float getProbabilityMaxHR(int hr, String condition);
float getProbabilitySTSlope(String slope, String condition);
void konversi(String ValueSex, String ValueST, String ValuePred);
void saveSelectedDataToJson(String ValueID, int ValueAge, int ValueBP, int ValueHR, String ValueSex, String ValueST, String ValuePred, String waktu);

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  pinMode(buttonUp, INPUT_PULLUP);
  pinMode(buttonDown, INPUT_PULLUP);
  pinMode(buttonRight, INPUT_PULLUP);
  pinMode(buttonLeft, INPUT_PULLUP);
  pinMode(motor, OUTPUT);
  pinMode(solenoid, OUTPUT);
  digitalWrite(motor, LOW);
  digitalWrite(solenoid, LOW);
  initLittleFS();
  createDir(LittleFS, "/data");
  welcomeScreen();
  menuWiFiAtauLanjut();
  startTime = millis();
  //deleteFile(LittleFS, var2Path);
}

void loop() {}

bool isButtonPressed(int pin) {
  static unsigned long lastPressTime = 0;
  const unsigned long debounceDelay = 300;
  if (digitalRead(pin) == LOW) {
    if (millis() - lastPressTime > debounceDelay) {
      lastPressTime = millis();
      while (digitalRead(pin) == LOW)
        ;
      return true;
    }
  }
  return false;
}

void welcomeScreen() {
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("WELCOME");
  delay(1000);
  lcd.clear();
}

void idUser() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ID Pengguna:");
  updateIdDisplay();
  while (true) {
    if (isButtonPressed(buttonUp)) {
      if (editIdRatusan) {
        idRatusan = (idRatusan + 1) % 10;
      } else if (editIdPuluhan) {
        idPuluhan = (idPuluhan + 1) % 10;
      } else {
        idSatuan = (idSatuan + 1) % 10;
      }
      updateIdDisplay();
    }
    if (isButtonPressed(buttonDown)) {
      if (editIdRatusan) {
        idRatusan = (idRatusan + 9) % 10;
      } else if (editIdPuluhan) {
        idPuluhan = (idPuluhan + 9) % 10;
      } else {
        idSatuan = (idSatuan + 9) % 10;
      }
      updateIdDisplay();
    }
    if (isButtonPressed(buttonLeft)) {
      if (editIdSatuan) {
        editIdSatuan = false;
        editIdPuluhan = true;
      } else if (editIdPuluhan) {
        editIdPuluhan = false;
        editIdRatusan = true;
      }
    }
    if (isButtonPressed(buttonRight)) {
      if (editIdRatusan) {
        editIdRatusan = false;
        editIdPuluhan = true;
      } else if (editIdPuluhan) {
        editIdPuluhan = false;
        editIdSatuan = true;
      } else {
        ValueID = idRatusan * 100 + idPuluhan * 10 + idSatuan;
        usia();
        break;
      }
    }
  }
}

void updateIdDisplay() {
  lcd.setCursor(0, 1);
  lcd.print(idRatusan);
  lcd.print(idPuluhan);
  lcd.print(idSatuan);
}

void usia() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Usia :");
  updateUsiaDisplay();
  while (true) {
    if (isButtonPressed(buttonUp)) {
      if (editUsiaPuluhan) {
        usiaPuluhan = (usiaPuluhan + 1) % 10;
      } else {
        usiaSatuan = (usiaSatuan + 1) % 10;
      }
      updateUsiaDisplay();
    }
    if (isButtonPressed(buttonDown)) {
      if (editUsiaPuluhan) {
        usiaPuluhan = (usiaPuluhan + 9) % 10;
      } else {
        usiaSatuan = (usiaSatuan + 9) % 10;
      }
      updateUsiaDisplay();
    }
    if (isButtonPressed(buttonRight)) {
      if (editUsiaPuluhan) {
        editUsiaPuluhan = false;
        editUsiaSatuan = true;
      } else {
        ValueAge = usiaPuluhan * 10 + usiaSatuan;
        sex();
        break;
      }
    }
    if (isButtonPressed(buttonLeft)) {
      if (editUsiaSatuan) {
        editUsiaSatuan = false;
        editUsiaPuluhan = true;
      } else {
        idUser();
        break;
      }
    }
  }
}

void updateUsiaDisplay() {
  lcd.setCursor(0, 1);
  lcd.print(usiaPuluhan);
  lcd.print(usiaSatuan);
}

void sex() {
  waktu = getTimestamp();
  delay(1000);
  Serial.print("Waktu dari NTP (RTC online): ");
  Serial.println(waktu);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi dimatikan!");
  lcd.clear();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Jenis Kelamin:");
  updateSexDisplay();
  while (true) {
    if (isButtonPressed(buttonUp) || isButtonPressed(buttonDown)) {
      isMale = !isMale;
      updateSexDisplay();
    }
    if (isButtonPressed(buttonRight)) {
      ValueSex = isMale ? "M" : "F";
      bloodPressure();
      break;
    }
    if (isButtonPressed(buttonLeft)) {
      usia();
      break;
    }
  }
}

void updateSexDisplay() {
  lcd.setCursor(0, 1);
  lcd.print(isMale ? "Pria  " : "Wanita");
}

void bloodPressure() {
  lcd.clear();
  while (true) {
    lcd.setCursor(0, 0);
    lcd.print("Tekanan Darah");
    if (mark == 2) {
      lcd.setCursor(0, 1);
      lcd.print("Hasil: ");
      lcd.print(ValueBP);
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Gunakan manset");
    }
    if (isButtonPressed(buttonUp) || isButtonPressed(buttonDown)) {
      bpSensor();
    }
    if (isButtonPressed(buttonRight)) {
      STslope();
      break;
    }
    if (isButtonPressed(buttonLeft)) {
      sex();
      break;
    }
  }
}

void bpSensor() {
  mark = 0;
  float mmhg = 0;
  float mmhgx = 0;
  float mmhgPrev = 0;
  bool hasRisen = false;
  float sistole = 0;
  int sistolex = 0;
  unsigned long lastLcdUpdateBP = millis();
  int dataadc = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tekanan Darah");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(1000);
  lcd.clear();
  digitalWrite(motor, HIGH);
  digitalWrite(solenoid, HIGH);
  while (true) {
    dataadc = analogRead(36);
    mmhg = (dataadc - 135) / 13.85;

    if (mark == 1) {
      if (mmhg >= mmhgPrev + 2) {
        hasRisen = true;
      }
      if ((mmhgPrev - mmhg) >= 2 && !hasRisen) {
        mmhgPrev = mmhg;
        continue;
      }
    }
    if (millis() - lastLcdUpdateBP >= updateInterval) {
      lcd.setCursor(0, 0);
      lcd.print("Mengukur...    ");
      lcd.setCursor(0, 1);
      lcd.print("mmhg= ");
      lcd.print(mmhg);
      lcd.print("    ");
      lastLcdUpdateBP = millis();
    }
    if ((mmhg >= mmhgx + 3) && (mmhg > 90) && (mark == 1)) {
      sistole = mmhg;
      mark = 2;
    }
    if ((mmhg <= 75) && (mark == 1)) {
      mark = 3;
      digitalWrite(solenoid, LOW);
      lcd.setCursor(0, 0);
      lcd.print("Tekanan Darah   ");
      lcd.setCursor(0, 1);
      lcd.print("Gagal deteksi   ");
      delay(1500);
      bloodPressure();
      break;
    }
    if (mmhg >= 160) {
      digitalWrite(motor, LOW);
      delay(500);
      mark = 1;
    }
    mmhgx = mmhg;
    mmhgPrev = mmhg;
    if ((mark == 2) && (mmhg < 80)) {
      Serial.println("Pengukuran Selesai");
      sistolex = sistole;
      ValueBP = sistolex;
      digitalWrite(solenoid, LOW);
      bloodPressure();
    }
  }
}

void STslope() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Slope ST Segment");
  unsigned long startTime = 0;
  bool running = false;
  static unsigned long lastMicros = 0;
  unsigned long lastLCDUpdate = 0;
  const unsigned long lcdInterval = 200;
  bool lcdUpdatedDuringRun = false;
  while (true) {
    if (millis() - lastLCDUpdate >= lcdInterval) {
      lastLCDUpdate = millis();
      lcd.setCursor(0, 1);
      if (running && ValueST == "Unknown" && !lcdUpdatedDuringRun) {
        lcd.print("Mengukur...     ");
        lcdUpdatedDuringRun = true;
      } else if (!running && ValueST == "Unknown") {
        lcd.print("Tempel Elektroda");
        lcdUpdatedDuringRun = false;
      } else if (ValueST != "Unknown") {
        lcd.print("ST Slope: ");
        lcd.print(ValueST);
        lcd.print("     ");
        lcdUpdatedDuringRun = false;
      }
    }
    if (!running && (isButtonPressed(buttonUp) || isButtonPressed(buttonDown))) {
      ValueST = "Unknown";
      temporaryIndex = 0;
      for (int i = 0; i < 7; i++) {
        strcpy(temporarySTs[i], "");
      }
      startTime = millis();
      running = true;
      lastMicros = 0;
      lcd.setCursor(0, 1);
      lcd.print("Mengukur...     ");
      delay(2000);
    }
    if (!running && isButtonPressed(buttonRight)) {
      heartRate();
      break;
    }
    if (!running && isButtonPressed(buttonLeft)) {
      bloodPressure();
      break;
    }
    if (running) {
      if (micros() - lastMicros >= ECG_interval) {
        lastMicros += ECG_interval;
        processECG();
      }
      if (ValueST != "Unknown") {
        Serial.print("ST Segment Slope: ");
        Serial.println(ValueST);
        running = false;
        resetDetection();
      }
      if (millis() - startTime > 30000) {
        Serial.println("Gagal menemukan ST Slope dalam 30 detik");
        running = false;
        resetDetection();
      }
    }
  }
}

void processECG() {
  int16_t raw = analogRead(ECG_pin);
  double filtered = notch.filter(lp.filter(hp.filter(raw - DC_offset)));
  int value = round(filtered + DC_offset);
  unsigned long now = millis();
  static int lastValue = 0;
  static bool lookingForR = true;
  static bool afterR = false;
  static unsigned long rCandidateTime = 0;
  static int rCandidateValue = 0;
  static int minVal = 1024;
  static unsigned long minTime = 0;
  static unsigned long r1StartTime = 0;
  if (bufferActive && bufferIndex < maxBufferSize) {
    ecgBuffer[bufferIndex++] = {now, value};
  }
  if (lookingForR && value > rThreshold && value > lastValue) {
    rCandidateTime = now;
    rCandidateValue = value;
    afterR = true;
    minVal = 1024;
    minTime = 0;
  }
  if (afterR && now - rCandidateTime <= rsWindow &&
      value < rCandidateValue - dropThreshold) {
    if (value < minVal) {
      minVal = value;
      minTime = now;
    }
    if (value > minVal) {
      afterR = false;
      if (!r1Detected) {
        r1Detected = true;
        millisR1 = rCandidateTime;
        valueR1 = rCandidateValue;
        valueS1 = minVal;
        r1StartTime = now;
        bufferIndex = 0;
        bufferActive = true;
        Serial.println("R1 & S1 VALID → Mulai buffer");
      } else if (!r2Detected) {
        millisR2 = rCandidateTime;
        valueR2 = rCandidateValue;
        valueS2 = minVal;
        int deltaR = abs(valueR2 - valueR1);
        if (deltaR > 10) {
          Serial.println("R2 amplitudo tidak cocok → Ganti jadi R1 baru");
          millisR1 = millisR2;
          valueR1 = valueR2;
          valueS1 = valueS2;
          r1StartTime = now;
          bufferIndex = 0;
          bufferActive = true;
          r2Detected = false;
        } else {
          r2Detected = true;
          Serial.println("R2 & S2 VALID → Proses ST Segment");
          processSTSegment();
        }
      }
    }
  }
  if (r1Detected && !r2Detected && (now - r1StartTime > maxRRInterval)) {
    Serial.println("Timeout R2 → Reset ke R1 baru");
    resetDetection();
  }
  lastValue = value;
}

void processSTSegment() {
  unsigned long rrInterval = millisR2 - millisR1;
  unsigned long startSTms = millisR1 + rrInterval * 0.0636;
  unsigned long stopSTms = millisR1 + rrInterval * 0.1909;
  int stStartVal = -1, stStopVal = -1;
  for (int i = 0; i < bufferIndex; i++) {
    if (abs((long)(ecgBuffer[i].timestamp - startSTms)) <= 2 &&
        stStartVal == -1) {
      stStartVal = ecgBuffer[i].value;
    }
    if (abs((long)(ecgBuffer[i].timestamp - stopSTms)) <= 2 &&
        stStopVal == -1) {
      stStopVal = ecgBuffer[i].value;
    }
    if (stStartVal != -1 && stStopVal != -1) break;
  }
  Serial.print("RR Interval: ");
  Serial.print(rrInterval);
  Serial.println(" ms");
  Serial.print("ST Start Val: ");
  Serial.println(stStartVal);
  Serial.print("ST Stop Val: ");
  Serial.println(stStopVal);
  if (stStartVal == -1 || stStopVal == -1) {
    Serial.println("Gagal mendapatkan nilai ST segment");
    resetDetection();
    return;
  }
  int deltaSt = stStopVal - stStartVal;
  const char *currentST;
  if (deltaSt >= 20){
    currentST = "Up";}
  else if (deltaSt <= -20){
    currentST = "Down";}
  else{
    currentST = "Flat";}
  Serial.print("ST Classification: ");
  Serial.println(currentST);
  if (temporaryIndex < 7) {
    strcpy(temporarySTs[temporaryIndex++], currentST);
  }
  int countUp = 0, countDown = 0, countFlat = 0;
  for (int i = 0; i < temporaryIndex; i++) {
    if (strcmp(temporarySTs[i], "Up") == 0) countUp++;
    else if (strcmp(temporarySTs[i], "Down") == 0) countDown++;
    else if (strcmp(temporarySTs[i], "Flat") == 0) countFlat++;
  }
  if (countUp >= 3){
    ValueST = "Up";}
  else if (countDown >= 3){
    ValueST = "Down";}
  else if (countFlat >= 3){
    ValueST = "Flat";}
  else if (temporaryIndex >= 7){
    ValueST = "Unknown";}
  if ((ValueST, "Unknown") != 0) {
    resetDetection();
  } else if (temporaryIndex >= 7) {
    Serial.println("Tidak ada mayoritas, ulang pengukuran");
    ValueST = "Unknown";
    temporaryIndex = 0;
    for (int i = 0; i < 7; i++) strcpy(temporarySTs[i], "");
    resetDetection();
  }
}

void resetDetection() {
  r1Detected = false;
  r2Detected = false;
  bufferIndex = 0;
  bufferActive = false;
}

void heartRate() {
  while (true) {
    lcd.setCursor(0, 0);
    lcd.print("Detak per Menit");
    if (ValueHR == 0) {
      lcd.setCursor(0, 1);
      lcd.print("Letakkan jari");
    } else {
      lcd.setCursor(0, 1);
      lcd.print("BPM: ");
      lcd.print(avg);
      lcd.print(" ");
      lcd.print("MHR: ");
      lcd.print(ValueHR);
    }
    if (isButtonPressed(buttonUp) || isButtonPressed(buttonDown)) {
      hrSensor();
    }
    if (isButtonPressed(buttonRight)) {
      konfirmasiUlang();
      break;
    }
    if (isButtonPressed(buttonLeft)) {
      STslope();
      break;
    }
  }
}

void hrSensor() {
  long lastBeat = 0;
  float beatsPerMinute = 0;
  unsigned long lastUpdateTime = 0;
  unsigned long activeMeasurementTime = 0;
  unsigned long lastDetectionMillis = 0;
  bool fingerJustDetected = false;
  unsigned long fingerDetectedAt = 0;
  unsigned int bpmSum = 0;
  unsigned int bpmCount = 0;
  avg = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Detak per Menit");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(1000);
  lcd.clear();
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 was not found. Please check wiring/power.");
    while (1);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);
  lastDetectionMillis = millis();
  while (true) {
    long irValue = particleSensor.getIR();
    bool fingerDetected = irValue > 50000;
    unsigned long currentMillis = millis();
    if (fingerDetected) {
      if (!fingerJustDetected) {
        fingerJustDetected = true;
        fingerDetectedAt = currentMillis;
      }
      if (currentMillis - fingerDetectedAt >= 1000) {
        activeMeasurementTime += (currentMillis - lastDetectionMillis);
        lastDetectionMillis = currentMillis;
        if (checkForBeat(irValue)) {
          long delta = millis() - lastBeat;
          lastBeat = millis();
          beatsPerMinute = 60 / (delta / 1000.0);
          if (beatsPerMinute < 255 && beatsPerMinute > 20) {
            bpmSum += beatsPerMinute;
            bpmCount++;
          }
        }
        if (currentMillis - lastUpdateTime >= updateInterval) {
          lastUpdateTime = currentMillis;
          avg = bpmCount > 0 ? bpmSum / bpmCount : 0;
          lcd.setCursor(0, 0);
          lcd.print("Mengukur BPM...");
          lcd.setCursor(0, 1);
          lcd.print("BPM: ");
          lcd.print(avg);
          lcd.print("        ");
          Serial.print("Current BPM: ");
          Serial.print(beatsPerMinute);
          Serial.print(" | Avg BPM: ");
          Serial.println(avg);
        }
      }
    } else {
      fingerJustDetected = false;
      lastDetectionMillis = currentMillis;
      lcd.setCursor(0, 0);
      lcd.print("Detak per Menit");
      lcd.setCursor(0, 1);
      lcd.print("Letakkan jari   ");
    }
    if (activeMeasurementTime >= bpmMeasurementTime) {
      avg = bpmCount > 0 ? bpmSum / bpmCount : 0;
      ValueHR = avg / 0.75;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Detak per Menit");
      lcd.setCursor(0, 1);
      lcd.print("Hasil BPM: ");
      lcd.print(avg);
      delay(2000);
      break;
    }
  }
}

void konfirmasiUlang() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Apakah yakin?");
  lcd.setCursor(0, 1);
  lcd.print("<no>        yes ");
  bool confirmYes = false;
  while (true) {
    if (isButtonPressed(buttonUp) || isButtonPressed(buttonDown)) {
      confirmYes = !confirmYes;
      lcd.setCursor(0, 1);
      lcd.print(confirmYes ? " no        <yes>" : "<no>        yes ");
    }
    if (isButtonPressed(buttonLeft)) {
      heartRate();
      break;
    }
    if (isButtonPressed(buttonRight) && confirmYes) {
      if (ValueID != 0 && ValueAge != 0 && ValueSex != 0 && ValueBP != 0 &&
          ValueHR != 0 && ValueST != "Unkown") {
        NaiveBayes();
        break;
      } else {
        lcd.setCursor(0, 1);
        lcd.print(" Lengkapi data! ");
        delay(2000);
        konfirmasiUlang();
        return;
      }
   // NaiveBayes();
    }
  }
}

void NaiveBayes() {
  lcd.clear();
  probYes *= getProbabilityAge(ValueAge, "yes");
  probNo *= getProbabilityAge(ValueAge, "no");
  probYes *= getProbabilitySex(ValueSex, "yes");
  probNo *= getProbabilitySex(ValueSex, "no");
  probYes *= getProbabilityRestingBP(ValueBP, "yes");
  probNo *= getProbabilityRestingBP(ValueBP, "no");
  probYes *= getProbabilityMaxHR(ValueHR, "yes");
  probNo *= getProbabilityMaxHR(ValueHR, "no");
  probYes *= getProbabilitySTSlope(ValueST, "yes");
  probNo *= getProbabilitySTSlope(ValueST, "no");
  lcd.setCursor(0, 0);
  lcd.print("Hasil Prediksi:");
  lcd.setCursor(0, 1);
  lcd.print(probYes > probNo ? "Berpeluang" : "Tdk Berpeluang");
  ValuePred = (probYes > probNo) ? "Berpeluang" : "Tidak Berpeluang";
  melihat();
  konversi(ValueSex,ValueST, ValuePred);
  saveSelectedDataToJson(ValueConvID, ValueAge, ValueBP, ValueHR, ValueConvSex, ValueConvST, ValueConvPred, waktu);
  printSavedData();
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Simpan Selesai!");
  lcd.setCursor(0, 1);
  lcd.print("Restart ESP32...");
  delay(2000);
  ESP.restart();
  while (true) {
    if (isButtonPressed(buttonLeft)) {
      konfirmasiUlang();
      break;
    }
  }
}

void menuWiFiAtauLanjut() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Setup WiFi ?");
  lcd.setCursor(0, 1);
  lcd.print("<no>        yes ");
  bool pilihYes = false;
  while (true) {
    if (isButtonPressed(buttonUp) || isButtonPressed(buttonDown)) {
      pilihYes = !pilihYes;
      lcd.setCursor(0, 1);
      lcd.print(pilihYes ? " no        <yes>" : "<no>        yes ");
    }
    if (isButtonPressed(buttonRight)) {
      if (pilihYes) {
        startWiFiSetup();
        break; 
      } else {
        menu1WiFiAtauLanjut();
        break; 
      }
    }
  }
}

void ensureWiFiConnected() {
  ssid = readFile(LittleFS, ssidPath);
  pass = readFile(LittleFS, passPath);
  while (ssid == "" || pass == "") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Belum Diatur");
    lcd.setCursor(0, 1);
    lcd.print("Atur WiFi Dulu!");
    delay(1500);
    startWiFiSetup();
    ssid = readFile(LittleFS, ssidPath);
    pass = readFile(LittleFS, passPath);
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sambung WiFi...");
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid.c_str(), pass.c_str());
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 30) {
      delay(500);
      lcd.setCursor(0, 1);
      lcd.print("Mencoba...");
      retry++;
    }
    if (WiFi.status() != WL_CONNECTED) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WiFi Gagal!");
      lcd.setCursor(0, 1);
      lcd.print("Coba Lagi...");
      delay(1500);
    }
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Terhubung!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());
  delay(1500);
}

void menu1WiFiAtauLanjut() {
  ensureWiFiConnected(); 
  if (WiFi.status() == WL_CONNECTED) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sinkron Waktu...");
    Serial.println("Memulai sinkronisasi waktu NTP...");
    timeClient.begin(); 
    unsigned long ntpAttemptStart = millis();
    bool ntpSuccess = false;
    int dots = 0;
    while (millis() - ntpAttemptStart < 30000) {
      lcd.setCursor(0, 1);
      String dotsStr = "Mencoba";
      for (int i = 0; i < dots; i++) dotsStr += ".";
      for (int i = dots; i < 4; i++) dotsStr += " ";
      lcd.print(dotsStr.substring(0, 16));
      dots = (dots + 1) % 5;
      if (timeClient.forceUpdate()) { 
        unsigned long epochTime = timeClient.getEpochTime();
        Serial.println("NTP forceUpdate berhasil, epoch: " + String(epochTime));
        if (epochTime > 1577836800UL) { 
          Serial.println("NTP Berhasil disinkronkan.");
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Waktu Tersinkron");
          lcd.setCursor(0, 1);
          lcd.print(timeClient.getFormattedTime());
          ntpSuccess = true;
          delay(1500);
          break;
        } else {
          Serial.println("NTP update OK, tapi waktu masih belum valid (epoch: " + String(epochTime) + ")");
        }
      } else {
        Serial.println("NTP forceUpdate gagal.");
      }
      delay(1000); 
    }
    if (!ntpSuccess) {
      Serial.println("Gagal sinkronisasi NTP setelah 30 detik. Waktu mungkin tidak akurat.");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sinkron Gagal!");
      lcd.setCursor(0, 1);
      lcd.print("Waktu Tidak Akurat");
      delay(2000); 
    }
  } else {
    Serial.println("WiFi tidak terhubung, tidak bisa sinkronisasi NTP.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Offline!");
    lcd.setCursor(0, 1);
    lcd.print("NTP Tidak Bisa");
    delay(2000);
  }
  menuKirimFirebaseAtauLanjut();
}

void menuKirimFirebaseAtauLanjut() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Send Data ?");
  lcd.setCursor(0, 1);
  lcd.print("<no>        yes ");
  bool pilihYes = false;
  while (true) {
    if (isButtonPressed(buttonUp) || isButtonPressed(buttonDown)) {
      pilihYes = !pilihYes;
      lcd.setCursor(0, 1);
      lcd.print(pilihYes ? " no        <yes>" : "<no>        yes ");
    }
    if (isButtonPressed(buttonRight)) {
      if (pilihYes) {
        if (WiFi.status() != WL_CONNECTED) {
          ensureWiFiConnected();
        }
        setupFirebase();
        sendDataToFirebase();
        break;
      } else {
        idUser();
        break;
      }
    }
  }
}

float getProbabilityAge(int age, String condition) {
  if (age >= 28 && age <= 37)
    return (condition == "yes") ? 0.0255906 : 0.0878049;
  if (age >= 38 && age <= 47)
    return (condition == "yes") ? 0.1417323 : 0.2878049;
  if (age >= 48 && age <= 57)
    return (condition == "yes") ? 0.3681102 : 0.3951220;
  if (age >= 58 && age <= 67)
    return (condition == "yes") ? 0.3897638 : 0.1902439;
  if (age >= 68 && age <= 77)
    return (condition == "yes") ? 0.0748031 : 0.0390244;
  return 0.0;
}

float getProbabilitySex(String sex, String condition) {
  if (sex == "M")
    return (condition == "yes") ? 0.9015748 : 0.6512195;
  if (sex == "F")
    return (condition == "yes") ? 0.0984252 : 0.3487805;
  return 0.0;
}

float getProbabilityRestingBP(int bp, String condition) {
  if (bp < 120)
    return (condition == "yes") ? 0.1732283 : 0.1780488;
  if (bp >= 120 && bp <= 129)
    return (condition == "yes") ? 0.2086614 : 0.2634146;
  if (bp >= 130 && bp <= 139)
    return (condition == "yes") ? 0.2145669 : 0.2609756;
  if (bp >= 140 && bp <= 179)
    return (condition == "yes") ? 0.3779528 : 0.2804878;
  if (bp >= 180)
    return (condition == "yes") ? 0.0255906 : 0.0170732;
  return 0.0;
}

float getProbabilityMaxHR(int hr, String condition) {
  if (hr < 135)
    return (condition == "yes") ? 0.6279528 : 0.2536585;
  if (hr >= 135 && hr <= 150)
    return (condition == "yes") ? 0.2145669 : 0.2634146;
  if (hr >= 151 && hr <= 165)
    return (condition == "yes") ? 0.1003937 : 0.2341463;
  if (hr > 165)
    return (condition == "yes") ? 0.0570866 : 0.2487805;
  return 0.0;
}

float getProbabilitySTSlope(String slope, String condition) {
  if (slope == "Up")
    return (condition == "yes") ? 0.1535433 : 0.7731707;
  if (slope == "Flat")
    return (condition == "yes") ? 0.7500000 : 0.1926829;
  if (slope == "Down")
    return (condition == "yes") ? 0.0964567 : 0.0341463;
  return 0.0;
}

void melihat(){
  Serial.println("=== Data Pasien ===");
  Serial.print("ID: ");
  Serial.println(ValueID);  
  Serial.print("Usia: ");
  Serial.println(ValueAge);
  Serial.print("Tekanan Darah (BP): ");
  Serial.println(ValueBP);
  Serial.print("Denyut Jantung (HR): ");
  Serial.println(ValueHR);
  Serial.print("Jenis Kelamin: ");
  Serial.println(ValueSex);
  Serial.print("Segmen ST: ");
  Serial.println(ValueST);
  Serial.print("Hasil Prediksi: ");
  Serial.println(ValuePred);
  }

void initLittleFS() {
  LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED);
}

void createDir(fs::FS &fs, const char *path) {
  fs.mkdir(path);
}

String readFile(fs::FS &fs, const char *path) {
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    return String();
  }
  String fileContent;
  while (file.available()) {
    fileContent = file.readStringUntil('\n');
    break;
  }
  return fileContent;
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    return;
  }
  file.print(message);
}

void deleteFile(fs::FS &fs, const char *path) {
  fs.remove(path);
}

void appendToFile(fs::FS &fs, const char *path, const char *message) {
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    return;
  }
  file.println(message);
  file.close();
}

void printSavedData() {
  File file = LittleFS.open(var2Path, FILE_READ);
  if (!file) {
    Serial.println("File tidak ditemukan!");
    return;
  }
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
  Serial.println();
}

String getTimestamp() {
  timeClient.update(); 
  unsigned long epochTime = timeClient.getEpochTime();
  if (epochTime < 1577836800UL) {
    Serial.println("[ERROR] NTP belum sinkron! epoch: " + String(epochTime) + ". Mengembalikan default timestamp.");
    return "1970-01-01T00:00:00+07:00"; 
  }
  time_t epoch_for_gmtime = epochTime; 
  struct tm *ptm = gmtime(&epoch_for_gmtime);
  if (!ptm) {
    Serial.println("[ERROR] gmtime gagal mengkonversi epoch: " + String(epoch_for_gmtime));
    return "0000-00-00T00:00:00+07:00"; 
  }
  char buffer[26];  
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d+07:00",
           ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
           ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  return String(buffer);
}

void startWiFiSetup() {
  WiFi.softAP("ESP-WIFI-MANAGER", NULL);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP); 
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Setup");
  lcd.setCursor(0, 1);
  lcd.print(IP.toString());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", wifiManager_html);
  });

  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
    int params = request->params();
    for (int i = 0; i < params; i++) {
      const AsyncWebParameter* p = request->getParam(i);
      if (p->isPost()) {
        if (p->name() == PARAM_INPUT_1) {
          ssid = p->value().c_str();
          writeFile(LittleFS, ssidPath, ssid.c_str());
        }
        if (p->name() == PARAM_INPUT_2) {
          pass = p->value().c_str();
          writeFile(LittleFS, passPath, pass.c_str());
        }
        if (p->name() == PARAM_INPUT_3) {
          ip = p->value().c_str();
          writeFile(LittleFS, ipPath, ip.c_str());
        }
        if (p->name() == PARAM_INPUT_4) {
          gateway = p->value().c_str();
          writeFile(LittleFS, gatewayPath, gateway.c_str());
        }
      }
    }
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Tersimpan!");
    lcd.setCursor(0, 1);
    lcd.print("Restart ESP32...");

    request->send(200, "text/plain",
      "WiFi berhasil dikonfigurasi!\n"
      "ESP32 akan restart dalam 3 detik...");
      
    delay(3000);
    ESP.restart();
  });

  server.begin();
}

void initWiFi() {
  ssid = readFile(LittleFS, ssidPath);
  pass = readFile(LittleFS, passPath);
  if (ssid == "") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Belum Diatur");
    lcd.setCursor(0, 1);
    lcd.print("Atur WiFi Dulu!");
    delay(2000);
    return;
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sambung WiFi...");
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  WiFi.mode(WIFI_STA);
  if (ip != "") {
    localIP.fromString(ip.c_str());
    localGateway.fromString(gateway.c_str());
    if (!WiFi.config(localIP, localGateway, subnet)) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Konfigurasi Gagal");
      delay(1500);
    }
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  unsigned long currentMillis = millis();
  previousMillis = currentMillis;
  while (WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= intervalWifi) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WiFi Gagal!");
      lcd.setCursor(0, 1);
      lcd.print("Restart ESP32...");
      delay(2000);
      ESP.restart();
    }
    delay(500);
    lcd.setCursor(0, 1);
    lcd.print("Mencoba...");
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Terhubung!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());
  delay(1500);
  setupFirebase();
  sendDataToFirebase();
}

void setupFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")) {
    Firebase.reconnectWiFi(true);
  } else {
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void konversi(String ValueSex, String ValueST, String ValuePred) {
    if (ValueID >= 0 && ValueID <= 9) {
        ValueConvID = "00" + String(ValueID);
    } else if (ValueID >= 10 && ValueID <= 99) {
        ValueConvID = "0" + String(ValueID);
    } else if (ValueID >= 100 && ValueID <= 999) {
        ValueConvID = String(ValueID);
    } else {
        ValueConvID = "000";
    }

    if (ValueSex == "M") {
        ValueConvSex = "1";
    } else if (ValueSex == "F") {
        ValueConvSex = "2";
    } else {
        ValueConvSex = "5";
    }

    if (ValuePred == "Tidak Berpeluang") {
        ValueConvPred = "2";
    } else if (ValuePred == "Berpeluang") {
        ValueConvPred = "1";
    } else {
        ValueConvPred = "5";
    }

    if (ValueST == "Up") {
        ValueConvST = "2";
    } else if (ValueST == "Down") {
        ValueConvST = "0";
    } else if (ValueST == "Flat") {
        ValueConvST = "1";
    } else if (ValueST == "Unknown") {
        ValueConvST = "5";
    }
}

void saveSelectedDataToJson(String ValueID, int ValueAge, int ValueBP, int ValueHR, String ValueSex, String ValueST, String ValuePred, String waktu) {
  DynamicJsonDocument doc(8192);
  File file = LittleFS.open(var2Path, FILE_READ);
  if (file) {
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error || !doc["sensor"] || !doc["sensor"].is<JsonObject>()) {
      doc.clear();
      doc.createNestedObject("sensor");
    }
  } else {
    doc.clear();
    doc.createNestedObject("sensor");
  }
  JsonObject sensorObj = doc["sensor"].as<JsonObject>();
  JsonObject data = sensorObj.createNestedObject(waktu);
  data["Age"] = String(ValueAge);
  data["Blood_Pressure"] = String(ValueBP);
  data["Max_HR"] = String(ValueHR);
  data["Prediction"] = ValuePred;
  data["ST_Slope"] = ValueST;
  data["Sex"] = ValueSex;
  data["idmicro"] = ValueID;
  data["timestamp"] = waktu;
  file = LittleFS.open(var2Path, FILE_WRITE);
  if (!file) {
    Serial.println("Gagal membuka file untuk menulis.");
    return;
  }
  serializeJson(doc, file);
  file.close();
  Serial.println("Data berhasil DITAMBAHKAN ke file JSON di LittleFS (KEY = WAKTU, STRUCTURE SAMA DENGAN PERMINTAAN)");
}

void sendDataToFirebase() {
  if (!Firebase.ready()) {
    Serial.println("Firebase tidak siap!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Firebase Tidak Siap");
    delay(2000);
    return;
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Kirim ke Firebase");
  lcd.setCursor(0, 1);
  lcd.print("Mengirim...");
  File file = LittleFS.open(var2Path, FILE_READ);
  if (!file) {
    Serial.println("Gagal membuka file data di LittleFS.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Gagal Buka File!");
    delay(2000);
    return;
  }
  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) {
    Serial.println("Gagal parse JSON dari file data!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Gagal Parse JSON!");
    delay(2000);
    return;
  }
  if (!doc.containsKey("sensor") || !doc["sensor"].is<JsonObject>()) {
    Serial.println("Format JSON tidak sesuai (tidak ada object 'sensor').");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("JSON Tidak Valid!");
    delay(2000);
    return;
  }
  JsonObject sensorObj = doc["sensor"].as<JsonObject>();
  bool allSuccess = true;
  for (JsonPair kv : sensorObj) {
    const char* timestamp = kv.key().c_str();
    JsonObject dataObj = kv.value().as<JsonObject>();
    FirebaseJson singleDataJson;
    singleDataJson.set("Age", dataObj["Age"].as<String>());
    singleDataJson.set("Blood_Pressure", dataObj["Blood_Pressure"].as<String>());
    singleDataJson.set("Max_HR", dataObj["Max_HR"].as<String>());
    singleDataJson.set("Prediction", dataObj["Prediction"].as<String>());
    singleDataJson.set("ST_Slope", dataObj["ST_Slope"].as<String>());
    singleDataJson.set("Sex", dataObj["Sex"].as<String>());
    singleDataJson.set("idmicro", dataObj["idmicro"].as<String>());
    singleDataJson.set("timestamp", dataObj["timestamp"].as<String>());
    String path = String("/sensor/") + timestamp;
    if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &singleDataJson)) {
      Serial.println("Data berhasil dikirim ke Firebase di path: " + path);
    } else {
      Serial.print("Gagal mengirim data ke Firebase di path: ");
      Serial.print(path);
      Serial.print(". Error: ");
      Serial.println(fbdo.errorReason());
      allSuccess = false;
    }
  }
  if (allSuccess) {
    Serial.println("Semua data berhasil dikirim ke Firebase sesuai struktur yang diminta.");
    deleteFile(LittleFS, var2Path);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Data Terkirim!");
    lcd.setCursor(0, 1);
    lcd.print("Restart ESP32...");
    delay(2000);
    ESP.restart();
  } else {
    Serial.println("Beberapa data gagal dikirim ke Firebase.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Gagal Kirim Data!");
    lcd.setCursor(0, 1);
    lcd.print("Cek Koneksi!");
    delay(2000);
  }
}
