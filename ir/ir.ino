/*
   ir to serial
   Author: Remi Sarrailh (madnerd.org)

   Forked from IRRecord
   Version 0.11 September, 2009
   Copyright 2009 Ken Shirriff
   http://arcfn.com
*/

#include <IRremote.h>

//Serial
const String usb_name = "ir:42007";
String serString;

//IR
const int RECV_PIN = 11;
IRrecv irrecv(RECV_PIN);
IRsend irsend;
decode_results results;

//Recorded IR
int codeType = -1; // The type of code
unsigned long codeValue; // The code value if not raw
unsigned int rawCodes[RAWBUF]; // The durations if raw
int codeLen; // The length of the code
int toggle = 0; // The RC5/6 toggle state

//Convert characters sent by serial to string
void serialManager() {
  while (Serial.available()) {
    delay(3);
    if (Serial.available() > 0) {
      char c = Serial.read();
      serString += c;
    }
  }
  if (serString.length() > 0) {
    if (serString == "/info") {
      Serial.println(usb_name);
    } else {
      codeType = getValue(serString,':', 0).toInt();
      String code_buf = getValue(serString,':',1);
      codeValue = getValue(code_buf,';',0).toInt();;
      codeLen = getValue(code_buf,';',1).toInt();
      Serial.print(codeType);
      Serial.print(":");
      Serial.print(codeValue);
      Serial.print(";");
      Serial.println(codeLen);
      sendCode(5);
    }
    serString = "";
  }
}

void setup()
{
  Serial.begin(115200);
  irrecv.enableIRIn(); // Start the receiver
}



// Stores the code for later playback
// Most of this code is just logging
void displayCode(decode_results *results) {
  codeType = results->decode_type;
  //Serial.print(codeType);
  //Serial.print(":");
  int count = results->rawlen;
  if (codeType == UNKNOWN) {
    Serial.print("RAW:");
    codeLen = results->rawlen - 1;
    // To store raw codes:
    // Drop first value (gap)
    // Convert from ticks to microseconds
    // Tweak marks shorter, and spaces longer to cancel out IR receiver distortion
    for (int i = 1; i <= codeLen; i++) {
      if (i % 2) {
        // Mark
        rawCodes[i - 1] = results->rawbuf[i] * USECPERTICK - MARK_EXCESS;
        Serial.print("m");
      }
      else {
        // Space
        rawCodes[i - 1] = results->rawbuf[i] * USECPERTICK + MARK_EXCESS;
        Serial.print("s");
      }
      Serial.print(rawCodes[i - 1], DEC);
    }
    Serial.println("");
  }
  else {
    if (codeType == NEC) {

      if (results->value == REPEAT) {
        // Don't record a NEC repeat value as that's useless.
        //Serial.println("repeat; ignoring.");
        return;
      } else {
        Serial.print(codeType, DEC);
        Serial.print(":");
      }
    }
    else if (codeType == SONY) {
      Serial.print(codeType, DEC);
      Serial.print(":");
    }
    else if (codeType == PANASONIC) {
      Serial.print(codeType, DEC);
      Serial.print(":");
    }
    else if (codeType == JVC) {
      Serial.print(codeType, DEC);
      Serial.print(":");
    }
    else if (codeType == RC5) {
      Serial.print(codeType, DEC);
      Serial.print(":");
    }
    else if (codeType == RC6) {
      Serial.print(codeType, DEC);
      Serial.print(":");
    }
    else {
      Serial.print(codeType, DEC);
      Serial.print(":");
    }
    Serial.print(results->value, DEC);
    Serial.print(";");
    Serial.println(results->bits);
    codeValue = results->value;
    codeLen = results->bits;
  }
}

//int codeType
//unsigned long codeVaue
//int codeLen
void sendCode(int repeat) {
  if (codeType == NEC) {
    if (repeat) {
      irsend.sendNEC(REPEAT, codeLen);
      //Serial.println("Sent NEC repeat");
    }
    else {
      irsend.sendNEC(codeValue, codeLen);
      Serial.print("Sent NEC ");
      Serial.println(codeValue, HEX);
    }
  }
  else if (codeType == SONY) {
    irsend.sendSony(codeValue, codeLen);
    //Serial.print("Sent Sony ");
    Serial.println(codeValue, HEX);
  }
  else if (codeType == PANASONIC) {
    irsend.sendPanasonic(codeValue, codeLen);
    //Serial.print("Sent Panasonic");
    Serial.println(codeValue, HEX);
  }
  else if (codeType == JVC) {
    irsend.sendPanasonic(codeValue, codeLen);
    //Serial.print("Sent JVC");
    Serial.println(codeValue, HEX);
  }
  else if (codeType == RC5 || codeType == RC6) {
    if (!repeat) {
      // Flip the toggle bit for a new button press
      toggle = 1 - toggle;
    }
    // Put the toggle bit into the code to send
    codeValue = codeValue & ~(1 << (codeLen - 1));
    codeValue = codeValue | (toggle << (codeLen - 1));
    if (codeType == RC5) {
      //Serial.print("Sent RC5 ");
      Serial.println(codeValue, HEX);
      irsend.sendRC5(codeValue, codeLen);
    }
    else {
      irsend.sendRC6(codeValue, codeLen);
      Serial.print("Sent RC6 ");
      Serial.println(codeValue, HEX);
    }
  }
  else if (codeType == UNKNOWN /* i.e. raw */) {
    // Assume 38 KHz
    irsend.sendRaw(rawCodes, codeLen, 38);
    //Serial.println("Sent raw");
  }
}


void loop() {
  serialManager();
  if (irrecv.decode(&results)) {
    displayCode(&results);
    irrecv.resume(); // resume receiver
  }
}


//Equivalent of explode in PHP (use for serial commands parsing)
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
