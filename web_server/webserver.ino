#include <SPI.h>
#include <Ethernet2.h>
#include <SD.h>
 
// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   20
 
// MAC address from Ethernet shield sticker under board
byte mac[] = { 0x90, 0xA2, 0xDA, 0x10, 0x7D, 0x00 };
IPAddress ip(192, 168, 0, 99); // IP address, may need to change depending on network
EthernetServer server(80);  // create a server at port 80
//File webFile;
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char FileName[10];
char req_index = 0;              // index into HTTP_req buffer






char json[50] ;
void setup()
{     
    Serial.begin(9600);       // for debugging
    Serial1.begin(9600);
    Serial1.setTimeout(300); 
    // initialize SD card
    Serial.println(F("Init SD"));
    if (!SD.begin(4)) {
        Serial.println(F("ERROR SD "));
        return;    // init failed
    }
    Serial.println(F("SUC - SD"));
    // check for index.htm file
    if (!SD.exists("index.htm")) {
        Serial.println(F("no htm"));
        return;  // can't find index file
    }
    Serial.println(F("SUCCESS - Found index.htm file."));
     
    Ethernet.begin(mac, ip);  // initialize Ethernet device
    server.begin();           // start to listen for clients
    delay(1000);  
}
 
void loop()
{
  EthernetComm();
  SerialComm();

    
}
void EthernetComm()
{File webFile;

  
     EthernetClient client = server.available();  // try to get client
 
    if (client) {  // got client?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {   // client data available to read
                char c = client.read(); // read 1 byte (character) from client
                // buffer first part of HTTP request in HTTP_req array (string)
                // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
                if (req_index < (REQ_BUF_SZ - 1)) {
                    HTTP_req[req_index] = c;          // save HTTP request character
                    req_index++;
                }
                // print HTTP request character to serial monitor
                //Serial.print(c);
                // last line of client request is blank and ends with \n
                // respond to client only after last line received
                if (c == '\n' && currentLineIsBlank) {
                    // open requested web page file
                    findFileName(HTTP_req);
                    if (StrContains(HTTP_req, "GET / ")
                                 || StrContains(HTTP_req, "GET /index.htm")) {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-Type: text/html");
                        client.println("Connnection: close");
                        client.println();
                        webFile = SD.open("index.htm");        // open web page file
                    }
                    else if (StrContains(HTTP_req, "GET /jquery.js")) {
                      Serial.println("js");
                        webFile = SD.open("jquery.js");
                        if (webFile) {
                          
                            client.println("HTTP/1.1 200 OK");
                            client.println("Content-Type: text/javascript");
                            client.println();
                        }
                    }

                    else if (StrContains(HTTP_req, "GET /waterquality")){
                       Serial.println("water");
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-Type: text/html");
                        client.println("Server: Arduino");
                        client.println("Connnection: close");
                        client.println();
                        
                        client.println("<!DOCTYPE HTML>");
                        //client.println("<html>");
                        client.println(json);
                        //client.println("</html>");
//                        client.println("<head>");
//                        client.println("head");
//                        client.println("</head>");
                        
//                        client.println("<body>");
//                        client.println("sbsvsvsvsvsvsvsvsvsvsvs");
//                        client.println("</body>");
//                        client.println("</html>");
                           
                        

                    }

                    if (webFile) {
                        while(webFile.available()) {
                            client.write(webFile.read()); // send web page to client
                        }
                        webFile.close();
                    }
                    // reset buffer index and all buffer elements to 0
                    req_index = 0;
                    StrClear(HTTP_req, REQ_BUF_SZ);
                    break;
                }
                // every line of text received from the client ends with \r\n
                if (c == '\n') {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                } 
                else if (c != '\r') {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
            } // end if (client.available())
        } // end while (client.connected())
        delay(1);      // give the web browser time to receive the data
        client.stop(); // close the connection
    }
}
 void SerialComm()
 {
     union WATERDATA{
  float f_data[5];
  char c_data[20];
  }w;
    if (Serial1.available()>=23)
  {
    //Serial.println("ready!");
    if(Serial1.peek() == 0x0a)
    {
      //Serial.println("first byte!");
      char c = (char)Serial1.read();
      if((char)Serial1.read() == 0x0b)
      {
        //Serial.println("second byte!");
        
        Serial1.readBytes(w.c_data, 20);
        Serial1.read();
        

        //sprintf(json, "{\"one\":%.2f, \"two\":%.2f, \"three\":%.2f, \"four\":%.2f, \"five\":%.2f}", w.f_data[0],w.f_data[1],w.f_data[2],w.f_data[3],w.f_data[4],w.f_data[5] );
        //sprintf(json, "%.2f,%.2f,%.2f,%.2f,%.2f", w.f_data[0],w.f_data[1],w.f_data[2],w.f_data[3],w.f_data[4]);
        String str = String(w.f_data[0], 2)+ ',' + String(w.f_data[1], 2)+','+ String(w.f_data[2], 2)+','+ String(w.f_data[3], 2)+','+ String(w.f_data[4], 2);
        //Serial.println(str.c_str());
        strcpy(json, str.c_str());
        Serial.println(json);
      }
      
      else 
        while (Serial1.available())
        {
          Serial1.read();
        }
     }
     while (Serial1.available())
     {
        Serial1.read();
     }
  }
 }
// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}
 
// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
void findFileName(char *str1)  {
   
  char *fname;
   
   
  if (strstr (str1, "GET / ") != 0) {
    // send a standard http response header ------ a page to display has been found  
  }
  else if (strstr (str1, "GET /") != 0) {
          fname = str1 + 5; // look after the "GET /" (5 chars)
          // a little trick, look for the " HTTP/1.1" string and 
          // turn the first character of the substring into a 0 to clear it out.
          (strstr (str1, " HTTP"))[0] = 0;
          // print the file we want
          Serial.print("Filename is ");
          Serial.println(fname); 
          Serial.print("Extension is ");
          Serial.println(get_filename_ext(fname));     
  }
}
 
const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}
 
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;
 
    len = strlen(str);
     
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }
 
    return 0;
}
