
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

// int anem_counter = 0;

void setup()
{
  Serial.begin(9600);
  Serial.println("rs_arw");
  if (!bmp.begin()) {
    Serial.println("!BMP");
  }
  else { bmp_ok = 1; }
}


int read_vane() 
{
    return analogRead(VANE_PIN);
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


void read_dht22()
{
  Serial.print("DHT: ");
  int chk = dht.read22(DHT22_PIN);
  switch (chk)
  {
    case DHTLIB_OK:  
        Serial.print("OK,\t"); 
        break;
    case DHTLIB_ERROR_CHECKSUM: 
        Serial.print("Checksum error,\t"); 
        break;
    case DHTLIB_ERROR_TIMEOUT: 
        Serial.print("Time out error,\t"); 
        break;
    default: 
        Serial.print("Unknown error,\t"); 
        break;
  }
  // DISPLAY DATA
  Serial.print(" t=");  
  Serial.print(dht.temperature, 1);
  Serial.print("  h=");  
  Serial.print(dht.humidity, 1);
  Serial.println();  
}

void read_bmp()
{
  // read_bmp

  // Connect VCC of the BMP085 sensor to 3.3V (NOT 5.0V!)
  // Connect GND to Ground
  // Connect SCL to i2c clock - on '168/'328 Arduino Uno/Duemilanove/etc thats Analog 5
  // Connect SDA to i2c data - on '168/'328 Arduino Uno/Duemilanove/etc thats Analog 4
  // SCL -> D3, SDA -> D2 on Lennardo

    Serial.print("bmp_ok: "); Serial.print(bmp_ok);
    Serial.println();

    Serial.print("Temperature = ");
    Serial.print(bmp.readTemperature());
    Serial.println(" *C");
    
    Serial.print("Pressure = ");
    Serial.print(bmp.readPressure());
    Serial.println(" Pa");
    
    Serial.println();
}


void loop()
{
/*
    read_dht22();
    read_bmp();

    int w = read_anem();
    Serial.print("w: "); Serial.print(w); Serial.println();
*/    
    int va = read_vane();
    Serial.print("va: "); Serial.print(va); Serial.println();
    delay(10);
}

//.
