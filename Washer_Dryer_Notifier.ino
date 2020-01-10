/*
 * Author: Scott Mayberry
 * Date: 1/10/2020
 * Description: This code monitors changes in a vibration sensor to detect when a washer/dryer starts and ends.
 * When VIBRATION_MONITORING_THRESHOLD is exceeded (when the vibration sensor triggers enough to guarantee it is not a false
 * positive), the system begins to monitor the vibration sensor, looking for a signifcant stopping point. If the vibration 
 * sensor stops for VIBRATION_STOPPED_TIME_DELAY_SECONDS, then this code sends a notification to the IFTTT Webhooks platform, 
 * which subsequently publishes a notifcation on the subscribed phones.
 * 
 * If vibrations do not exceed VIBRATION_MONITORING_THRESHOLD, and the amount of time between the last vibration exceeds 
 * VIBRATION_COUNTER_RESET_DELAY_SECONDS, then the vibration counter will be reset to 0.
 * 
 * There are optional delays added for when the system changes state from Monitoring Stops to Waiting for Threshold. These
 * can be commented out.
 * 
 * 
 * Requirements:
 * Libraries Required:
 * 1. DatatoMaker.h file is required. Can be found at https://github.com/mylob/ESP-To-IFTTT
 * 
 * Personalization Requirements for Successful operation:
 * 1. Change WIFI_SSID to personal WIFI_SSID
 * 2. Change WIFI_PASS to personal WIFI_PASS
 * 3. Change STARTED_MESSAGE and STOPPED_MESSAGE to the proper device you are monitoring
 * 4. Change WEBHOOKS_EVENT to the proper event decided in the IFTTT Webhooks section
 * 5. Change myWebhooksKey and otherWebhooksKey to the keys provided in the IFTTT Webhooks section
 * 6. Delete or add extra DataToMaker event objects for IFTTT publishing (this may not be necessary, but I decided to use seperate objects)
 * 7. Adjust all thresholds and times to fit your needs
 * 
 */





#include <ESP8266WiFi.h>
#include "DataToMaker.h"

#define WIFI_SSID "null"
#define WIFI_PASS "null"
#define SERIAL_BAUDRATE 115200

#define STARTED_MESSAGE "Washer started."
#define STOPPED_MESSAGE "Washer is done."

#define WEBHOOKS_EVENT "null";



#define VIB_PIN D5
#define VIBRATION_MONITORING_THRESHOLD 5000
#define VIBRATION_STOPPED_TIME_DELAY_SECONDS 150
#define VIBRATION_COUNTER_RESET_DELAY_SECONDS 180

/*Vibration State
   0. Monitoring for vibration stop
   1. Counting vibrations before starting to monitor for vibration stop. This will eliminate false positives
*/
int vibrationState;
long lastVibrationStateChange;//last vibration state change times tracking for buffer size
int previousVibrationValue;

long startedTime;

int vibrationCounter;

const char* myWebhooksKey = "null"; // your maker key here
const char* otherWebhooksKey = "null";
DataToMaker event(myWebhooksKey, WEBHOOKS_EVENT);
DataToMaker event2(otherWebhooksKey, WEBHOOKS_EVENT);


void wifiSetup() {
  // Set WIFI module to STA mode
  WiFi.mode(WIFI_STA);

  // Connect
  Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Wait
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  // Connected!
  Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}//wifi setup

void pinModeSetup() {
  pinMode(VIB_PIN, INPUT);
}//pinModeSetup

void setup() {
  // put your setup code here, to run once:
  Serial.begin(SERIAL_BAUDRATE);
  wifiSetup();
  pinModeSetup();
  vibrationState = 1;
  lastVibrationStateChange = millis();
  previousVibrationValue = 0;
  vibrationCounter = 0;
  startedTime = 0;
}//setup


void washerDryerNotification() {
  //Serial.println("WASHER_DRYER_STOPPED");
  //first event code (my phone notification)
  while (!event.connect()) {
    Serial.print(".");
  }
  event.setValue(1, STOPPED_MESSAGE);
  delay(100);
  event.post();
  //second event code (other phone notification)
  while (!event2.connect()) {
    Serial.print(".");
    }
  event2.setValue(1, STOPPED_MESSAGE);
  delay(100);
  event2.post();
}//washerDryerNotification
void washerDryerStarted() {
  //Serial.println("WASHER_DRYER_STARTED");
  //first event code (my phone notification)
  while (!event.connect()) {
    Serial.print(".");
  }
  event.setValue(1, STARTED_MESSAGE);
  delay(100);
  event.post();
  //second event code (other phone notification)
  while (!event2.connect()) {
    Serial.print(".");
    }
  event2.setValue(1, STARTED_MESSAGE);
  delay(100);
  event2.post();
}//washerDryerNotification
void checkForVibration() {
  //read the digital pin and compare it to the previous value
  //if value is different, update the new value and reset the lastVibrationStateChange
  if (digitalRead(VIB_PIN) != previousVibrationValue) {
    previousVibrationValue = abs(previousVibrationValue - 1);
    if (vibrationState == 1) {
      vibrationCounter++;
      //Serial.println(vibrationCounter);
    }
    lastVibrationStateChange = millis();
  }//read the digital pin and compare it
  if (vibrationState == 1 && abs(millis() - lastVibrationStateChange) > VIBRATION_COUNTER_RESET_DELAY_SECONDS * 1000) {
    vibrationCounter = 0;
    //Serial.println("Vibration Counter Reset");
  }
}//checkForVibration
void checkForVibrationStop() {
  //check to see if the time between the last vibration state change is greater than delay
  //if so, update washer dryer notification, and go into next state
  if (abs(millis() - lastVibrationStateChange) > VIBRATION_STOPPED_TIME_DELAY_SECONDS * 1000) {
    washerDryerNotification();
    vibrationState = 1;
    vibrationCounter = 0;
    Serial.println("WAITING FOR THRESHOLD");
    //optional delay before moving to monitoring for false positives
    delay(60*1000);
  }
}//checkForVibrationStop

void checkVibrationCounter() {
  //if enough vibrations have occured to stop false positives
  if (vibrationCounter > VIBRATION_MONITORING_THRESHOLD) {
    vibrationCounter = 0;
    vibrationState = 0;
    startedTime = millis();
    washerDryerStarted();
    Serial.println("MONITORING STOP");
    //optional delay before moving to monitoring for false positives
    delay(60*1000);
  }
}//checkVibrationCounter

void loop() {
  // put your main code here, to run repeatedly:
  checkForVibration();
  if (vibrationState == 0) { //if vibration state is set to monitoring stoppage
    checkForVibrationStop();
  } else if (vibrationState == 1) { //if vibration state is set to minimize false positives
    checkVibrationCounter();
  }
}//loop
