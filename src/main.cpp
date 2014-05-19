
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <dht.h>

Adafruit_BMP085 bmp;
Dht dht;

int bmp_ok = 0;

#define DHT22_PIN 6

#define VANE_PIN A0
#define ANEM_PIN A1

#define ANEM_INTERVAL 10
#define ANEM_ALEVEL 500

// !!! 30
#define SEC10_NUM 3

// 150, 108, 274, 197, 394, 346, 689, 668,
// 868, 809, 840, 735, 785, 512, 561, 135

#define RHN 16

int RH_A[RHN] = {
    140, 100, 260, 190, 390, 340, 680, 660,
    860, 800, 830, 730, 780, 500, 550, 120
};
#define RH_D 20


int   cycle = 0;

float t_sum,   h_sum,   p_sum,   w_sum,   t0_sum; 
int   t_count, h_count, p_count, w_count, t0_count;
int   gust, va_min, va_max;
int   rh[RHN];


void setup()
{
  Serial.begin(9600);
  Serial.println("rs_arw");
}


int read_vane() 
{
    int a = analogRead(VANE_PIN); 
    if(va_min == 0 || a < va_min) { va_min = a; }
    if(a > va_max) { va_max = a; }

    for( int i=0; i < RHN; i++ ) {
        if(RH_A[i] <= a && a < RH_A[i]+RH_D) { return i; }
    }
    return -1;
}


int read_anem() 
{
    int count = 0;
    boolean high = false;
    for(int seconds=0; seconds < ANEM_INTERVAL; seconds++ ) 
    {
        // one second cycle
        for(int dm=0; dm < 100; dm++) {
            int v = analogRead(ANEM_PIN);
            if(v > ANEM_ALEVEL) {
                if(!high){ high = true; }
            }
            else {
                if(high) { high = false; count++; }
            }
            delay(10);
        }
    }
    return count;
}


// read_bmp

// Connect VCC of the BMP085 sensor to 3.3V (NOT 5.0V!)
// Connect GND to Ground
// Connect SCL to i2c clock - on '168/'328 Arduino Uno/Duemilanove/etc thats Analog 5
// Connect SDA to i2c data - on '168/'328 Arduino Uno/Duemilanove/etc thats Analog 4
// SCL -> D3, SDA -> D2 on Lennardo


void loop()
{
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

            Serial.print("r: "); Serial.print(r); 
            Serial.print(" "); Serial.print(va_min); 
            Serial.print(" "); Serial.print(va_max); 
            Serial.println();
        }

        if(dht.read22(DHT22_PIN) == DHTLIB_OK)
        {
            t_sum += dht.temperature; t_count++;
            h_sum += dht.humidity;    h_count++;
        }

        if(bmp.begin()) 
        {
            p_sum += bmp.readPressure()/100.; p_count++;
            t0_sum += bmp.readTemperature(); t0_count++;

            Serial.print("pp: "); Serial.println(bmp.readPressure());
        }

    }

    if(t_count) { Serial.print("t: "); Serial.print(t_sum/t_count, 1); Serial.println(); }
    if(h_count) { Serial.print("h: "); Serial.print(h_sum/h_count, 1); Serial.println(); }
    if(p_count) { Serial.print("p: "); Serial.print(p_sum/p_count, 1); Serial.println(); }
    if(t0_count) { Serial.print("t0: "); Serial.print(t0_sum/t0_count, 1); Serial.println(); }
    if(w_count) { Serial.print("w: "); Serial.print(w_sum/w_count, 1); Serial.println(); }

    // send
   
}

//.
