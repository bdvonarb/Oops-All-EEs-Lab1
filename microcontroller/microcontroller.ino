#ifndef VSCODE
#include <Arduino.h>
#else
#undef Arduino_h
#include <esp8266/Arduino.h>
#endif
#include <ESP8266WiFi.h> //for Wifi connectivity
#include <ESP8266WebServer.h> //for hosting the API
#include <ESP8266mDNS.h> //for giving a friendly hostname
#include <WiFiManager.h> //for WiFi user interface
#include <ESP_Mail_Client.h> //for sending SMS
#include <ESP8266HTTPClient.h> //for doing capitve portal login
#include <WiFiClientSecureBearSSL.h> //for doing captive portal login with https
#include <OneWire.h> //for communicating with temperature sensor
#include <Ticker.h> //for hardware timer to trigger temp conversions every second
#include <ESP8266Firebase.h> //for accessing our backend database

#include "password.h" //password file contains passwords for SMTP as well as API keys MAKE SURE THIS DOES NOT GET COMMITED TO GIT

//ESP8266 pins are labeled weird, these defines link the physical labeled pins with their Arduino pin numbers
#define D0 16
#define D1 5
#define D2 4
#define D3 0
#define D4 2
#define D5 14
#define D6 12
#define D7 13
#define D8 15
#define D9 3
#define D10 1

//define pins for display shift registers
#define SER D0
#define Sclk D1
#define Rclk D2

//define pin for temperature probe
#define TEMP_PROBE D3

//define bit locations for decimal point and display on
#define D_ON 0x0008
#define DP 0x0100

const int8_t tempdatapowers[] = {-4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 0, 0, 0, 0, 0};

//bit format for each digit
const uint16_t digForm[4] = {0x0080, 0x0040, 0x0020, 0x0010};

//stores bits to be sent to display
uint16_t dispData[4] = {0,0,0,0};

bool dispOn = false;

char disp[6] = "--.--";

byte addr[8];

WiFiManager wifimanager;

SMTPSession smtp;

OneWire tempProbe;

bool captiveLogin();

//we can send text messages via SMTP by sending emials to <phonenumber>@vtext.com
void sendSMS(String phonenumber, String messageText);

//callback for hardware timer to initiate temperature conversion every second
void ICACHE_RAM_ATTR eachSecond();

//sends display data to display
void refreshDisplay();

//sends actual bits to display
void writeToReg(uint16_t trans);

//sets up dispData array with bits needed to display the string
void display(char disp[]);

//converts a char into a byte containing the segments needed to display it
uint8_t segMap(char digit);

//computes exponent, can handle negative exponents
float power(uint8_t base, int8_t exponent);

void setup() {
    Serial.begin(115200); //start serial at 115200 baud for debug information on USB
    Serial.println("---Oops All EEs---");
    
    //configure pins
    pinMode(SER, OUTPUT);
    pinMode(Sclk, OUTPUT);
    pinMode(Rclk, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT); //on the ESP8266 this is pin 2/D4

    //attach temperature probe to OneWire library object
    tempProbe.begin(TEMP_PROBE);
    
    Serial.println("Starting WiFi Manager");

    wifimanager.autoConnect("Oops All EEs"); //automatically connects to previously connected WiFi SSID, 
        //or if this fails it automatically starts a AP that you can connect to from your phone to configure SSID and password
        //simply connect to the AP (no password), then enter the IP address 192.168.4.1 to access the configuration portal

    //now if we are at the university we have a problem, there are three SSIDs we can try to connect to:
    //  eduroam: for this we would need to generate a WPA2 certificate, this while easy on an ESP32, is practically impossible 
    //      on the ESP8266
    //  ui-devicenet: it would seem ITS (for some reason) does not allow ESPs to connect to this network, despite me registering it
    //  ui-guest: this now is our only option, there is no password, but it does have a "captive portal." Normally this could be
    //      handled simply by clicking "Agree" in a browser window, but on the ESP8266 this is not so easily done without manual
    //      intervention, basically what we need to do is hit a top sneaky login server on the university network with a login request
    //      before it will give us access to the internet, the following function does just that

    if(!captiveLogin()) {
        Serial.println("Captive portal login failed, program aborting");
        return;
    }

    Serial.println("Internet Access Achieved!");
    
    //setup timer intrrupt for measuring temperature every second
    timer1_attachInterrupt(eachSecond);
    //timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
    //timer1_write(5000000);



    if(!tempProbe.search(addr)) { //search for temp probe, if it's not there then return
        Serial.println("Temp probe not found");
        return;
    }

    std::unique_ptr<BearSSL::WiFiClientSecure>sclient(new BearSSL::WiFiClientSecure);

    sclient->setInsecure(); //except we don't actually care about security so we just fake it

    HTTPClient http;

    String uri = "https://api.thingspeak.com/update";
    http.begin(*sclient, uri);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded"); //we need to give this content type header so it accepts our form
    String postcode = "api_key=";
    postcode += THINGSPEAK_API_WRITEKEY;
    postcode += "&Temperature=12";
    httpCode = http.POST(postcode);
    Serial.print("response from: ");
    Serial.println(uri);
    Serial.print("code: ");
    Serial.println(httpCode);
    http.end();

}

void loop() {
    refreshDisplay();
}

void display(char disp[]) {
    uint8_t indexOfDP = strchr(disp, '.') - disp;
    for(uint8_t i = 0; i < 4; i++) {
        dispData[i] = digForm[i] | (segMap(disp[i + (i >= indexOfDP)], (i + 1) == indexOfDP ) << 8);
    }
}

uint8_t segMap(char digit, bool dp) {
    uint8_t map = 0;
    switch(digit) {
        case '0':
            map = 0b11111100;
            break;
        case '1':
            map = 0b01100000;
            break;
        case '2':
            map = 0b11011010;
            break;
        case '3':
            map = 0b11110010;
            break;
        case '4':
            map = 0b01100110;
            break;
        case '5':
            map = 0b10110110;
            break;
        case '6':
            map = 0b10111110;
            break;
        case '7':
            map = 0b11100000;
            break;
        case '8':
            map = 0b11111110;
            break;
        case '9':
            map = 0b11110110;
            break;
        case 'A': //A
            map = 0b11101110;
            break;
        case 'b': //b
            map = 0b00111110;
            break;
        case 'c': //c
            map = 0b00011010;
            break;
        case 'd': //d
            map = 0b01111010;
            break;
        case 'E': //E
            map = 0b10011110;
            break;
        case 'F': //F
            map = 0b10001110;
            break;
        case 'r': //r
            map = 0b00001010;
            break;
        case 'H': //H
            map = 0b01101110;
            break;
        case 'L': //L
            map = 0b00011100;
            break;
        case 'o': //o
            map = 0b00111010;
            break;
    }
    return map | dp;
}

void refreshDisplay() {
    writeToReg(dispData[0]);
    writeToReg(dispData[1]);
    writeToReg(dispData[2]);
    writeToReg(dispData[3]);
}

void writeToReg(uint16_t trans) {
    shiftOut(SER,Sclk,LSBFIRST, lowByte(trans) | (D_ON * dispOn));
    shiftOut(SER,Sclk,LSBFIRST, highByte(trans));
    digitalWrite(Rclk, HIGH);
    delayMicroseconds(100);
    digitalWrite(Rclk, LOW);
}

void ICACHE_RAM_ATTR eachSecond() {
    byte data[12];

    //read previously started conversion
    tempProbe.reset();
    tempProbe.select(addr);
    tempProbe.write(0xBE); //read scratchpad
    for (uint8_t i = 0; i < 9; i++) {           // we need 9 bytes
        data[i] = tempProbe.read();
        if(i < 2) {
            Serial.print(data[i], BIN);
        } else {
            Serial.print(data[i], HEX);
        }
        Serial.print(" ");
    }
    Serial.println("");
    uint16_t tempBytes = data[1] << 8 | data[0];
    Serial.println(tempBytes, BIN);

    //start new conversion for next time
    tempProbe.reset();
    tempProbe.select(addr);
    tempProbe.write(0x44); //start conversion

    float temp = 0;

    for(uint8_t i = 0; i < 16; i++) {
        if((tempBytes >> i) & 1) {
            temp += power(2,tempdatapowers[i]);
        }
    }

    Serial.print(" CRC=");
    Serial.print( OneWire::crc8( data, 8), HEX);
    Serial.println();

    temp *= (-2 * ((data[1] >> 7) & 0x01)) + 1;

    sprintf(disp, "%5.1f", temp);
    display(disp);

    Serial.print("Temp: ");
    Serial.println(temp);
}

float power(uint8_t base, int8_t exponent) {
	float result = 1.0; //init at 1 becase anything to the 0 power is 1
	for (int i = 0; i < abs(exponent); i++) {
		result *= base;
	}
	if (exponent < 0) { //if exponent is negative return inverse
		return 1.0/result;
	}
	return result;
}

void sendSMS(String phonenumber, String messageText) {
  smtp.debug(1); //turn on debug messages

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

//this whole function is very very very "happy path" but i'm not fixing it - ben
bool captiveLogin() {
    //We will need to keep track of some cookies during these interactions, the ESP8266 HTTPClient requires that we explicitly 
    //  tell it which headers we want as it will discard all others to save memory or something.
    static const char* LOCATION = "Location";
    static const char* SET_COOKIE = "Set-Cookie";
    static const char* HEADER_NAMES[] = {LOCATION, SET_COOKIE};

    Serial.println("Attempting captive login");

    //some of these sites use https for which we need a secure client
    std::unique_ptr<BearSSL::WiFiClientSecure>sclient(new BearSSL::WiFiClientSecure);

    sclient->setInsecure(); //except we don't actually care about security so we just fake it

    WiFiClient client;

    HTTPClient http;


    //the first thing we do is hit captive.apple.com which will redirect us to the login portal, or if we can already access the
    //  internet it will give us a 200 and we can stop this madness
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
    if (httpCode != 302 || !http.hasHeader(LOCATION)) { //if they don't have a location for us then we're screwed, return false
        return false;
    }
    uri = http.header(LOCATION); //store the location header, cuz that's where we're going next
    Serial.print("portal: ");
    Serial.println(uri);
    http.end();

    //next we try to hit the login portal we were redirected to, this will give us a cookie we need to login to the sneaky server
    // or I guess I just assume we need the cookie, I never actually tried without it
    uri += "&_browser=1"; //we need to append this, I don't know why
    http.begin(*sclient, uri); //notice that because the ui-guest portal uses https we must use the sclient here
    http.collectHeaders(HEADER_NAMES, 2);
    httpCode = http.GET();

    Serial.print("response from: ");
    Serial.println(uri);
    Serial.print("code: ");
    Serial.println(httpCode);
    if (httpCode != 200) { //if this fails we're screwed, return false
        return false;
    }
    String cookie = http.header(SET_COOKIE);
    /* 
                  .---. .---. 
                 :     : o   :    me want cookie!
             _..-:   o :     :-.._    /
         .-''  '  `---' `---' "   ``-.    
       .'   "   '  "  .    "  . '  "  `.  
      :   '.---.,,.,...,.,.,.,..---.  ' ;
      `. " `.                     .' " .'
       `.  '`.                   .' ' .'
        `.    `-._           _.-' "  .'  .----.
          `. "    '"--...--"'  . ' .'  .'  o   `.
          .'`-._'    " .     " _.-'`. :       o  :
    jgs .'      ```--.....--'''    ' `:_ o       :
      .'    "     '         "     "   ; `.;";";";'
     ;         '       "       '     . ; .' ; ; ;
    ;     '         '       '   "    .'      .-'
    '  "     "   '      "           "    _.-'
    */
    Serial.print("cookie: ");
    Serial.println(cookie);
    http.end();

    //now we post a form to the login portal
    uri = uri.substring(0,uri.indexOf("?"))+"?_browser=1";
    http.begin(*sclient, uri);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Cookie", cookie);
    http.collectHeaders(HEADER_NAMES, 2);

    String postcode = uri.substring(uri.indexOf("?"),uri.length());
    postcode += "&no_login=&static_u=&no_u=&no_p=&user=ui_guestuser&password=&visitor_accept_terms=1";
    httpCode = http.POST(postcode);
    Serial.print("response from: ");
    Serial.println(uri);
    Serial.print("code: ");
    Serial.println(httpCode);
    http.end();

    //now technically with that last request we would have agreed to a TOS and clicked a login button, but under the hood that just hits
    //  this super secret login server. We can fake the form it expects in the POST request and it works lol
    uri = "https://arubactrl.its.uiowa.edu/cgi-bin/login";
    http.begin(*sclient, uri);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded"); //we need to give this content type header so it accepts our form
    http.addHeader("Cookie", cookie); //send the cookie along
    http.collectHeaders(HEADER_NAMES, 2);
    //the user and password appear to be completely static, at least it always seems to work. This form was obtained by logging network
    //  traffic on my laptop while logging into ui-guest
    httpCode = http.POST("user=ui_guestuser&password=529376&url=https%3A%2F%2Fwww.uiowa.edu&cmd=authenticate&Login=Log+In");
    Serial.print("response from: ");
    Serial.println(uri);
    Serial.print("code: ");
    Serial.println(httpCode);
    http.end();

    //finally we try to hit captive.apple.com again to see if we can successfully access the network now
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
    return false; //if that didn't work we're screwed return false
}