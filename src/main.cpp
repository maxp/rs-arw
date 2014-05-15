
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
#define ANEM_LEVEL 400

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
    boolean high = false;
    int count = 0;
    for(int second=0; second < ANEM_INTERVAL; second++ ) {
        for(int dm=0; dm < 10; dm++) {
            int v = analogRead(ANEM_PIN);
            Serial.print("a:"); Serial.print(v); Serial.println();

            if(v > ANEM_LEVEL) {
                if(!high){ high = true; count++; }
            }
            else {
                if(high) { high = false; }
            }

            delay(100);
        }
    }
}


void loop()
{
  // READ DATA
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

  delay(1000);

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
    
    // Calculate altitude assuming 'standard' barometric
    // pressure of 1013.25 millibar = 101325 Pascal
    Serial.print("Altitude = ");
    Serial.print(bmp.readAltitude());
    Serial.println(" meters");

  // you can get a more precise measurement of altitude
  // if you know the current sea level pressure which will
  // vary with weather and such. If it is 1015 millibars
  // that is equal to 101500 Pascals.
    Serial.print("Real altitude = ");
    Serial.print(bmp.readAltitude(101500));
    Serial.println(" meters");
    
    Serial.println();
    delay(1000);

}

//.




