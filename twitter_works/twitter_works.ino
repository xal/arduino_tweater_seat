// include SIM900 GSM libraries
#include "SIM900.h"
#include <SoftwareSerial.h>
#include "inetGSM.h"

// buffers size 
#define MESSAGE_BUFFER_FOR_INET_RESPONSE_SIZE 50
#define MESSAGE_BUFFER_FOR_IN_SERIAL_SIZE 50
#define MESSAGE_BUFFER_FOR_SPRINTF_SIZE 128
// buad rate for GSM module. For http gprs use - 2400
#define GSM_BAUD_RATE 2400

// libarary for sprinft
#include <stdarg.h>

InetGSM gprsInetWrapper;

char bufferForSPrintf[MESSAGE_BUFFER_FOR_SPRINTF_SIZE];
char messageBuffertForInetResponse[MESSAGE_BUFFER_FOR_INET_RESPONSE_SIZE];

int fsrAnalogInputPin = 0; // FSR is connected to analog A0
int fsrValue;      // readed value from input

boolean isNeedSendTweet = false;
int counterSeatingTick = 0;
int counterLoopTick = 0;
int numberSeatingForDetectStandUp = 50;
int pushPowerForSendTweet = 200;

char * sprintf(char *fmt, ... ){
        va_list args;
        va_start (args, fmt );
        vsnprintf(bufferForSPrintf, MESSAGE_BUFFER_FOR_SPRINTF_SIZE, fmt, args);
        va_end (args);
}

boolean startGSM(int baudrate) 
{

  boolean started = false;
  //Start configuration of shield with baudrate.
  //For http uses is raccomanded to use 4800 or slower.
  if (gsm.begin(baudrate)){
    //enable AT echo 
    Serial.println("\nstatus=READY");
    started = true;  
  } else  {
    Serial.println("\nstatus=IDLE");    
    started = false;  
  }

  return started;

}


boolean attachGPRS(char * accessPoint, char * login, char * password) 
{

  boolean attached = false;
   //GPRS attach, put in order APN, username and password.
    //If no needed auth let them blank.
    if (gprsInetWrapper.attachGPRS(accessPoint, login, password)) {
      attached = true;
      Serial.println("status=ATTACHED");
    } else  {
      attached = false;
      Serial.println("status=ERROR");
    }
    delay(1000);
  return attached;

}

boolean retrieveIP() 
{

    //Read IP address.
    gsm.SimpleWriteln("AT+CIFSR");
    delay(5000);
    //Read until serial buffer is empty.
    gsm.WhileSimpleRead();    

}

void setup() 
{
  
  //Serial connection.
  Serial.begin(9600); 
  Serial.println("GSM Shield testing.");
  
  boolean started = false;
 
  started = startGSM(GSM_BAUD_RATE);
  
  if(started){
   
    boolean attached = attachGPRS("internet.wind", "", "");

    if(attached) {
      retrieveIP();
    }
  }

};

void doGSMModuleWork() {

  //Read for new byte on serial hardware,
  //and write them on NewSoftSerial.
   serialhwread();
  //Read for new byte on NewSoftSerial.
   serialswread();
}

void loop() 
{

  counterLoopTick++;

  // Every tick do gsm work if need
  doGSMModuleWork();
   
  // every 1000 ticks read and analyze fsr value. 
  // if need gsm job - we do it in 1000 ticks in any case, 
  // if there are no gsm work we listen fsr
  if(counterLoopTick > 1000) {
     
    counterLoopTick = 0;
   
    fsrValue = analogRead(fsrAnalogInputPin);
  
    // Case if sit down on chair
    if((fsrValue > pushPowerForSendTweet) && (isNeedSendTweet == false)) {

      isNeedSendTweet = true;
      counterSeatingTick = 0;

      int weight = fsrValue - pushPowerForSendTweet;

      sprintf("status=Twitter chair #izolab weight is %d", weight);
      int numdata;

      Serial.println(bufferForSPrintf);
      
      numdata=gprsInetWrapper.httpPOST("api.supertweet.net", 80, "/1.1/statuses/update.json", bufferForSPrintf, messageBuffertForInetResponse, MESSAGE_BUFFER_FOR_INET_RESPONSE_SIZE);
     
      Serial.println("\nNumber of data received:");
      Serial.println(numdata);  
      Serial.println("\nData received:"); 
      Serial.println(messageBuffertForInetResponse); 
    } 
  
    // Case if any already seating
    if(fsrValue>pushPowerForSendTweet) {
      counterSeatingTick = 0;
    }
    
    // If no seating
    if((fsrValue <= pushPowerForSendTweet) && (isNeedSendTweet == true)) {
      counterSeatingTick++;
      Serial.print("(isNeedSendTweet == true), fsrValue = ");
      Serial.print(fsrValue);
      Serial.print(" counterSeatingTick = ");
      Serial.println(counterSeatingTick);  
      
      // if anybody stand up
      if(counterSeatingTick >= numberSeatingForDetectStandUp) {
         isNeedSendTweet = false;
         Serial.print("(isNeedSendTweet == false), fsrValue = ");
         Serial.print(fsrValue);
         Serial.print(" counterSeatingTick = ");
         Serial.println(counterSeatingTick);
         counterSeatingTick = 0;
      }
    }
    
    // Print debug info
    if(isNeedSendTweet == true) {
      Serial.print("(isNeedSendTweet == true), fsrValue = ");
      Serial.println(fsrValue);
    } else {
      Serial.print("(isNeedSendTweet == false), fsrValue = ");
      Serial.println(fsrValue);
    }  
  } 
};

void serialhwread(){
  
  char inSerial[MESSAGE_BUFFER_FOR_IN_SERIAL_SIZE];
  int i = 0;
  i=0;
  if (Serial.available() > 0){            
    while (Serial.available() > 0) {
      inSerial[i]=(Serial.read());
      delay(10);
      i++;      
    }
    
    inSerial[i]='\0';
    if(!strcmp(inSerial,"/END")){
      Serial.println("_");
      inSerial[0]=0x1a;
      inSerial[1]='\0';
      gsm.SimpleWriteln(inSerial);
    }
    //Send a saved AT command using serial port.
    if(!strcmp(inSerial,"TEST")){
      Serial.println("SIGNAL QUALITY");
      gsm.SimpleWriteln("AT+CSQ");
    }
    //Read last message saved.
    if(!strcmp(inSerial,"MSG")){
      Serial.println(messageBuffertForInetResponse);
    }
    else{
      Serial.println(inSerial);
      gsm.SimpleWriteln(inSerial);
    }    
    inSerial[0]='\0';
  }
}

void serialswread(){
  gsm.SimpleRead();
}

