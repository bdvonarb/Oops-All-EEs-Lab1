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
#define ARDUINO 100 //this is so OneWire includes the right file
#include <OneWire.h> //for communicating with temperature sensor
#include <Ticker.h> //for hardware timer to trigger temp conversions every second
#include <asyncHTTPrequest.h> //for non-interruting HTTP requests

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

//define one *cough* second of ticks
#define ONE_SEC_TICKS ((80000000) / (256)) * 1.2

enum STATE {
    WAIT,
    READ_TEMP,
    START_CONVERSION,
    POST_DATA,
    SEARCH_FOR_PROBE,
    PROBE_RECOVERY_WAIT,
    PROBE_RECOVERY
};

STATE state = WAIT;

const int8_t tempdatapowers[] = {-4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6};

float minTemp = 0.0;
float maxTemp = 100.0;
String phonenumber = "4205556969";

bool alertSent = false;

//bit format for each digit
const uint16_t digForm[4] = {0x0080, 0x0040, 0x0020, 0x0010};

//stores bits to be sent to display
uint16_t dispData[4] = {0,0,0,0};

bool dispOn = false;

char disp[6] = "--.--";

byte addr[8];

WiFiManager wifimanager;

//some of these sites use https for which we need a secure client
std::unique_ptr<BearSSL::WiFiClientSecure>sclient(new BearSSL::WiFiClientSecure);

asyncHTTPrequest ahttp;

HTTPClient http;

SMTPSession smtp;

OneWire tempProbe;

volatile bool requestOut = false;

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

//reads the current temperature and updates the display fields
void readTemp();

//starts a new temperature conversion
void startTempConversion();

//sends temp data to thingspeak
void postTempData(String temp);

//requests config data (display on/off, alert phonenumber, min/max temp) from thingspeak
void GetConfigData();

//checks whether there is a temp probe connected to the box
bool probeAvailable();

//posts NaN to thingspeak to signify lack of probe
void postNaN();

//callback function for aysnc http requests
void responseReceived(void* optParm, asyncHTTPrequest* request, int readyState) {
    requestOut = false;
    String response = request->responseText();
    if(response.charAt(0) == '{') { 
        //Serial.println(response);
        int tmpindex = response.indexOf("\"field1\"");
        dispOn = response.substring(tmpindex+10,response.indexOf("\"", tmpindex+11)).equals("1");
        //Serial.println(dispOn);
        tmpindex = response.indexOf("\"field2\"");
        phonenumber = response.substring(tmpindex+10,response.indexOf("\"", tmpindex+11));
        //Serial.println(phonenumber);
        tmpindex = response.indexOf("\"field3\"");
        minTemp = response.substring(tmpindex+10,response.indexOf("\"", tmpindex+11)).toFloat();
        //Serial.println(minTemp);
        tmpindex = response.indexOf("\"field4\"");
        maxTemp = response.substring(tmpindex+10,response.indexOf("\"", tmpindex+11)).toFloat();
        //Serial.println(maxTemp);
    } else {
        GetConfigData();
    }
}

void setup() {
    Serial.begin(115200); //start serial at 115200 baud for debug information on USB
    Serial.println("---Oops All EEs---");
    
    //configure pins
    pinMode(SER, OUTPUT);
    pinMode(Sclk, OUTPUT);
    pinMode(Rclk, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT); //on the ESP8266 this is pin 2/D4

    display("--.--");

    //attach temperature probe to OneWire library object
    tempProbe.begin(TEMP_PROBE);

    //see if probe is attached, if so, set addr
    tempProbe.search(addr);
    
    //we need to make some https requests later, for that we need a secure client, but we don't actually care about security
    //  because it will be annoying to set up, so we just fake a secure client that's really not secure
    sclient->setInsecure();

    //add callback for async http requests
    ahttp.onData(responseReceived);
    //ahttp.setDebug(true); //prints loads of debug info for the async requests

    Serial.println("Starting WiFi Manager");

    wifimanager.autoConnect("Oops All EEs"); //automatically connects to previously connected WiFi SSID (credentials in EEPROM), 
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

    //starts initial temperature conversion if probe is connected, otherwise starts FSM in search state
    if(probeAvailable()) {
        startTempConversion();
    } else {
        state=SEARCH_FOR_PROBE;
    }
    
    //setup timer intrrupt for measuring temperature every second
    timer1_attachInterrupt(eachSecond);
    timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);
    timer1_write(ONE_SEC_TICKS); //technically 1.2s but the http isn't fast enough to actually do 1s shhhhhhhhhh

}

void loop() {
    switch(state) {
        case WAIT:
            break;
        case READ_TEMP:
            if(probeAvailable()) {
                readTemp();
                state = START_CONVERSION;
            } else {
                state = SEARCH_FOR_PROBE;
            }
            break;
        case START_CONVERSION:
            if(probeAvailable()) {
                startTempConversion();
                state = POST_DATA;
            } else {
                state = SEARCH_FOR_PROBE;
            }
            break;
        case POST_DATA:
            postTempData(String(disp));
            state = WAIT;
            break;
        case SEARCH_FOR_PROBE:
            display("Er.Pr");
            if (probeAvailable()) {
                tempProbe.search(addr);
                startTempConversion();
                state = WAIT;
            } else {
                postNaN();
                state = PROBE_RECOVERY_WAIT;
            }
            break;
        case PROBE_RECOVERY_WAIT:
            break;
        case PROBE_RECOVERY:
            startTempConversion();
            state = WAIT;
            break;
    }
    refreshDisplay();
}

void postTempData(String temp) {
    String postcode = "api_key=" THINGSPEAK_API_WRITEKEY "&field1=";
    temp.trim();
    postcode += temp;

    if(!requestOut) {
        requestOut = true;
        ahttp.open("POST","http://api.thingspeak.com/update");
        ahttp.setReqHeader("Content-Type", "application/x-www-form-urlencoded");
        ahttp.send(postcode);
    }
}

bool probeAvailable() {
    return bool(tempProbe.reset());
}

void GetConfigData() {
    if(!requestOut) {
        requestOut = true;
        ahttp.open("GET","http://api.thingspeak.com/channels/" THINGSPEAK_CHANNEL_ID "/feeds/last.json?api_key=" THINGSPEAK_API_READKEY);
        ahttp.send();
    }
}

void postNaN() {
    if(!requestOut) {
        String postcode = "api_key=" THINGSPEAK_API_WRITEKEY "&field1=NaN";
        requestOut = true;
        ahttp.open("POST","http://api.thingspeak.com/update");
        ahttp.setReqHeader("Content-Type", "application/x-www-form-urlencoded");
        ahttp.send(postcode);
    }
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
        case 'A':
            map = 0b11101110;
            break;
        case 'b':
            map = 0b00111110;
            break;
        case 'c':
            map = 0b00011010;
            break;
        case 'd':
            map = 0b01111010;
            break;
        case 'E':
            map = 0b10011110;
            break;
        case 'F':
            map = 0b10001110;
            break;
        case 'r':
            map = 0b00001010;
            break;
        case 'H':
            map = 0b01101110;
            break;
        case 'L':
            map = 0b00011100;
            break;
        case 'o':
            map = 0b00111010;
            break;
        case 'P':
            map = 0b11001110;
            break;
        case '-':
            map = 0b00000010;
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

void readTemp() {
    byte data[12];

    //read previously started conversion
    tempProbe.reset();
    tempProbe.select(addr);
    tempProbe.write(0xBE); //read scratchpad
    for (uint8_t i = 0; i < 9; i++) {           // we need 9 bytes
        data[i] = tempProbe.read();
    }
    uint16_t tempBytes = data[1] << 8 | data[0];

    float temp = 0;

    for(uint8_t i = 0; i < 11; i++) {
        if((tempBytes >> i) & 1) {
            temp += power(2,tempdatapowers[i]);
        }
    }

    if(tempBytes & 0x8000) {
        temp -= 127.9375;
    }

    if(!alertSent) { //we check the alert sent flag to see if an alert for this condition has already been sent
        if(temp < minTemp) {
            sendSMS(phonenumber, "Temperature out of range low");
            alertSent = true; //so we don't send a text every second when out of range, we set this flag
        } else if (temp > maxTemp) {
            sendSMS(phonenumber, "Temperature out of range high");
            alertSent = true; //so we don't send a text every second when out of range, we set this flag
        }
    } else if(temp > minTemp && temp < maxTemp) { //if temperature is back in range, we reset the alert flag
        alertSent = false;
    }

    sprintf(disp, "%5.1f", temp);
    display(disp);
}

void startTempConversion() {
    tempProbe.reset();
    tempProbe.select(addr);
    tempProbe.write(0x44); //start conversion
}

void ICACHE_RAM_ATTR eachSecond() {
    if (state == PROBE_RECOVERY_WAIT) {
        state = SEARCH_FOR_PROBE;
    } else {
        state = READ_TEMP;
    }
    timer1_write(ONE_SEC_TICKS);
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
  char address[21];
  (phonenumber + "@vtext.com").toCharArray(address, 21);
  message.addRecipient("Alertee", address );

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

    WiFiClient client;

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