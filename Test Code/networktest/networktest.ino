#ifndef VSCODE
#include <ESP8266WiFi.h> //for Wifi connectivity
#include <ESP8266WebServer.h> //for hosting the API
#include <ESP8266mDNS.h> //for giving a friendly hostname
#include <WiFiManager.h> //for WiFi user interface
#include <ESP_Mail_Client.h> //for sending SMS
#include <ESP8266HTTPClient.h> //for doing capitve portal login
#include <WiFiClientSecureBearSSL.h> //for doing captive portal login with https
#else

#include "../../Libraries/ESP8266/cores/esp8266/Arduino.h"
#include "../../Libraries/ESP8266/libraries/ESP8266WiFi/src/ESP8266WiFi.h"
#include "../../Libraries/ESP8266/libraries/ESP8266WebServer/src/ESP8266WebServer.h"
#include "../../Libraries/ESP8266/libraries/ESP8266mDNS/src/ESP8266mDNS.h"
#include "../../Libraries/WiFiManager/WiFiManager.h"
#include "../../Libraries/ESP-Mail-Client/src/ESP_Mail_Client.h"
#include "../../Libraries/ESP8266/libraries/ESP8266HTTPClient/src/ESP8266HTTPClient.h"
#include "../../Libraries/ESP8266/libraries/ESP8266WiFi/src/WiFiClientSecureBearSSL.h"

#endif

#include "password.h"

  
ESP8266WebServer server(80);

WiFiManager manager;

SMTPSession smtp;

//Handles http request 
void handleRoot() {
  digitalWrite(2, 0);   //Blinks on board led on page request 
  server.send(200, "text/plain", "hello from esp8266!");
  digitalWrite(2, 1);
}

void smtpCallback(SMTP_Status status);

bool captiveLogin();

void sendSMS(String phonenumber, String messageText);

// the setup function runs once when you press reset or power the board
void setup() {
  
  Serial.begin(115200);

  manager.autoConnect("Oops All EEs");

  if(!captiveLogin()) {
    Serial.println("Capive login failed");
    return;
  }

  Serial.println("Internet Access Achieved");

  if(WiFi.status() == WL_CONNECTED) //If WiFi connected to hot spot then start mDNS
  {
    if (MDNS.begin("oopsallees")) {  //Start mDNS with name esp8266
      Serial.println("MDNS started");
    }
  }

  server.on("/", handleRoot);  //Associate handler function to path
    
  server.begin();                           //Start server
  Serial.println("HTTP server started");

  static const char* LOCATION = "Location";
  static const char* SET_COOKIE = "Set-Cookie";
  static const char* HEADER_NAMES[] = {LOCATION, SET_COOKIE};

  std::unique_ptr<BearSSL::WiFiClientSecure>sclient(new BearSSL::WiFiClientSecure);

  sclient->setInsecure();

  HTTPClient http;

  String uri = "https://console.firebase.google.com/project/oops-all-ees-lab-1/database/data/";

  http.begin(*sclient, uri);
  http.collectHeaders(HEADER_NAMES, 2);
  int httpCode = http.GET();

  Serial.print("response from: ");
  Serial.println(uri);
  Serial.print("code: ");
  Serial.println(httpCode);
  Serial.print("location: ");
  Serial.println(http.header(LOCATION));
  Serial.println("body: ");
  Serial.println(http.getString());
}

// the loop function runs over and over again forever
void loop() {
  server.handleClient();
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status){
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
  }
}

void sendSMS(String phonenumber, String messageText) {
  /** Enable the debug via Serial port
   * none debug or 0
   * basic debug or 1
  */
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Declare the session config data */
  ESP_Mail_Session session;

  /* Set the session config */
  session.server.host_name = "smtp.gmail.com";
  session.server.port = 465;
  session.login.email = "oopsallees@gmail.com";
  session.login.password = EMAILPASSWORD;
  session.login.user_domain = "";

  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = "ESP";
  message.sender.email = "oopsallees@gmail.com";
  message.subject = "ECE:4880 Lab 1 Alert";
  message.addRecipient("Alertee", (phonenumber + "@vtext.com").c_str() );

  //Send raw text message
  message.text.content = messageText.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  if (!smtp.connect(&session))
    return;

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
}

bool captiveLogin() {
  static const char* LOCATION = "Location";
  static const char* SET_COOKIE = "Set-Cookie";
  static const char* HEADER_NAMES[] = {LOCATION, SET_COOKIE};

  Serial.println("Attempting captive login");

  std::unique_ptr<BearSSL::WiFiClientSecure>sclient(new BearSSL::WiFiClientSecure);

  sclient->setInsecure();

  WiFiClient client;

  HTTPClient http;

  String uri = "http://captive.apple.com/?cmd=redirect&arubalp=12345";
  http.begin(client, uri);
  http.collectHeaders(HEADER_NAMES,2);
  int httpCode = http.GET();

  Serial.print("response from: ");
  Serial.println(uri);
  Serial.print("code: ");
  Serial.println(httpCode);
  if (httpCode == 200) { //we can hit the internet, return success
    return true;
  }
  if (httpCode != 302 || !http.hasHeader(LOCATION)) { //if they don't have a location for us then we're screwed
    return false;
  }
  uri = http.header(LOCATION);
  Serial.print("portal=");
  Serial.println(uri);
  http.end();

  uri += "&_browser=1";
  http.begin(*sclient, uri);
  http.collectHeaders(HEADER_NAMES, 2);
  httpCode = http.GET();
  Serial.print("Trying to hit portal: ");
  Serial.println(httpCode);
  Serial.println("Response: ");
  Serial.println(http.getString());
  if (httpCode != 200) {
    return false;
  }
  String cookie = http.header(SET_COOKIE);
  Serial.print("cookie=");
  Serial.println(cookie);
  http.end();
  
  /*
  http.begin(*sclient, uri.substring(0,uri.indexOf("?"))+"?_browser=1");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Cookie", cookie);
  http.collectHeaders(HEADER_NAMES, 2);
  
  String postcode = uri.substring(uri.indexOf("?"),uri.length());
  postcode += "&no_login=&static_u=&no_u=&no_p=&user=ui_guestuser&password=&visitor_accept_terms=1";
  postcode += "fuckyouletmein";
  httpCode = http.POST(postcode);
  Serial.print("Trying to hit portal with form: ");
  Serial.println(httpCode);
  Serial.println("Response: ");
  Serial.println(http.getString().substring(0,100));
  http.end();*/


  http.begin(*sclient, "https://arubactrl.its.uiowa.edu/cgi-bin/login");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Cookie", cookie);
  http.collectHeaders(HEADER_NAMES, 2);
  httpCode = http.POST("user=ui_guestuser&password=529376&url=https%3A%2F%2Fwww.uiowa.edu&cmd=authenticate&Login=Log+In");
  Serial.print("Trying to hit sneaky login server: ");
  Serial.println(httpCode);
  Serial.println("Response: ");
  Serial.println(http.getString());
  Serial.print("Redirect: ");
  Serial.println(http.header(LOCATION));
  http.end();

  uri = "http://captive.apple.com/?cmd=redirect&arubalp=12345";
  http.begin(client, uri);
  http.collectHeaders(HEADER_NAMES,2);
  httpCode = http.GET();

  Serial.print("response from: ");
  Serial.println(uri);
  Serial.print("code: ");
  Serial.println(httpCode);
  if (httpCode == 200) { //we can hit the internet, return success
    return true;
  }
  return false;
}