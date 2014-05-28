



#define RBUFF_LEN 80
#define UBUFF_LEN 220

#define GSM_PWR_PIN  8

#define GsmPort  Serial1

#define HOST "rs.angara.net"
#define PORT "80"

#define TCP_TIMEOUT 10

#define APN  "internet"
#define USER ""
#define PASS ""


#define CSTT "AT+CSTT=\""APN"\",\""USER"\",\""PASS"\""
#define CIPSTART "AT+CIPSTART=\"TCP\",\""HOST"\","PORT


char rbuff[RBUFF_LEN];
char ubuff[UBUFF_LEN];


void setup()
{
    Serial.begin(9600);
    GsmPort.begin(9600);
    
    pinMode(13, OUTPUT); 
    digitalWrite(13, HIGH); delay(3000); 
    digitalWrite(13, LOW); delay(2000);
}


void gsm_pwr()
{
  pinMode(GSM_PWR_PIN, OUTPUT); 
  digitalWrite(GSM_PWR_PIN, LOW ); delay(1000);
  digitalWrite(GSM_PWR_PIN, HIGH); delay(2000);
  digitalWrite(GSM_PWR_PIN, LOW ); delay(9000);
}

void gsm_flush() 
{
    Serial.print("flush:{");
    delay(100);
    while(GsmPort.available()){ 
        char c = GsmPort.read();
        Serial.print(c);
        delay(1);    
    }
    Serial.println("}.");
}


void gsm_send(char* s) 
{
    GsmPort.print(s);
}

void gsm_cmd(char* s) 
{
    Serial.print("send:"); Serial.print(s); Serial.println();  
    gsm_send(s); gsm_send("\r");
}

int gsm_recv(int timeout_sec) 
{
    int i=0;
    for(; i < RBUFF_LEN; i++ ) {
        unsigned char c = 0;
        for(int t=0; t < timeout_sec*1000; t++ ) {
            if(GsmPort.available()) { 
                c = GsmPort.read(); 
                Serial.print(" *"); Serial.print((int)c); Serial.print(" ");
                break; 
            }
            else { delay(1); }
        }
        if(c == 0 || c == '\n') { break; } 
        rbuff[i] = c;     
    }
    if(i > 0 && rbuff[i-1] == '\r') { i--; }
    rbuff[i] = 0;
    return i;
}


int gsm_cmd_ok(char* s, int timeout)
{
    gsm_flush(); gsm_cmd(s);
    int n = gsm_recv(timeout);
    if(!n) { n = gsm_recv(timeout); } // skip empty line
    
    Serial.print("recv:"); Serial.print((char*)rbuff); Serial.println();
    
    if(rbuff[0] == 'O' && rbuff[1] == 'K' && rbuff[2] == 0) { 
        Serial.println("got ok");
        return 1; 
    }
    else { return 0; }
}


int check_pwr() 
{
    if(gsm_cmd_ok("AT", 3)) { return 1; }
    gsm_pwr();
    gsm_cmd_ok("ATE0", 1);
    if(gsm_cmd_ok("AT", 3)) { return 1; }
    return 0;
}

void loop()
{
  
    // collect_data(); -> ubuff
    
    
    strcpy(ubuff, "cycle=999");

    // gprs
    
    if( check_pwr() ) {
        Serial.println("pwr_ok");  
        
        gsm_cmd("AT+GSN");
        gsm_recv(1); gsm_recv(1);
        Serial.print("imei:"); Serial.println(rbuff);

        strcat(ubuff,"&hwid="); strncat(ubuff, rbuff, 20);

        Serial.print("ubuff:"); Serial.println(ubuff);
        delay(3000);
        
        gsm_cmd_ok("at+cipshut", 3);
        gsm_cmd_ok(CSTT, 3); 
        gsm_cmd_ok("at+ciicr", 3);
        gsm_cmd_ok("at+cifsr", 3);
        gsm_cmd_ok(CIPSTART, 3);

        gsm_recv(TCP_TIMEOUT);
        
        if( strcmp(rbuff, "CONNECT OK") ) {
            Serial.println("connected");
        }

        gsm_cmd_ok("at+cipsend", 3);
        
        gsm_send("GET /dat?"); gsm_send(ubuff); gsm_send(" HTTP/1.0\r\n");
        gsm_send("Host: "); gsm_send(HOST); gsm_send("\r\n\r\n");
        gsm_send("\x1a"); // Ctrl-Z
        
        gsm_recv(TCP_TIMEOUT);
        gsm_recv(TCP_TIMEOUT);
        Serial.print("response:"); Serial.println(rbuff);
  
        gsm_cmd_ok("at+cipshut", 3);
        // at+cgatt=0
        

        delay(8000);
        
        // while(1) { commands(); }
        
    }
    else {
        Serial.println("no power");
        delay(1000);
    }
    
}


// +HTTPINIT
// AT+HTTPPARA="URL","www.google.com"
// AT+HTTPACTION=0
// AT+HTTPREAD

/*

  at+cipstatus
  
  at+cipshut
  at+cstt="internet.mts.ru","mts","mts"
  at+ciicr
  at+cifsr
  
  at+cipstart="TCP","rs.angara.net",80
  
  
  CONNECT OK
  
  at+cipsend
  
  at+cgatt=0
  
  
  AT+CSTT="internet"
  AT+CIICR
  AT+CIFSR
  AT+CIPSPRT=0
  
  AT+CIPSTART="tcp","rs.angara.net","80"
  
  = OK
  = CONNECT OK
  
  AT+CIPSEND
  
  GET /dat?..... HTTP/1.0\n
  Host: rs.angara.net\n
  \n
  ^Z
  
  AT+CIPCLOSE

  
  mySerial.println("AT+CGATT?");
  delay(1000);
 
  ShowSerialData();
 
  mySerial.println("AT+CSTT=\"CMNET\"");//start task and setting the APN,
  delay(1000);
 
  ShowSerialData();
 
  mySerial.println("AT+CIICR");//bring up wireless connection
  delay(3000);
 
  ShowSerialData();
 
  mySerial.println("AT+CIFSR");//get local IP adress
  delay(2000);
 
  ShowSerialData();
 
  mySerial.println("AT+CIPSPRT=0");
  delay(3000);
 
  ShowSerialData();
 
  mySerial.println("AT+CIPSTART=\"tcp\",\"api.cosm.com\",\"8081\"");//start up the connection
  delay(2000);
 
  ShowSerialData();
 
  mySerial.println("AT+CIPSEND");
  delay(4000);
  ShowSerialData();
  String humidity = "1031";//these 4 line code are imitate the real sensor data, because the demo did't add other sensor, so using 4 string variable to replace.
  String moisture = "1242";//you can replace these four variable to the real sensor data in your project
  String temperature = "30";//
  String barometer = "60.56";//
  mySerial.print("{\"method\": \"put\",\"resource\": \"/feeds/42742/\",\"params\"");//here is the feed you apply from pachube
  delay(500);
  ShowSerialData();
  mySerial.print(": {},\"headers\": {\"X-PachubeApiKey\":");//in here, you should replace your pachubeapikey
  delay(500);
  ShowSerialData();
  mySerial.print(" \"_cXwr5LE8qW4a296O-cDwOUvfddFer5pGmaRigPsiO0");//pachubeapikey
  delay(500);
  ShowSerialData();
  mySerial.print("jEB9OjK-W6vej56j9ItaSlIac-hgbQjxExuveD95yc8BttXc");//pachubeapikey
  delay(500);
  ShowSerialData();
  mySerial.print("Z7_seZqLVjeCOmNbEXUva45t6FL8AxOcuNSsQS\"},\"body\":");
  delay(500);
  ShowSerialData();
  mySerial.print(" {\"version\": \"1.0.0\",\"datastreams\": ");
  delay(500);
  ShowSerialData();
  mySerial.println("[{\"id\": \"01\",\"current_value\": \"" + barometer + "\"},");
  delay(500);
  ShowSerialData();
  mySerial.println("{\"id\": \"02\",\"current_value\": \"" + humidity + "\"},");
  delay(500);
  ShowSerialData();
  mySerial.println("{\"id\": \"03\",\"current_value\": \"" + moisture + "\"},");
  delay(500);
  ShowSerialData();
  mySerial.println("{\"id\": \"04\",\"current_value\": \"" + temperature + "\"}]},\"token\": \"lee\"}");
 
 
  delay(500);
  ShowSerialData();
 
  mySerial.println((char)26);//sending
  delay(5000);//waitting for reply, important! the time is base on the condition of internet 
  mySerial.println();
 
  ShowSerialData();
 
  mySerial.println("AT+CIPCLOSE");//close the connection
  delay(100);
  ShowSerialData();
*/
