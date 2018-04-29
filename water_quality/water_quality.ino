
#include <Wire.h>                //enable I2C.

#include "MS5837.h"
#include "TSYS01.h"

MS5837 sensor;
TSYS01 t_sensor;


#define address 97               //default I2C ID number for EZO D.O. Circuit.



char computerdata[20];           //we make a 20 byte character array to hold incoming data from a pc/mac/other.
byte received_from_computer = 0; //we need to know how many characters have been received.
byte code = 0;                   //used to hold the I2C response code.
char DO_data[20];                //we make a 20 byte character array to hold incoming data from the D.O. circuit.
byte in_char = 0;                //used as a 1 byte buffer to store in bound bytes from the D.O. Circuit.
int time_ = 600;                 //used to change the delay needed depending on the command sent to the EZO Class D.O. Circuit.
float DO_float;                  //float var used to hold the float value of the DO.
char *DO;                        //char pointer used in string parsing.
char *sat;                       //char pointer used in string parsing.
float do_float;                  //float var used to hold the float value of the dissolved oxygen.
float sat_float;                 //float var used to hold the float value of the saturation percentage.


float pHValue = 0;
float voltage = 0;
float DOData = 0;
float Pressure = 0;
float Temp = 0;
float Turbidity = 0;


#define SensorPin A1            //pH meter Analog output to Arduino Analog Input 0
#define SensorPin2 A0            //turbity meter Analog output to Arduino Analog Input 0

#define Offset 0.00             //deviation compensate
#define LED 13
#define samplingInterval 40
#define printInterval 1000
#define DOInterval 2000
#define TPInterval 1000


#define ArrayLenth  50        //times of collection
int pHArray[ArrayLenth];      //Store the average value of the sensor feedback
int pHArrayIndex=0; 


union WATERDATA{
  float f_data[5];
  char c_data[20];
  }w;

void setup(void)
{
  pinMode(LED,OUTPUT);  
  Serial.begin(9600); 
  
  //Serial.println("pH meter experiment!");    //Test the Serial monitor
   Wire.begin();                //enable I2C port.

  sensor.init();
  t_sensor.init();
  sensor.setModel(MS5837::MS5837_30BA);
  sensor.setFluidDensity(997); // kg/m^3 (freshwater, 1029 for seawater)
}

void loop(void)
{
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static unsigned long DOTime = millis();
  static unsigned long TPTime = millis();
  //static float pHValue,voltage;
  
  /////////////////////////////////////
  //ph 
  if(millis()-samplingTime > samplingInterval)
  {
      pHArray[pHArrayIndex++]=analogRead(SensorPin);
     // Serial.println(analogRead(SensorPin));
      if(pHArrayIndex==ArrayLenth)pHArrayIndex=0;
      voltage = avergearray(pHArray, ArrayLenth)*5.0/1024;
      pHValue = 3.5*voltage+Offset;
      samplingTime=millis();

      int sensorValue = analogRead(SensorPin2);// read the input on analog pin 0:
      Turbidity = sensorValue *20*(5.0 / 1024.0); // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
      
      //Serial.println(voltage); 
  }

  ///////////////////////////////
  //dissolved oxygen
  if(millis() - DOTime > DOInterval)
  {
    data();
    DOTime = millis();
  }

  /////////////////////////////
  //imu  temperature pressure
  if(millis() - TPTime > TPInterval)
  {
    sensor.read();
    t_sensor.read();

    Pressure = sensor.pressure();
    float tt = t_sensor.temperature();
    if (isnan(tt) || isinf(tt) || abs(tt)>4294967040.0) Temp = 0; 
    else Temp = tt;
    
    //TPPrint();
    TPTime = millis();
  }
  
  
  
  
  
  //////////////////////////////
  //send out sensor data
   if(millis() - printTime > printInterval)   
  {  
     w.f_data[0] = pHValue;
     w.f_data[1] = DOData;
     w.f_data[2] = Pressure;
     w.f_data[3] = Temp;
     w.f_data[4] = Turbidity;
     

     char str[sizeof(w)+3];
     for(char i=0; i<20; i++)
     {
        str[i+2] = w.c_data[i];
      
     }
     str[0] = 0x0a;
     str[1] = 0x0b;
     str[sizeof(str)-1] = '\n';
     Serial.write(str, sizeof(str));
//     Serial.println(pHValue);
//     Serial.println(DOData);
//     Serial.println(Pressure);
//     Serial.println(Temp);
//     Serial.println(Turbidity);
//     Serial.println("________");
     
     //Serial.println(Temp,DEC);
     digitalWrite(LED,digitalRead(LED)^1);
     printTime=millis();
     

  }
}

double avergearray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount=0;
  if(number<=0){
    Serial.println("Error number for the array to avraging!/n");
    return 0;
  }
  if(number<5){   //less than 5, calculated directly statistics
    for(i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }else{
    if(arr[0]<arr[1]){
      min = arr[0];max=arr[1];
    }
    else{
      min=arr[1];max=arr[0];
    }
    for(i=2;i<number;i++){
      if(arr[i]<min){
        amount+=min;        //arr<min
        min=arr[i];
      }else {
        if(arr[i]>max){
          amount+=max;    //arr>max
          max=arr[i];
        }else{
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
}
void data()
{
  byte i = 0;
  Wire.beginTransmission(address);                                      //call the circuit by its ID number.
    Wire.write("R");                                             //transmit the command that was sent through the Serial port.
    Wire.endTransmission();                                               //end the I2C data transmission.

   delay(600);   
    Wire.requestFrom(address, 20, 1); //call the circuit and request 20 bytes (this may be more than we need)
    //Serial.println("bbb");
    code = Wire.read();  //the first byte is the response code, we read this separately.
    

  
    while (Wire.available()) {       //are there bytes to receive.
      //Serial.println("avail");
      
      in_char = Wire.read();         //receive a byte.
      DO_data[i] = in_char;          //load this byte into our array.
      i += 1;                        //incur the counter for the array element.
      if (in_char == 0) {            //if we see that we have been sent a null command.
        i = 0;                       //reset the counter i to 0.
        Wire.endTransmission();      //end the I2C data transmission.
        break;                       //exit the while loop.
      }
    }

    if (isDigit(DO_data[0])) {
      string_pars();                  //If the first char is a number we know it is a DO reading, lets parse the DO reading
    }
    else {                            //if it’s not a number
      //Serial.println(DO_data);        //print the data.
      for (i = 0; i < 20; i++) {      //step through each char
        DO_data[i] = 0;               //set each one to 0 this clears the memory
      }
    }
}

  
void string_pars() {                  //this function will break up the CSV string into its 2 individual parts, DO and %sat.
  byte flag = 0;                      //this is used to indicate is a “,” was found in the string array
  byte i = 0;                         //counter used for DO_data array.


  for (i = 0; i < 20; i++) {          //Step through each char
    if (DO_data[i] == ',') {          //do we see a ','
      flag = 1;                       //if so we set the var flag to 1 by doing this we can identify if the string being sent from the DO circuit is a CSV string containing tow values
    }
  }

  if (flag != 1) {                    //if we see the there WAS NOT a ‘,’ in the string array
//    Serial.print("DO:");              //print the identifier
//    Serial.println(DO_data);          //print the reading
    DOData = atof(DO_data);
  }

  if (flag == 1) {                    //if we see the there was a ‘,’ in the string array
    DO = strtok(DO_data, ",");        //let's pars the string at each comma
    sat = strtok(NULL, ",");          //let's pars the string at each comma
//    Serial.print("DO:");              //print the identifier
//    Serial.println(DO);               //print the reading
//    Serial.print("Sat:");             //print the identifier
//    Serial.println(sat);              //print the reading
    flag = 0;                         //reset the flag
  }
                                      
    /*                                //uncomment this section if you want to take the ASCII values and convert them into a floating point number.
    DO_float=atof(DO);
    sat_float=atof(sat);
   */ 
}







