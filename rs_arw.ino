//
//  Angara.Net weather station sensor
//
//  DHT-22, BMP180, Wind vane, Anemometer, Arduino Leonardo, TinySine SIM900, IPS SKAT
//  http://github.com/maxp/rs_arw
//


#define VERSION "rs_arw 0.5"

#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <dht.h>

Adafruit_BMP085 bmp;
dht dh;

// pin0,pin1 - hardware Serial1 (Leonardo)
// pin2,pin3 - SoftwareSerial (Pro)

#define BLINK_PIN    13
#define DHT22_PIN    6
#define GSM_PWR_PIN  8

#define VANE_PIN A0
#define ANEM_PIN A1
// #define PWR_PIN  A2
// #define BAT_PIN  A3

#define ANEM_INTERVAL 10
#define ANEM_ALEVEL   500
// #define PWRBAT_ALEVEL 90

#define HOST      "rs.angara.net"
#define PORT      "80"
#define BASE_URI  "/dat?"

#define APN   ""
#define USER  ""
#define PASS  ""

#define SEC10_NUM 30

// 203, 152, 352, 368, 489, 435, 806, 758
// 987, 929, 959, 855, 904, 620, 672, 186

#define RHN 16

int RH0[RHN] = {
    193, 120, 300, 361, 460, 400, 780, 700,
    970, 920, 940, 840, 880, 600, 650, 166
};

int RH1[RHN] = {
    250, 165, 360, 399, 500, 459, 839, 779,
    999, 939, 969, 879, 919, 649, 699, 192
};


int   cycle = 0;

float t_sum,   h_sum,   p_sum,   w_sum,   t0_sum; 
int   t_count, h_count, p_count, w_count, t0_count;
int   gust, va_min, va_max, rh[RHN];


#define RBUFF_LEN 80
#define UBUFF_LEN 220

// #define GsmPort  Serial1
SoftwareSerial GsmPort(2,3);

#define TCP_TIMEOUT 10
#define SEND_RETRY  3

#define APN  ""
#define USER ""
#define PASS ""

#define CSTT     "AT+CSTT=\""APN"\",\""USER"\",\""PASS"\""
#define CIPSTART "AT+CIPSTART=\"TCP\",\""HOST"\","PORT

#define IMEI_LEN 20

char imei[IMEI_LEN];

char rbuff[RBUFF_LEN];
char ubuff[UBUFF_LEN];


void blink(int n) 
{
    delay(200);
    for(int i=0; i<n; i++) {
        digitalWrite(BLINK_PIN, HIGH); delay(250);
        digitalWrite(BLINK_PIN, LOW);  delay(250);
    }
}

void setup()
{
    Serial.begin(9600);
    GsmPort.begin(9600);
    
    Serial.println(VERSION);
    pinMode(BLINK_PIN, OUTPUT);

    digitalWrite(ANEM_PIN, HIGH);
    digitalWrite(VANE_PIN, HIGH);
//    digitalWrite(PWR_PIN, HIGH);
//    digitalWrite(BAT_PIN, HIGH);
    
    imei[0] = 0;
}

// sensors

int read_vane() 
{
    int a = analogRead(VANE_PIN); 

    if(va_min == 0 || a < va_min) { va_min = a; }
    if(a > va_max) { va_max = a; }

    for( int i=0; i < RHN; i++ ) {
        if(RH0[i] <= a && a < RH1[i]) { return i; }
    }
    return -1;
}


int read_anem() 
{
    int count = 0;
    boolean high = false;
    boolean blink = false;
    for(int seconds=0; seconds < ANEM_INTERVAL; seconds++) 
    {
        digitalWrite(BLINK_PIN, HIGH);
        // one second cycle
        for(int dm=0; dm < 100; dm++) 
        {
            int v = analogRead(ANEM_PIN);
            if(v > ANEM_ALEVEL) {
                if(!high){ 
                    high = true; 
                    digitalWrite(BLINK_PIN, blink?HIGH:LOW); blink = !blink;
                }
            }
            else {
                if(high) { high = false; count++; }
            }
            delay(10);
        }
    }
    digitalWrite(BLINK_PIN, LOW);
    return count;
}

// gsm

void gsm_pwr()
{
  pinMode(GSM_PWR_PIN, OUTPUT); 
  digitalWrite(GSM_PWR_PIN, LOW ); delay(1000);
  digitalWrite(GSM_PWR_PIN, HIGH); delay(2000);
  digitalWrite(GSM_PWR_PIN, LOW ); delay(9000);
}

void gsm_flush() 
{
    Serial.print("drop: ");
    delay(100);
    while(GsmPort.available()){ 
        char c = GsmPort.read();
        Serial.print(c);
        delay(1);    
    }
    Serial.println();
}

void gsm_send(char* s) 
{
    GsmPort.print(s);
}

void gsm_cmd(char* s) 
{
    Serial.print("send: "); Serial.print(s); Serial.println();  
    gsm_send(s); gsm_send("\r");
}

int gsm_recv(int timeout_sec) 
{
    Serial.print("recv: ");
    int i=0;
    for(; i < RBUFF_LEN; i++ ) {
        char c = 0;
        for(int t=0; t < timeout_sec*1000; t++ ) {
            if(GsmPort.available()) { 
                c = GsmPort.read(); 
                Serial.print(c);
                break; 
            }
            else { delay(1); }
        }
        if(c == 0 || c == '\n') { break; } 
        rbuff[i] = c;     
    }
    if(i > 0 && rbuff[i-1] == '\r') { i--; }
    rbuff[i] = 0;
    Serial.println();
    return i;
}

int gsm_cmd_ok(char* s, int timeout)
{
    gsm_flush(); gsm_cmd(s);
    int n = gsm_recv(timeout);
    if(!n) { n = gsm_recv(timeout); } // skip empty line
    if(rbuff[0] == 'O' && rbuff[1] == 'K' && rbuff[2] == 0) { return 1; }
    else { return 0; }
}

int check_pwr() 
{
    // if(gsm_cmd_ok("AT", 3)) { return 1; }
    gsm_pwr();
    gsm_cmd_ok("ATE0", 1);
    if(gsm_cmd_ok("AT", 3)) { 
      Serial.println("gsm on.");
      return 1; 
    }
    Serial.println("gsm off.");
    return 0;
}


// read sensors

void collect_data() 
{
    char f[20];
 
    cycle++;
    Serial.print("\r\ncycle: "); Serial.println(cycle);

    t_sum = h_sum = p_sum = w_sum = t0_sum = 0.;
    t_count = h_count = p_count = w_count = t0_count = 0;

    for(int i=0; i<RHN; i++) { rh[i] = 0; }
    gust = va_min = va_max = 0;

    for(int s10=0; s10 < SEC10_NUM; s10++) 
    {
        int w = read_anem(); // delay 10 seconds
        if(w > gust) { gust = w; }
        w_sum += w; w_count++;

        int r = read_vane();
        if( r >= 0 ) {
            rh[r] += 1;
        }

        if(dh.read22(DHT22_PIN) == DHTLIB_OK)   
        {
            t_sum += dh.temperature; t_count++;
            h_sum += dh.humidity;    h_count++;
        }

        if(bmp.begin()) 
        {
            p_sum += bmp.readPressure()/100.; p_count++;
            t0_sum += bmp.readTemperature(); t0_count++;
        }
    }

    ubuff[0] = 0;
    itoa(cycle, f, 10); strcat(ubuff, "cycle="); strcat(ubuff, f);
    if(imei[0]) { strcat(ubuff, "&hwid="); strcat(ubuff, imei); }

//    int pwr = 0;
//    if( analogRead(PWR_PIN) > PWRBAT_ALEVEL) { pwr += 1; }; // pwr == 1: no external power
//    if( analogRead(BAT_PIN) > PWRBAT_ALEVEL) { pwr += 2; }; // pwr == 2: battery failure
//    if(pwr) { itoa(pwr, f, 10); strcat(ubuff, "&pwr="); strcat(ubuff, f); }

    if(t_count) { 
        dtostrf(t_sum/t_count, 3, 1, f); strcat(ubuff, "&t="); strcat(ubuff, f);
    }
    if(h_count) { 
        dtostrf(h_sum/h_count, 3, 1, f); strcat(ubuff, "&h="); strcat(ubuff, f);
    }
    if(p_count) { 
        dtostrf(p_sum/p_count, 3, 1, f); strcat(ubuff, "&p="); strcat(ubuff, f);
    }
    if(t0_count){ 
        dtostrf(t0_sum/t0_count, 3, 1, f); strcat(ubuff, "&t0="); strcat(ubuff, f);
    }
    
    // 10 seconds wind counters
    if(w_count) { 
        dtostrf(0.1*w_sum/w_count, 3, 1, f); strcat(ubuff, "&w="); strcat(ubuff, f);
    }
    if(gust) { 
        dtostrf(0.1*gust, 3, 1, f); strcat(ubuff, "&g="); strcat(ubuff, f); 
    }

    if(va_min || va_max) {
        strcat(ubuff, "&va_mm="); itoa(va_min, f, 10); strcat(ubuff, f);
        strcat(ubuff, ","); itoa(va_max, f, 10); strcat(ubuff, f);
    }

    if(w_sum || gust) 
    {
        int mpos = -1, mw = 0;
        for(int i=0; i < RHN; i++) { if(rh[i] > mw) { mw = rh[i]; mpos = i; } }
        if(mpos >= 0) {
            float m  = float(mw);
            float m0 = float(rh[(mpos-1+RHN) % RHN]);
            float m1 = float(rh[(mpos+1) % RHN]);

            int b = int((float(mpos)-(m0/m*0.5)+(m1/m*0.5))*22.5);
            strcat(ubuff, "&b="); itoa(b, f, 10); strcat(ubuff, f);

            strcat(ubuff, "&rh=");
            for(int i=0; i<RHN; i++) { 
                if(i) { strcat(ubuff, ","); }
                itoa(rh[i], f, 10); strcat(ubuff, f);
            }
        }
    }

    Serial.print("data: "); Serial.println(ubuff);
}

//

void loop()
{
    delay(100);
    blink(4);
    delay(200);

    collect_data();
    
    // send 
    
    blink(2);
    
    if( check_pwr() ) {
      
        for( int retry=0; retry < SEND_RETRY; retry++ )
        {
            if(!imei[0]) {
                gsm_cmd("AT+GSN");
                gsm_recv(1); gsm_recv(1);
                Serial.print("imei:"); Serial.println(rbuff);
                strncpy(imei, rbuff, 20);
                if(imei[0]) { strcat(ubuff,"&hwid="); strcat(ubuff, imei); }
            }

            // gsm_cmd_ok("at+cipshut", 3);
            gsm_cmd_ok(CSTT, 3); 
            gsm_cmd_ok("at+ciicr", 3);
            gsm_cmd_ok("at+cifsr", 3);
            gsm_cmd_ok(CIPSTART, 3);
            gsm_recv(TCP_TIMEOUT);
        
            if( strcmp(rbuff, "CONNECT OK") ) { Serial.println("connected"); }

            gsm_cmd_ok("at+cipsend", 3);
        
            gsm_send("GET "); gsm_send(BASE_URI); gsm_send(ubuff); gsm_send(" HTTP/1.0\r\n");
            gsm_send("Host: "); gsm_send(HOST); gsm_send("\r\n\r\n");
            gsm_send("\x1a"); // Ctrl-Z
        
            gsm_recv(TCP_TIMEOUT);
            gsm_recv(TCP_TIMEOUT);
            Serial.print("resp: "); Serial.println(rbuff);
            
            boolean send_ok = (strcmp(rbuff, "SEND OK") == 0);
            gsm_cmd_ok("at+cipshut", 3);
            
            if(send_ok) { Serial.println("send_ok."); break; }
        }        
        gsm_pwr();
    }
 
}

//.

