#include <MFRC522.h>  //library responsible for communicating with the module RFID-RC522
#include "WiFi.h"
#include <map>
#include <vector>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RTClib.h"
#include <time.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define API_KEY "AIzaSyC_8wwCLsEfS58m6Y2UCRAuqiAwcEB0b3Y"
#define USER_EMAIL "abhilashbhatt69@gmail.com"
#define USER_PASSWORD "HalaMadrid"
#define DATABASE_URL "https://demothingy-5d18e-default-rtdb.asia-southeast1.firebasedatabase.app/"

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1  // Reset pin # (or -1 if sharing Arduino resetpin)
#define SS_PIN 15
#define RST_PIN 13
#define SIZE_BUFFER 18
#define MAX_SIZE_BLOCK 16
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String uid;
String databasePath;
String parentPath;

FirebaseJson json;

RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


DateTime now;
void defdisp();  //for displaying always "welcome to marvel"
void in();       //for displaying card read in
void out();      //for displaying card read out
//used in authentication
MFRC522::MIFARE_Key key;
//authentication return status code
MFRC522::StatusCode status;
// Defined pins to module RC522
MFRC522 mfrc522(SS_PIN, RST_PIN);

unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 1000;
const char *ssid = "Marvel-Guest";
const char *password = "orangesandapples";
WiFiClient client;
String MakerIFTTT_Key;
String MakerIFTTT_Event;
char *append_str(char *here, String s) {
  int i = 0;
  while (*here++ = s[i]) {
    i++;
  };
  return here - 1;
}
char *append_ul(char *here, unsigned long u) {
  char buf[20];
  return append_str(here, ultoa(u, buf, 10));
}
char post_rqst[256];
char *p;
char *content_length_here;
char *json_start;
int compi;
//std::map<String, boolean> m;
std::vector<String> idVec;

void setup() {
  Serial.begin(9600);
  if (!rtc.begin()) {

    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }


  rtc.adjust(DateTime(__DATE__, __TIME__));
  now = rtc.now();


  delay(3000);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = uid + "/Logs/" + String(now.day()) + String(now.month()) + String(now.year()) + "/";

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

  SPI.begin();  // Init SPI bus
  // Init MFRC522

  mfrc522.PCD_Init();
  Serial.println("Approach your reader card...");
  Serial.println();
  // Clear the buffer.
  display.clearDisplay();
  // Display Text
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(2, 10);
  display.println("Welcome to");
  display.setCursor(30, 40);
  display.println("MARVEL");
  display.display();
  delay(5000);
}

void loop() {
  //waiting the card approach
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select a card
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Dump debug info about the card; PICC_HaltA() is automatically called
  // mfrc522.PICC_DumpToSerial(&(mfrc522.uid));</p><p> //call menu function and retrieve the desired option

  readingData();


  //instructs the PICC when in the ACTIVE state to go to a "STOP" state
  mfrc522.PICC_HaltA();
  // "stop" the encryption of the PCD, it must be called after communication with authentication, otherwise new communications can not be initiated
  mfrc522.PCD_StopCrypto1();
}

//reads data from card/tag
void readingData() {
  //prints the technical details of the card/tag
  //mfrc522.PICC_DumpDetailsToSerial( & (mfrc522.uid));
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  Serial.println();
  Serial.print("UID : ");
  content.toUpperCase();
  Serial.print(content);
  String check_status;
  //std::map<String, boolean>::iterator itr;
  std::vector<String>::iterator itr;
  itr = std::find(idVec.begin(), idVec.end(), content);
  //itr = idVec.find(content);
  if (itr != idVec.end()) {
    Serial.println("\nChecked out");
    check_status = "Checked out";
    idVec.erase(itr);
    Serial.println("List");
    for (itr = idVec.begin(); itr != idVec.end(); itr++) {
      Serial.println(*itr);
    }
    out();
    delay(2000);
    defdisp();

  } else if (itr == idVec.end()) {
    Serial.println("\nChecked in");
    check_status = "Checked in";
    idVec.push_back(content);
    Serial.println("List");
    for (itr = idVec.begin(); itr != idVec.end(); itr++) {
      Serial.println(*itr);
    }
    in();
    delay(2000);
    defdisp();
  }
  now = rtc.now();
  if (Firebase.ready()) {

    parentPath = databasePath + String(now.timestamp()) + "/";
    Serial.println(parentPath);
    json.set("rfid_code", content);
    json.set("time", String(now.day()) + "-" + String(now.month()) + "-" + String(now.year()) + " " + String(now.hour()) + ":" + String(now.minute()));
    json.set("status", check_status);
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }

  //prepare the key - all keys are set to FFFFFFFFFFFFh
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  //buffer for read data
  byte buffer[SIZE_BUFFER] = {
    0
  };

  //the block to operate
  byte block = 1;
  byte size = SIZE_BUFFER;                                                                          //authenticates the block to operate
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));  //line 834 of MFRC522.cpp file


  //read data from block
  status = mfrc522.MIFARE_Read(block, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    return;
  }
}

void in() {
  // Clear the buffer.
  display.clearDisplay();
  // Display Text
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(40, 10);
  display.println("In");
  display.setTextSize(2);
  display.setCursor(40, 40);
  display.println(String(now.hour()) + ":" + String(now.minute()));
  display.display();
}
void defdisp() {
  // Clear the buffer.
  display.clearDisplay();
  // Display Text
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(2, 10);
  display.println("Welcome to");
  display.setCursor(30, 40);
  display.println("MARVEL");
  display.display();
  delay(5000);
}
void out() {
  display.clearDisplay();
  // Display Text
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(40, 10);
  display.println("Out");
  display.setTextSize(2);
  display.setCursor(40, 40);
  display.println(String(now.hour()) + ":" + String(now.minute()));
  display.display();
}