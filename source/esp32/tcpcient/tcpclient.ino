#include <WiFi.h>
#include <WebServer.h>   // Include the WebServer library

// https://einstronic.com/wp-content/uploads/2017/06/NodeMCU-32S-Catalogue.pdf

#define NUMCHANNELS 6
#define HEADERLEN 4
#define PACKETLEN (NUMCHANNELS * 2 + HEADERLEN + 1)

// Replace the next variables with your SSID/Password combination
const char* ssid        = "emg" ;
const char* password    = "emg" ;
const int data_port     = 8080 ;

long lastMsg = 0 ;
unsigned char TXBuf[PACKETLEN] ;  //The transmission packet

WebServer httpServer(80) ;
WiFiServer tcpServer(data_port) ;

void handleRoot() ;              // function prototypes for HTTP handlers
void handleNotFound() ;


void setup() 
{

    //Write packet header and footer
    TXBuf[0] = 0xa5;    //Sync 0
    TXBuf[1] = 0x5a;    //Sync 1
    TXBuf[2] = 2;       //Protocol version
    TXBuf[3] = 0;       //Packet counter
    TXBuf[4] = 0x02;    //CH1 High Byte
    TXBuf[5] = 0x00;    //CH1 Low Byte
    TXBuf[6] = 0x02;    //CH2 High Byte
    TXBuf[7] = 0x00;    //CH2 Low Byte
    TXBuf[8] = 0x02;    //CH3 High Byte
    TXBuf[9] = 0x00;    //CH3 Low Byte 
    TXBuf[10] = 0x02;   //CH4 High Byte
    TXBuf[11] = 0x00;   //CH4 Low Byte
    TXBuf[12] = 0x02;   //CH5 High Byte
    TXBuf[13] = 0x00;   //CH5 Low Byte
    TXBuf[14] = 0x02;   //CH6 High Byte
    TXBuf[15] = 0x00;   //CH6 Low Byte 
    TXBuf[2 * NUMCHANNELS + HEADERLEN] =  0x01;	// Switches state

    pinMode(LED_BUILTIN, OUTPUT) ;
    digitalWrite(LED_BUILTIN, HIGH) ;   // turn the LED on (HIGH is the voltage level)

    Serial.begin(115200) ;

    setup_wifi() ;

    httpServer.on("/", handleRoot);               // Call the 'handleRoot' function when a client requests URI "/"
    httpServer.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
    httpServer.begin() ;                           // Actually start the server
    Serial.println("HTTP server started") ;
    
    tcpServer.begin() ;
    Serial.println("TCP server started") ;

    digitalWrite(LED_BUILTIN, LOW) ;   // turn the LED on (HIGH is the voltage level)
}

void setup_wifi() 
{
    delay(10) ;
    // We start by connecting to a WiFi network
    Serial.println() ;
    Serial.print("Starting softAP...\n") ;
    Serial.println(ssid) ;

    WiFi.softAP(ssid, password) ;

    Serial.print("IP address: ") ;
    Serial.println(WiFi.softAPIP()) ;

}

void loop() 
{  
    WiFiClient client = tcpServer.available() ;
    if (client) 
    {
        // absolute priority to tcp client
        // transmit continously up to disconnection
        while (client.connected()) 
        {
            client.write(TXBuf,PACKETLEN) ;
            TXBuf[3] = TXBuf[3] + 1 ; // update packet counter
            delayMicroseconds( 50 ) ; // 20kHz sampling rate
        }
    }       
    // Handle HTTP requests
    httpServer.handleClient() ;    
}

void handleRoot() 
{
    String message ;
    message += "ESP32S EMG platform\n" ;  
    httpServer.send(200, "text/plain", message );   // Send HTTP status 200 (Ok) and send some text to the browser/client  
}

void handleNotFound()
{
    httpServer.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
