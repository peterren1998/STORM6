#include <SoftwareSerial.h>
#include <SerialCommand.h>

#include <Wire.h>

// Address that comes with the valves

#define DEFAULT_ADDRESS 0x0E

// Adresses that we'll give to each of the valves. Valves are
// numbered from right to left, looking at the valves from the
// side that the connections and arduino are visible. the Hex
// numbers are 0x0E = 14, 0x18 = 24, 0x1A = 26 and 0x1C = 28
#define VALVE_ADDRESS_1 0x18
#define VALVE_ADDRESS_2 0x1A
#define VALVE_ADDRESS_3 0x1C

SerialCommand serialCommand;

bool sendValveCommand(byte address, byte command, byte value) {
  byte success = 1;
  byte attempts = 0;

  while (success != 0 && attempts < 50) {
    //Wire.beginTransmission(address);
    Wire.beginTransmission(address >> 1);
    delay(2);
    Wire.write(command);
    Wire.write(value);
    Wire.write(address ^ command ^ value);
    success = Wire.endTransmission(); //return 0 if success
    attempts++;
  }

  if (attempts >= 50) {
    Serial.print("Waring: failed in sending valve command ");
    Serial.println(address, HEX);
    return false;
    
  }
  else {
    //Serial.print("Success sending valve command ");
    //Serial.println(address, HEX);
    return true;
    
  }
}

int setValveAddress(byte oldAddress, byte newAddress) {
 return sendValveCommand(oldAddress, 'N', newAddress);
}

int setValvePosition(byte address, byte newPosition) {
  return sendValveCommand(address, 'P', newPosition);
}

int setValveCommandMode(byte address, byte commandMode) {
  return sendValveCommand(address, 'F', commandMode);
}

int setValveProfile(byte address, byte newProfile) {
  return sendValveCommand(address, 'O', newProfile);
}

int homeValve(byte address) {
  return sendValveCommand(address, 'M', 0x01);
}

int getValveResponse(byte address, byte command) {
  bool success = sendValveCommand(address, command, 0x01);
  if (!success) {
    return -1;
  }
  Wire.requestFrom(address >> 1, 2);
  byte r1 = 0;
  byte r2 = 0;
  int wireStatus = Wire.available();
  
  if (Wire.available() <= 2) {
    r1 = Wire.read();
    r2 = Wire.read();
  }
  else {
    return -1;
  }

  return r1;
}

int getValveStatus(byte address) {
  return getValveResponse(address, 'S');
}

void searchValveAddress(int start) {
  for (int addr = start; addr < 36; addr++) {
      Serial.print(addr,HEX);
      Serial.print(" ");
      Serial.print(getValveStatus(addr));
      Serial.println(" ");
  }
}

void setPosition(int newPosition) {
  if (newPosition < 0 || newPosition > 34) {
    return;
  }

  if (newPosition < 12) {
    setValvePosition(VALVE_ADDRESS_1, newPosition);
  }
  else if (newPosition < 23) {
    setValvePosition(VALVE_ADDRESS_1, 12);
    setValvePosition(VALVE_ADDRESS_2, newPosition - 11);
  }
  else {
    setValvePosition(VALVE_ADDRESS_1, 12);
    setValvePosition(VALVE_ADDRESS_2, 12);
    setValvePosition(VALVE_ADDRESS_3, newPosition - 22);
  }
}

void processMove() {
  int newPosition;
  char *arg;
  
  
  arg = serialCommand.next();
  if (arg != NULL) {
    newPosition = atoi(arg);
    //Serial.print("new position ");
    //Serial.println(newPosition);
    setPosition(newPosition);
  }
}

bool isErrorStatus(int inStatus) {
  if (inStatus == 99 || inStatus == 88 || inStatus == 77 ||
      inStatus == 66 || inStatus == 55 || inStatus == 44 ||
      inStatus == -1) {
    return true;
  }
  return false;
}

void processPositionRequest() {
  int status1 = getValveStatus(VALVE_ADDRESS_1);
  int status2 = getValveStatus(VALVE_ADDRESS_2);
  int status3 = getValveStatus(VALVE_ADDRESS_3);
  //plot status if debugging:
  byte outputStatus = 0;
  if (outputStatus) {
      Serial.print("status: ");
      Serial.print(status1);
      Serial.print(" ");
      Serial.print(status2);
      Serial.print(" ");
      Serial.print(status3);
      Serial.println(" ");
  }
  
  if (!isErrorStatus(status1) && !isErrorStatus(status2) &&
      !isErrorStatus(status3)) {
    if (status1 != 12) {
      Serial.print("P ");
      Serial.println(status1);
    }
    else if (status2 != 12) {
      Serial.print("P ");
      Serial.println(status2 + 11);
    }
    else {
      Serial.print("P ");
      Serial.println(status3 + 22);
    }
  }
  else {
    Serial.println("P!");
  }
}

void processPortCountRequest() {
  Serial.println("N 34");
}

void processUnrecognized(const char *) {
  Serial.println("Unrecognized command");
}

// function to initialize the valves. This should be used once,
// during the setup of the system. Start with VALVE_ADDRESS_1,
// and continue with _2 and _3, following the convention described
// at the beggining of the file.
void updateValveAddress() {
  Serial.println("updating valve address");
  //setValveAddress(DEFAULT_ADDRESS, VALVE_ADDRESS_1);
  //setValveAddress(DEFAULT_ADDRESS, VALVE_ADDRESS_2);
  //setValveAddress(DEFAULT_ADDRESS, VALVE_ADDRESS_3);
  processPositionRequest(); // check current status
}

void setup() {
  Wire.begin();
  Wire.setClock(10000);
  Serial.begin(9600);
  delay(100);
  
  
  // To be used only once during the first system setup.
  // comment afterwards.
  //updateValveAddress();
  // To be used for debugging, search all possible addresses
  //searchValveAddress(8); 
  
  serialCommand.addCommand("P", processMove);
  serialCommand.addCommand("P?", processPositionRequest);
  serialCommand.addCommand("N?", processPortCountRequest);
  serialCommand.setDefaultHandler(processUnrecognized);
}

void loop() {
  serialCommand.readSerial();
}
