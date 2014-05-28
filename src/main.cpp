//
//  Angara.Net weather station sensor
//
//  DHT-22, BMP180, Wind vane, Anemometer, Arduino Leonardo, TinySine SIM900, IPS SKAT
//  http://github.com/maxp/rs_arw
//


#define VERSION "rs_arw 0.2"

#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <dht.h>

Adafruit_BMP085 bmp;
Dht dht;

#define BLINK_PIN 13
#define DHT22_PIN 6

#define VANE_PIN A0
#define ANEM_PIN A1
#define PWR_PIN  A2
#define BAT_PIN  A3

#define ANEM_INTERVAL 10
#define ANEM_ALEVEL 500
#define PWRBAT_ALEVEL 90

#define HOST      "rs.angara.net"
#define PORT       80
#define BASE_URI  "/dat?"

#define APN   ""
#define USER  ""
#define PASS  ""

// !!! 30
#define SEC10_NUM 2

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


// GSM.h:
//   GSM_TX    2
//   GSM_RX    3
// GSM.cpp:
//   GSM_RESET 9 (!!! TinySine sim900)
//   GSM_ON    8 (!!! TinySine sim900)

#include <SoftwareSerial.h>
#include "SIM900.h"
#include "inetGSM.h"

InetGSM inet;

#define IMEI_LEN 20

char tchar[10];
char imei[IMEI_LEN+1];


#define RBUFF_LEN 50
#define UBUFF_LEN 120

char rbuff[RBUFF_LEN];
char ubuff[UBUFF_LEN];

//

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
    // while (!Serial) {} // Leonardo

    Serial.println(VERSION);
    pinMode(BLINK_PIN, OUTPUT);

    digitalWrite(ANEM_PIN, HIGH);
    digitalWrite(VANE_PIN, HIGH);
    digitalWrite(PWR_PIN, HIGH);
    digitalWrite(BAT_PIN, HIGH);

    blink(4);
}


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
    for(int seconds=0; seconds < ANEM_INTERVAL; seconds++ ) 
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

// GSM

void sim_power() {
  pinMode(GSM_ON, OUTPUT);
  digitalWrite(GSM_ON, LOW);
  delay(1000);
  digitalWrite(GSM_ON, HIGH);
  delay(1300);
  digitalWrite(GSM_ON, LOW);
  delay(2500);
}

void sim_reset() {
  pinMode(GSM_RESET, OUTPUT);
  digitalWrite(GSM_RESET, LOW);
  delay(100);
  digitalWrite(GSM_RESET, HIGH);
  delay(200);
  digitalWrite(GSM_RESET, LOW);
  delay(100);
}

// #include "sms.h"

void gsm_send(char* buff) 
{
  gsm.begin(9600);
  
  // SMSGSM sms;
  // sms.SendSMS("+79025118669","test");
  // sms.SendSMS("7070","GO");
  
  int rc;
  rc = gsm.SendATCmdWaitResp("AT+GSN", 500, 100, "OK", 1);  // == 1
  
  int i = 0, j = 0; 
  byte c; 
  while((c = gsm.comm_buf[i]) && i < 1000) {
    if( c <= 32 ) { i++; } else { break; }
  }
  while( (c = gsm.comm_buf[i]) && (j < IMEI_LEN) ) {
    if( c <= 32 ) { break; }
    imei[j++] = c; i++;
  }
  imei[j] = 0;
  
  Serial.print("\nimei: "); Serial.println(imei);
 
  strcat(buff, "&hwid=");
  strcat(buff, imei);
  
  // TODO: rbuff = sha1.update(ubuff).update(secret).hexdigest()
  // strcat(ubuff, "&_hkey=");
  // strcat(ubuff, rbuff);

  inet.attachGPRS(APN, USER, PASS);
  delay(2000);

  gsm.SimpleWriteln("AT+CIFSR");
  delay(5000);

  Serial.print("\nurl: "); Serial.println(buff);
  
  inet.httpGET(HOST, PORT, buff, rbuff, 50); 
  Serial.print("\nresp: "); Serial.println(rbuff);
  
  sim_reset();  
}

// main loop

void loop()
{
    char f[20];

    cycle++;
    Serial.print("cycle: "); Serial.println(cycle);

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

        if(dht.read22(DHT22_PIN) == DHTLIB_OK)
        {
            t_sum += dht.temperature; t_count++;
            h_sum += dht.humidity;    h_count++;
        }

        // if(bmp.begin()) 
        // {
        //     p_sum += bmp.readPressure()/100.; p_count++;
        //     t0_sum += bmp.readTemperature(); t0_count++;
        // }
    }

    ubuff[0] = 0;
    itoa(cycle, f, 10); strcat(ubuff, "cycle="); strcat(ubuff, f);

    int pwr = 0;
    if( analogRead(PWR_PIN) > PWRBAT_ALEVEL) { pwr += 1; }; // pwr == 1: no external power
    if( analogRead(BAT_PIN) > PWRBAT_ALEVEL) { pwr += 2; }; // pwr == 2: battery failure
    if(pwr) { itoa(pwr, f, 10); strcat(ubuff, "&pwr="); strcat(ubuff, f); }

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
    if(w_count) { 
        dtostrf(w_sum/w_count, 3, 1, f); strcat(ubuff, "&w="); strcat(ubuff, f);
    }
    if(gust) { itoa(gust, f, 10); strcat(ubuff, "&g="); strcat(ubuff, f); }

    if(va_min || va_max) {
        strcat(ubuff, "&va_mm="); itoa(va_min, f, 10); strcat(ubuff, f);
        strcat(ubuff, ","); itoa(va_max, f, 10); strcat(ubuff, f);
    }

    if(w_sum || gust) 
    {
        int mpos = -1, mw = 0;
        for(int i=0; i<RHN; i++) { 
            if(rh[i] > mw) { mw = rh[i]; mpos = i; } 
        }
        if(mpos >= 0) 
        {
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
    

    Serial.print("ubuff: "); Serial.println(ubuff);

    blink(2);

    // gsm_send(ubuff);

}

//.
