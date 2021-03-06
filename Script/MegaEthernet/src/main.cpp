// ------------------------------------------------------------ System ------------------------------------------------------------
#include <Arduino.h>
#include <MemoryFree.h>


// ---------------------------------------- Dobby ----------------------------------------
#include <DobbyLib.h>

// FIX - This sets the hostname, move it to somewhare else
DobbyLib DL("NotConfigured");

#define Version 1101004
// First didget = Software type 1-Production 2-Beta 3-Alpha
// Secound = Board
//  Board types:
//  0 = Wemos D1 Mini
//  1 = Arduino Mega 2650
// Third and Fourth didget = Major version number
// Fifth to seventh didget = Minor version number


// ------------------------------------------------------------ json ------------------------------------------------------------
#include <ArduinoJson.h>


// ------------------------------------------------------------ Log() ------------------------------------------------------------
// http://github.com/MBojer/MB_Queue
#include <MB_Queue.h>
// FIX - Might need to add support for log full handling
#define Log_Max_Queue_Size 50
MB_Queue Log_Queue_Topic(Log_Max_Queue_Size);
MB_Queue Log_Queue_Log_Text(Log_Max_Queue_Size);

// Log Levels
// 0 = Debug
// 1 = Info
// 2 = Warning
// 3 = Error
// 4 = Fatal
#define Log_Level_DEBUG 0
#define Log_Level_INFO 1
#define Log_Level_WARNING 2
#define Log_Level_ERROR 3
#define Log_Level_FATAL 4

byte Log_Level = 0;


// ------------------------------------------------------------ Ammeter - ACS712 ------------------------------------------------------------
// https://github.com/rkoptev/ACS712-arduino
#include "ACS712.h"

#define ACS712_Max_Number_Of 2
#define ACS712_Number_1 A0
#define ACS712_Number_2 A1

ACS712 ACS712_1(ACS712_30A, ACS712_Number_1);
ACS712 ACS712_2(ACS712_30A, ACS712_Number_2);

// 50Hz = 20ms per cycle
// 60Hz = 16.6ms per cycle
// Hence reading every 80ms should give two highs and two lows
// Hence reading every 200ms should give five highs and five lows
#define ACS712_Store_Values_Delay 200
unsigned long ACS712_Store_Values_At = 0;

#define ACS712_Max_Number_Of 2
float ACS712_Amps[ACS712_Max_Number_Of];
float ACS712_Watts[ACS712_Max_Number_Of];
float ACS712_Watts_Min[ACS712_Max_Number_Of];
float ACS712_Watts_Max[ACS712_Max_Number_Of];


// ------------------------------------------------------------ Ethernet ------------------------------------------------------------
#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC_Address address and IP_Address address for your controller below.
// The IP_Address address will be dependent on your local network:
// byte MAC_Address[] = {0xDE, 0xA2, 0x001, 0x03, 0x03, 0x07};

// Initialize the Ethernet server library
bool Ethernet_Up = false;


// ------------------------------------------------------------ MQTT ------------------------------------------------------------
#include <PubSubClient.h>

EthernetClient Ethernet_Client_MQTT;
PubSubClient MQTT_Client(Ethernet_Client_MQTT);

#define MQTT_Reconnect_Delay 1500
unsigned long MQTT_Reconnect_At = 0;

// ------------------------------------------------------------ EEPROM Config ------------------------------------------------------------
#include <EEPROMex.h>

// Used to get config since MQTT client will only resieve 127 bytes
#include <EthernetUdp.h>

// local port to listen on
#define EEPROM_Config_Port 8888

bool EEPROM_Config_Requested = false;


// ------------------------------------------------------------ CLI ------------------------------------------------------------
#define Command_List_Length 20
const char* Commands_List[Command_List_Length] = {
  "hostname",
  "wifi ssid",
  "wifi password",
  "mqtt broker",
  "mqtt port",
  "mqtt user",
  "mqtt password",
  "system header",
  "system sub header",
  "list",
  "json",
  "fs list",
  "fs format",
  "fs config load",
  "fs config drop",
  "show mac",
  "save",
  "check",
  "reboot",
  "shutdown"};

String CLI_Input_String;
bool CLI_Command_Complate = false;

#define Serial_CLI_Boot_Message_Timeout 3


// ############################################################ Headers ############################################################
// ############################################################ Headers ############################################################
// ############################################################ Headers ############################################################
void MQTT_Unsubscribe();

bool EEPROM_Config_Request(bool Force);

void EEPROM_Config_Print();

void CLI_Print(String Print_String);
String CLI_Config_Check();
void Serial_CLI();
bool Get_Serial_Input();
void Serial_CLI_Boot_Message(unsigned int Timeout);
void Serial_CLI_Command_Check();

void Log(String Topic, String Log_Text);
void Log(String Topic, int Log_Text);
void Log(String Topic, float Log_Text);



// ############################################################ nibble2c() ############################################################
char nibble2c(char c) {
   if ((c>='0') && (c<='9'))
      return c-'0' ;
   if ((c>='A') && (c<='F'))
      return c+10-'A' ;
   if ((c>='a') && (c<='a'))
      return c+10-'a' ;
   return -1 ;
} // nibble2c()

char hex2c(char c1, char c2)
{
   if(nibble2c(c2) >= 0)
     return nibble2c(c1)*16+nibble2c(c2) ;
   return nibble2c(c1) ;
}


// ############################################################ Reboot() ############################################################
void(* Reboot) (void) = 0;//declare reset function at address 0


// ############################################################ Log() ############################################################
// Writes text to MQTT and Serial
void Log(String Topic, String Log_Text) {
  // Print Log message to serial
  Serial.println(Topic + " - " + Log_Text);

  // See if its a log message and deturman message log level
  byte Message_Log_Level = 255;
  if (Topic.indexOf(DL.MQTT_Topic[Topic_Log_Debug]) != -1) Message_Log_Level = Log_Level_DEBUG;
  else if (Topic.indexOf(DL.MQTT_Topic[Topic_Log_Info]) != -1) Message_Log_Level = Log_Level_INFO;
  else if (Topic.indexOf(DL.MQTT_Topic[Topic_Log_Warning]) != -1) Message_Log_Level = Log_Level_WARNING;
  else if (Topic.indexOf(DL.MQTT_Topic[Topic_Log_Error]) != -1) Message_Log_Level = Log_Level_ERROR;
  else if (Topic.indexOf(DL.MQTT_Topic[Topic_Log_Fatal]) != -1) Message_Log_Level = Log_Level_FATAL;

  // Online/Offline check
  // Online
  if (MQTT_Client.connected() == true) {

    if (Message_Log_Level >= Log_Level) {
      // Print log queue
      if (Log_Queue_Topic.Length() > 0) {
        // Send offline being marker
        MQTT_Client.publish(String(DL.MQTT_Topic[Topic_Log_Info] + "/Log").c_str(), "Offline log begin");
        while (Log_Queue_Topic.Length() > 0) {
          // State message post as retained message
          if (Log_Queue_Topic.Peek().indexOf("/State") != -1) {
            MQTT_Client.publish(Log_Queue_Topic.Pop().c_str(), Log_Queue_Log_Text.Pop().c_str(), true);
          }
          // Post as none retained message
          MQTT_Client.publish(Log_Queue_Topic.Pop().c_str(), Log_Queue_Log_Text.Pop().c_str());
        }
        // Send offline end marker
        MQTT_Client.publish(String(DL.MQTT_Topic[Topic_Log_Info] + "/Log").c_str(), "Offline log end");
      }

      // State message post as retained message
      if (Topic.indexOf("/State") != -1) {
        MQTT_Client.publish(Topic.c_str(), 0, true, Log_Text.c_str());
      }
      // Post as none retained message
      else MQTT_Client.publish(Topic.c_str(), Log_Text.c_str());
    }
  }

  // Offline
  else {
    // Check log level
    if (Message_Log_Level >= Log_Level) {
      // Add to log queue
      Log_Queue_Topic.Push(Topic);
      Log_Queue_Log_Text.Push(Log_Text);
    }
  }

} // Log()

void Log(String Topic, int Log_Text) {
  Log(Topic, String(Log_Text));
} // Log - Reference only

void Log(String Topic, float Log_Text) {
  Log(Topic, String(Log_Text));
} // Log - Reference only


// ############################################################ Ethernet_Connection_Check() ############################################################
void Ethernet_Connection_Check() {

  // Check DHCP State
  // 0: nothing happened
  // 1: renew failed
  // 2: renew success
  // 3: rebind fail
  // 4: rebind success
  byte DHCP_State = Ethernet.maintain();

  if (DHCP_State == 1 || DHCP_State == 3) {
    Ethernet_Up = false;
  }
  else {
    Ethernet_Up = true;
    // Check if config have been requested
    if (EEPROM_Config_Requested == false) {
      // Request config
      EEPROM_Config_Request(false);
      EEPROM_Config_Requested = true;
    }
  }
} // Ethernet_Connection_Check()


// ############################################################ ACS712() ############################################################
bool ACS712(String Topic, String Payload) {

  if (Topic.substring(0, DL.MQTT_Topic[Topic_Ammeter].length()) != DL.MQTT_Topic[Topic_Ammeter]) {
    return false;
  }

  int Selected_ACS712 = Topic.substring(DL.MQTT_Topic[Topic_Ammeter].length() + 1, Topic.length()).toInt();

  if (Selected_ACS712 > 0 && Selected_ACS712 <= ACS712_Max_Number_Of) {
    // Convert to array Number
    Selected_ACS712 = Selected_ACS712 - 1;

    if (Payload == "?") {
      Log(DL.MQTT_Topic[Topic_Ammeter] + "/Watts/State", ACS712_Watts[Selected_ACS712]);
    }
    else if (Payload == "json") {
      // Create json buffer
      DynamicJsonBuffer jsonBuffer(220);
      JsonObject& root = jsonBuffer.createObject();

      // encode json string
      root.set("Voltage", 230);
      root.set("Amps", ACS712_Amps[Selected_ACS712]);
      root.set("Watts", ACS712_Watts[Selected_ACS712]);
      root.set("Watts Min", ACS712_Watts_Min[Selected_ACS712]);
      root.set("Watts Max", ACS712_Watts_Max[Selected_ACS712]);

      String Return_String;

      root.printTo(Return_String);

      Log(DL.MQTT_Topic[Topic_Ammeter] + "/json/State", Return_String);
    }
    else if (Payload == "Calibrate") {
      if (Selected_ACS712 == 0) {
        Log(DL.MQTT_Topic[Topic_Log_Debug] + "/System/ACS712/1", "Calibrating");
        ACS712_1.calibrate();
      }
      else if (Selected_ACS712 == 0) {
        Log(DL.MQTT_Topic[Topic_Log_Debug] + "/System/ACS712/2", "Calibrating");
        ACS712_2.calibrate();
      }
    }
    else {
      Log(DL.MQTT_Topic[Topic_Log_Warning] + "/ACS712", "Invalid request: " + Payload);
    }
  }
  else {
    Log(DL.MQTT_Topic[Topic_Log_Warning] + "/ACS712", "Invalid ACS712 selected");
  }


  return true;

} // ACS712()

// ############################################################ ACS712_Loop() ############################################################
void ACS712_Loop() {

  if (ACS712_Store_Values_At < millis()) {
    float U = 230;

    for (byte i = 0; i < ACS712_Max_Number_Of; i++) {
      if (i == 0) {
        ACS712_Amps[i] = ACS712_1.getCurrentAC();
      }
      else if (i == 0) {
        ACS712_Amps[i] = ACS712_2.getCurrentAC();
      }

      ACS712_Watts[i] = U * ACS712_Amps[i];

      ACS712_Watts_Min[i] = min(ACS712_Watts[i], ACS712_Watts_Min[i]);
      ACS712_Watts_Max[i] = max(ACS712_Watts[i], ACS712_Watts_Max[i]);
    }
  }

} // ACS712_Loop()


// ############################################################ Commands() ############################################################
bool Commands(String Topic, String Payload) {

  if (Topic.substring(0, DL.MQTT_Topic[Topic_Commands].length()) != DL.MQTT_Topic[Topic_Commands]) {
    return false;
  }

  // Trim topic
  Topic = Topic.substring(DL.MQTT_Topic[Topic_Commands].length() + 1);

  // Start checking commands
  if (Topic == "Power") {
    if (Payload == "Reboot") {
      Log(DL.MQTT_Topic[Topic_Log_Info] + "/System/Power", "Rebooting");
      // just a little delay
      delay(500);
      Reboot();
    } // Reboot
  } // Power


  else if (Topic == "Log") {
    if (Payload.indexOf("Level ") != -1) {
      // Remove "Level " so string only contains the level request
      Payload.replace("Level ", "");
      // Convert to lower calse
      Payload.toLowerCase();
      // Check log level
      if (Payload == "debug") Log_Level = Log_Level_DEBUG;
      else if (Payload == "info") Log_Level = Log_Level_INFO;
      else if (Payload == "warning") Log_Level = Log_Level_WARNING;
      else if (Payload == "error") Log_Level = Log_Level_ERROR;
      else if (Payload == "fatal") Log_Level = Log_Level_FATAL;
      // Report error
      else {
        Log(DL.MQTT_Topic[Topic_Log_Warning] + "/Log", "Unknown log level: " + Payload);
        return true;
      }
      // Log event
      Log(DL.MQTT_Topic[Topic_Log_Info] + "/Log", "Log level change to: " + Payload);
    } // Level
  } // Log


  else if (Topic == "EEPROM") {
    if (Payload == "Print") {
      EEPROM_Config_Print();
    } // Print
    if (Payload == "Request") {
      EEPROM_Config_Request(true);
    } // Request
  } // EEPROM

  else if (Topic == "Test") {
    Log(DL.MQTT_Topic[Topic_Log_Debug], "Test");
  } // Test

  return true;

} // Commands()


// ############################################################ MQTT_On_Message() ############################################################
void MQTT_On_Message(char* topic, byte* payload, unsigned int length) {

  // Convert topic to String
  String Topic = topic;

  // Convert payload to String
  String Payload;
  for (byte i = 0; i < length; i++) {
    Payload += String((char)payload[i]);
  }

  // Trim payload for potential garbage chars
  if (Payload.indexOf(";") != -1) {
    Payload = Payload.substring(0, Payload.indexOf(";"));
  }

  // Commands
  if (Commands(Topic, Payload) == true) return;

  // Ammeter
  if (ACS712(Topic, Payload) == true) return;

  else {
    Serial.println("MARKER Unknown message"); // rm
    Serial.println(Topic); // rm
    Serial.println(Payload); // rm
  }
}


// ############################################################ MQTT_KeepAlive() ############################################################
void MQTT_KeepAlive() {

  // If the MQTT Client is not connected no reason to send try to send a keepalive
  // Dont send keepalives during updates
  if (MQTT_Client.connected() == false) return;

  if (DL.MQTT_KeepAlive_At < millis()) {
    // Create json buffer
    DynamicJsonBuffer jsonBuffer(220);
    JsonObject& root = jsonBuffer.createObject();

    // encode json string
    root.set("Hostname", DL.Hostname);
    root.set("IP", DL.IPtoString(Ethernet.localIP()));
    root.set("Uptime", millis());
    root.set("FreeMemory", freeMemory());
    root.set("Software", Version);

    String KeepAlive_String;

    root.printTo(KeepAlive_String);

    Log(DL.MQTT_Topic[Topic_KeepAlive], KeepAlive_String);

    // Reset timer
    DL.MQTT_KeepAlive_At = millis() + DL.MQTT_KeepAlive_Interval;
  }

} // MQTT_KeepAlive()


// ############################################################ MQTT_Unsubscribe() ############################################################
void MQTT_Unsubscribe() {

  for (byte i = 0; i < MQTT_Topic_Number_Of; i++) {
    if (DL.MQTT_Subscribtion_Active[i] == true) {

      String Unsubscribe_String = DL.MQTT_Topic[i];
      if (DL.MQTT_Topic_Subscribe_Subtopic[i] == PLUS) Unsubscribe_String = Unsubscribe_String + "/+";
      else if (DL.MQTT_Topic_Subscribe_Subtopic[i] == HASH) Unsubscribe_String = Unsubscribe_String + "/#";
      MQTT_Client.unsubscribe(Unsubscribe_String.c_str());

      DL.MQTT_Subscribtion_Active[i] = false;
      Log(DL.MQTT_Topic[Topic_Log_Debug] + "/MQTT", "Unsubscribed from Topic: " + Unsubscribe_String);
    }
  }

} //MQTT_Unsubscribe()


// ############################################################ MQTT_Subscribe() ############################################################
void MQTT_Subscribe(String Topic, bool Activate_Topic, byte SubTopics) {

  byte Topic_Number = 255;

  for (byte i = 0; i < MQTT_Topic_Number_Of; i++) {
    if (Topic == DL.MQTT_Topic[i]) {
      Topic_Number = i;
      break;
    }
  }
  if (Topic_Number == 255) {
    Log(DL.MQTT_Topic[Topic_Log_Error] + "/MQTT", "Unknown Subscribe Topic: " + Topic);
    return;
  }

  DL.MQTT_Topic_Subscribe_Active[Topic_Number] = Activate_Topic;
  DL.MQTT_Topic_Subscribe_Subtopic[Topic_Number] = SubTopics;

  // Check if MQTT is connected
  if (MQTT_Client.connected() == false) {
    // if not then do nothing
    return;
  }

  String Subscribe_String = DL.MQTT_Topic[Topic_Number];

  if (DL.MQTT_Subscribtion_Active[Topic_Number] == false && DL.MQTT_Topic_Subscribe_Active[Topic_Number] == true) {

    // Check if already subscribed
    if (DL.MQTT_Subscribtion_Active[Topic_Number] == true && DL.MQTT_Topic_Subscribe_Active[Topic_Number] == true) {
      Log(DL.MQTT_Topic[Topic_Log_Warning] + "/MQTT", "Already subscribed to Topic: " + Subscribe_String);
      return;
    }
    // Add # or + to topic
    if (DL.MQTT_Topic_Subscribe_Subtopic[Topic_Number] == PLUS) Subscribe_String = Subscribe_String + "/+";
    else if (DL.MQTT_Topic_Subscribe_Subtopic[Topic_Number] == HASH) Subscribe_String = Subscribe_String + "/#";

    // Try to subscribe
    if (MQTT_Client.subscribe(Subscribe_String.c_str(), 0)) {
      Log(DL.MQTT_Topic[Topic_Log_Info] + "/MQTT", "Subscribing to Topic: " + Subscribe_String + " ... OK");
      DL.MQTT_Subscribtion_Active[Topic_Number] = true;
    }
    // Log failure
    else {
      Log(DL.MQTT_Topic[Topic_Log_Error] + "/MQTT", "Subscribing to Topic: " + Subscribe_String + " ... FAILED");
    }
  }
}


// ############################################################ MQTT_Reconnect() ############################################################
bool MQTT_Reconnect() {

  if (MQTT_Client.connect(DL.Hostname.c_str(), DL.MQTT_Username.c_str(), DL.MQTT_Password.c_str(), String(DL.MQTT_Topic[Topic_Log_Error] + "/System").c_str(), 0, false, "Disconnected from MQTT")) {
    Log(DL.MQTT_Topic[Topic_Log_Debug] + "/System/MQTT", "Connected to Broker: " + DL.MQTT_Broker);
    for (byte i = 0; i < MQTT_Topic_Number_Of; i++) {
      MQTT_Subscribe(DL.MQTT_Topic[i], DL.MQTT_Topic_Subscribe_Active[i], DL.MQTT_Topic_Subscribe_Subtopic[i]);
    }
  }
  return MQTT_Client.connected();
}


// ############################################################ EEPROM_Config_Print() ############################################################
void EEPROM_Config_Print() {

  String Print_String;

  char Read_Char;

  for (int i = 0; i < 4096; i++) {
    Read_Char = EEPROM.read(i);
    Print_String = Print_String + Read_Char;
    if (String(Read_Char) == "}") {
      break;
    }
  }

  Log(DL.MQTT_Topic[Topic_Log_Info] + "/System/EEPROM/Content", Print_String);

} // EEPROM_Config_Print()


// ############################################################ EEPROM_Config_Save() ############################################################
bool EEPROM_Config_Request(bool Force) {

  // An EthernetUDP instance to let us send and receive packets over UDP
  EthernetUDP UDP_Client;

  // For resiving config via UDP
  UDP_Client.begin(EEPROM_Config_Port);

  if (Force == true) {
    Log(DL.MQTT_Topic[Topic_Dobby] + "Config", DL.Hostname + ",-1,UDP," + DL.IPtoString(Ethernet.localIP())); // Request config
  }
  else {
    Log(DL.MQTT_Topic[Topic_Dobby] + "Config", DL.Hostname + "," + DL.Config_ID + ",UDP," + DL.IPtoString(Ethernet.localIP())); // Request config
  }

  // Give Dobby time to send the config
  delay(500);

  // if there's data available, read a packet
  int packetSize = UDP_Client.parsePacket();

  // buffers for receiving and sending data
  char Config_Buffer[packetSize];  // buffer to hold incoming packet,

  if (packetSize) {
    // read the packet into packetBufffer
    UDP_Client.read(Config_Buffer, packetSize);
  }
  else {
    Log(DL.MQTT_Topic[Topic_Log_Debug] + "/System/EEPROM", "No reply on config request");
    return false;
  }

  // Stop udp client
  UDP_Client.stop();

  if (String((char)Config_Buffer[0]) == "O" && String((char)Config_Buffer[1]) == "K") {
    Log(DL.MQTT_Topic[Topic_Log_Debug] + "/System/EEPROM", "Config up to date");
    return true;
  }

  // Validate config
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(Config_Buffer);

  String Temp_String;

  root.printTo(Temp_String);

  Temp_String.toCharArray(Config_Buffer, Temp_String.length() + 1);


  if (root.success() == true) {
    // Write config to EEPROM
    // + 6 is to offset for mac address stored in 0 to 5
    for (int i = 0; i < packetSize + 1; i++) {
      EEPROM.update(i + 6, Config_Buffer[i]);
    }

    // Log new config recieved
    Log(DL.MQTT_Topic[Topic_Log_Info] + "/System/EEPROM", "Got new config");

    // Reboot
    Log(DL.MQTT_Topic[Topic_Log_Info] + "/System/Power", "Rebooting");
    delay(500);
    Reboot();
  }
  else {
    Log(DL.MQTT_Topic[Topic_Log_Warning] + "/System/EEPROM", "Unable to phrase incomming config");
    return false;
  }

  return true;
} // EEPROM_Config_Save()


// ############################################################ EEPROM_Config_Load() ############################################################
bool EEPROM_Config_Load() {

  Log(DL.MQTT_Topic[Topic_Log_Info] + "/System/EEPROM", "Loading config");

  String json_String;

  // Check if 0 = "{" if not assume the config is not there
  if (String((char)EEPROM.read(6)) != "{") {
    Log(DL.MQTT_Topic[Topic_Log_Warning] + "/System/EEPROM", "No config found");
    return false;
  }

  // A Mega's EEPROM size 4096
  for (unsigned int i = 6; i < 4096; i++) {
    // Read next char
    String Read_String = String((char)EEPROM.read(i));
    // Add it to json_String
    json_String += Read_String;
    if (Read_String == "}") break;
  }

  // // Check if "}" in json_String if no then assime config is not there
  // if (json_String.indexOf("}") != -1) {
  // }

  // Convert the string to json
  DynamicJsonBuffer jsonBuffer(json_String.length() + 100);
  JsonObject& root = jsonBuffer.parseObject(json_String);

  if (root.success() == true) {
    // Start applying config
    // Start with minimum config
    if (root.containsKey("Hostname")) DL.Hostname = root.get<String>("Hostname");
    if (root.containsKey("MQTT_Broker")) DL.MQTT_Broker = root.get<String>("MQTT_Broker");
    if (root.containsKey("MQTT_Port")) DL.MQTT_Port = root.get<unsigned long>("MQTT_Port");
    if (root.containsKey("MQTT_Username")) DL.MQTT_Username = root.get<String>("MQTT_Username");
    if (root.containsKey("MQTT_Password")) DL.MQTT_Password = root.get<String>("MQTT_Password");
    if (root.containsKey("System_Header")) DL.System_Header = root.get<String>("System_Header");
    if (root.containsKey("System_Sub_Header")) DL.System_Sub_Header = root.get<String>("System_Sub_Header");

    // Now you can rebuild topics
    DL.Rebuild_MQTT_Topics();

    // Config ID
    if (root.containsKey("Config_ID")) {
      DL.Config_ID = root.get<String>("Config_ID");
    }
    else {
      DL.Config_ID = "-1";
    }

    Log(DL.MQTT_Topic[Topic_Log_Debug] + "/System/EEPROM", "Base config loaded");

    // ACS712

  }

  // Failed to phrase json config
  else {
    Log(DL.MQTT_Topic[Topic_Log_Warning] + "/System/EEPROM", "Unable to phrase config");
    Serial.println("MARKER json_String");
    Serial.println(json_String);
    return false;
  }

  return true;
} // EEPROM_Config_Load()


// ############################################################ MQTT_Loop() ############################################################
void MQTT_Loop() {

  if (MQTT_Client.connected() == true) {
    MQTT_KeepAlive();
    MQTT_Client.loop();
  }

  else if (MQTT_Client.connected() == false && Ethernet_Up == true) {
    if (MQTT_Reconnect_At < millis()) {
      // Try to reconnect
      Log(DL.MQTT_Topic[Topic_Log_Debug] + "/System/MQTT", "Reconnecting");
      MQTT_Reconnect();
      // Reset timer
      MQTT_Reconnect_At = millis() + MQTT_Reconnect_Delay;
      // Attempt to reconnect
    }
  }
} // MQTT_Loop


// ############################################################ Wait_For_Serial_Input() ############################################################
void Wait_For_Serial_Input(String Wait_String) {
  CLI_Input_String = "";

  Serial.println();
  Serial.print("Please enter new " + Wait_String + ": ");

  while (Get_Serial_Input() == false) delay(1);
}


// ############################################################ HexToByte() ############################################################
byte HexToByte(String Hex_String) {

  Hex_String.toLowerCase();

  // String Hex[2];
  byte Hex_Value[2];

  // Hex[0] = Hex_String.substring(0, 1);
  // Hex[1] = Hex_String.substring(1, 2);

  for (byte i = 0; i < 2; i++) {
    if (Hex_String.substring(i, i + 1) == "a") Hex_Value[i] = 10;
    else if (Hex_String.substring(i, i + 1) == "b") Hex_Value[i] = 11;
    else if (Hex_String.substring(i, i + 1) == "c") Hex_Value[i] = 12;
    else if (Hex_String.substring(i, i + 1) == "d") Hex_Value[i] = 13;
    else if (Hex_String.substring(i, i + 1) == "e") Hex_Value[i] = 14;
    else if (Hex_String.substring(i, i + 1) == "f") Hex_Value[i] = 15;
    else Hex_Value[i] = Hex_String.toInt();
  }

  return Hex_Value[0] << 4 | Hex_Value[1];
  // if (Hex_Value[0] != 0) {
  //   Serial.println("MARKER Hex_Value[0] * Hex_Value[1]"); // rm
  //   Serial.println(Hex_Value[0] * Hex_Value[1]); // rm
  //
  // }
  // else {
  //   Serial.println("MARKER Hex_Value[1]"); // rm
  //   Serial.println(Hex_Value[1]); // rm
  //   convertCharToHex(a)<<4 | convertCharToHex(b);
  //
  //   return Hex_Value[1];
  // }

} // HexToByte()


// ############################################################ Serial_CLI_Command_Check() ############################################################
void Serial_CLI_Command_Check() {

  if (CLI_Command_Complate == false) {
    return;
  }

  CLI_Input_String.toLowerCase();

  // MAC Address
  if (CLI_Input_String == "mac") {
    Wait_For_Serial_Input("mac address");

    // Check if "-" was used as devider
    if (CLI_Input_String.indexOf("-") != -1) {
      CLI_Input_String.replace("-", ":");
    }

    if (CLI_Input_String.substring(2, 3) != ":" && CLI_Input_String.substring(5, 6) != ":" && CLI_Input_String.substring(8, 9) != ":" && CLI_Input_String.substring(10, 11) != ":" && CLI_Input_String.substring(13, 14) != ":") {
      // Invalid max address entered
      Serial.println("ERROR: Invalid max address entered");
      return;
    }

    // Remove ":"
    CLI_Input_String.replace(":", "");

    // Buffer for Maddress bytes
    // char MAC_Addess_Bytes[6];
    // // Buffer to hold MAC Accress chars to be converted to bytes
    // char MAC_Addess_Chars[12];
    //
    // CLI_Input_String.toCharArray(MAC_Addess_Chars, CLI_Input_String.length());
    //
    //
    // Serial.println("MARKER");
    // for (byte i = 0; i < 6; i++) {
    //   MAC_Addess_Bytes[i] = hex2c(MAC_Addess_Chars[1], MAC_Addess_Chars[2]);
    //   Serial.println(MAC_Addess_Bytes[i]);
    // }


    for (byte i = 0; i < 12; i = i + 2) {
      Serial.println(HexToByte(CLI_Input_String.substring(i, i + 2)));
      Serial.println(HexToByte(CLI_Input_String.substring(i, i + 2)), HEX);
    }




    // for (byte i = 0; i < 12; i = i + 2) {
    //   // Serial.println(CLI_Input_String.substring(i, i + 1));
    //   // Serial.println(CLI_Input_String.substring(i + 1, i + 2));
    //   // if(nibble2c(CLI_Input_String.substring(i + 1, i + 2).c_str()) >= 0) {
    //   //   MAC_Addess_Bytes[x] = nibble2c(CLI_Input_String.substring(i, i + 1).c_str())*16+nibble2c(CLI_Input_String.substring(i + 1, i + 2).c_str());
    //   // }
    //   // else {
    //   //   MAC_Addess_Bytes[x] = nibble2c(CLI_Input_String.substring(i, i + 1).c_str()) ;
    //   // }
    //   x = x + 1;
    // }



    // for (byte i = 0; i < 6; i++) {
    //   EEPROM.update(i, MAC_Addess_Bytes[i]);
    // }



    // char hex2c(char c1, char c2)
    // {
      // if(nibble2c(c2) >= 0)
      // return nibble2c(c1)*16+nibble2c(c2);
      // return nibble2c(c1) ;
    // }


    // EEPROM.update(100, 0x09);
    // EEPROM.update(101, 0x10);
    // EEPROM.update(102, 0xAA);
    // EEPROM.update(103, 0x0A);
    // EEPROM.update(104, 0xA0);
    // EEPROM.update(105, 0xFF);
    //
    // Serial.println("MARKER");
    // Serial.println(EEPROM.read(100));
    // Serial.println(EEPROM.read(101));
    // Serial.println(EEPROM.read(102));
    // Serial.println(EEPROM.read(103));
    // Serial.println(EEPROM.read(104));
    // Serial.println(EEPROM.read(105));


    // Serial.println(CLI_Input_String.substring(0, 1));
    // Serial.println(CLI_Input_String.substring(1, 2));
    // Serial.println(CLI_Input_String.substring(2, 3));
    // af:bf:cf:df:ef:ff



    Serial.println("MAC Address: " + String(EEPROM.read(0), HEX) + ":" + String(EEPROM.read(1), HEX) + ":" + String(EEPROM.read(2), HEX) + ":" + String(EEPROM.read(3), HEX) + ":" + String(EEPROM.read(4), HEX) + ":" + String(EEPROM.read(5)));
  }
  // MQTT
  else if (CLI_Input_String == "mqtt broker") {
    Wait_For_Serial_Input("mqtt broker");
    DL.MQTT_Broker = CLI_Input_String;
    Serial.println("mqtt broker set to: " + DL.MQTT_Broker);
  }
  else if (CLI_Input_String == "mqtt port") {
    Wait_For_Serial_Input("mqtt port");
    DL.MQTT_Port = CLI_Input_String;
    Serial.println("mqtt port set to: " + DL.MQTT_Port);
  }
  else if (CLI_Input_String == "mqtt user") {
    Wait_For_Serial_Input("mqtt user");
    DL.MQTT_Username = CLI_Input_String;
    Serial.println("mqtt user set to: " + DL.MQTT_Username);
  }
  else if (CLI_Input_String == "mqtt password") {
    Wait_For_Serial_Input("mqtt password");
    DL.MQTT_Password = CLI_Input_String;
    Serial.println("mqtt password set to: " + DL.MQTT_Password);
  }
  else if (CLI_Input_String == "system header") {
    Wait_For_Serial_Input("system header");
    DL.System_Header = CLI_Input_String;
    Serial.println("system header set to: " + DL.System_Header);
  }
  else if (CLI_Input_String == "system sub header") {
    Wait_For_Serial_Input("system sub header");
    DL.System_Sub_Header = CLI_Input_String;
    Serial.println("system sub header set to: " + DL.System_Sub_Header);
  }

  // JSON Config
  else if (CLI_Input_String == "json") {
    Wait_For_Serial_Input("json config string");

    DynamicJsonBuffer jsonBuffer(CLI_Input_String.length() + 100);
    JsonObject& root = jsonBuffer.parseObject(CLI_Input_String);

    if (root.containsKey("Hostname")) DL.Hostname = root.get<String>("Hostname");
    if (root.containsKey("MQTT_Broker")) DL.MQTT_Broker = root.get<String>("MQTT_Broker");
    if (root.containsKey("MQTT_Port")) DL.MQTT_Port = root.get<unsigned long>("MQTT_Port");
    if (root.containsKey("MQTT_Username")) DL.MQTT_Username = root.get<String>("MQTT_Username");
    if (root.containsKey("MQTT_Password")) DL.MQTT_Password = root.get<String>("MQTT_Password");
    if (root.containsKey("System_Header")) DL.System_Header = root.get<String>("System_Header");
    if (root.containsKey("System_Sub_Header")) DL.System_Sub_Header = root.get<String>("System_Sub_Header");
  }

  // Misc
  else if (CLI_Input_String == "hostname") {
    Wait_For_Serial_Input("hostname");
    DL.Hostname = CLI_Input_String;
  }

  else if (CLI_Input_String == "help") {

    Serial.println("Avalible commands:");
    Serial.println("");

    for (int i = 0; i < Command_List_Length; i++) {
      Serial.println("\t" + String(Commands_List[i]));
      Serial.flush();
    }
  }

  else if (CLI_Input_String == "save" || CLI_Input_String == "write") {

    if (CLI_Config_Check() != "Passed") {
      Serial.println("\tConfig check failed, please run 'check' for details");
      Serial.println("\tConfiguration NOT saved to FS");
    }

    else {
      // Generate base config
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.createObject();

      root.set("Hostname", DL.Hostname);
      root.set("System_Header", DL.System_Header);
      root.set("System_Sub_Header", DL.System_Sub_Header);
      root.set("Config_ID", -2);
      root.set("MQTT_Broker", DL.MQTT_Broker);
      root.set("MQTT_Port", DL.MQTT_Port);
      root.set("MQTT_Username", DL.MQTT_Username);
      root.set("MQTT_Password", DL.MQTT_Password);
      root.set("MQTT_KeepAlive_Interval", DL.MQTT_KeepAlive_Interval);

      String Temp_String;

      root.printTo(Temp_String);

      // Buffer for json config string
      char Config_Buffer[Temp_String.length() + 1];

      Temp_String.toCharArray(Config_Buffer, Temp_String.length() + 1);

      // Write config to EEPROM
      // + 6 is to offset for the mac address stored in 0 to 5
      for (unsigned int i = 0; i < Temp_String.length() + 1; i++) {
        EEPROM.update(i + 6, Config_Buffer[i]);
      }
      Serial.println("Config saved to EEPROM");
    }
  }

  else if (CLI_Input_String == "list") {
    Serial.println("");
    Serial.println("Settings List:");
    Serial.println("\t" + String(Commands_List[0]) + ": " + DL.Hostname);
    Serial.println("\t" + String(Commands_List[3]) + ": " + DL.MQTT_Broker);
    Serial.println("\t" + String(Commands_List[4]) + ": " + DL.MQTT_Port);
    Serial.println("\t" + String(Commands_List[5]) + ": " + DL.MQTT_Username);
    Serial.println("\t" + String(Commands_List[6]) + ": " + DL.MQTT_Password);
    Serial.println("\t" + String(Commands_List[7]) + ": " + DL.System_Header);
    Serial.println("\t" + String(Commands_List[8]) + ": " + DL.System_Sub_Header);
    Serial.flush();
  }


  else if (CLI_Input_String == "config show") {
    EEPROM_Config_Print();
  }

  // else if (CLI_Input_String == "fs config load") {
  //   FS_Config_Load();
  // }
  //
  // else if (CLI_Input_String == "fs config drop") {
  //   FS_Config_Drop();
  // }
  //
  // else if (CLI_Input_String == "fs format") {
  //   FS_Format();
  // }

  else if (CLI_Input_String == "check") {
    Serial.println("Config Check: " + CLI_Config_Check());
  }

  else if (CLI_Input_String == "reboot") {

    Log(DL.MQTT_Topic[Topic_Log_Info] + "/System/Power", "Rebooting");
    // just a little delay
    delay(500);
    Reboot();
  }

  else if (CLI_Input_String == "show mac") {
    Serial.print("MAC Address: ");
    // Serial.print(MAC_Address[0], HEX);
    // Serial.print(":");
    // Serial.print(MAC_Address[1], HEX);
    // Serial.print(":");
    // Serial.print(MAC_Address[2], HEX);
    // Serial.print(":");
    // Serial.print(MAC_Address[3], HEX);
    // Serial.print(":");
    // Serial.print(MAC_Address[4], HEX);
    // Serial.print(":");
    // Serial.print(MAC_Address[5], HEX);
  }

  else {
    if (CLI_Input_String != "") Log(DL.MQTT_Topic[Topic_System] + "/CLI", "Unknown command: " + CLI_Input_String);
  }

  if (CLI_Input_String != "") CLI_Print("");
  CLI_Input_String = "";
  CLI_Command_Complate = false;
}


// ############################################################ CLI_Print() ############################################################
void CLI_Print(String Print_String) {

  Serial.println(Print_String);
  Serial.print(DL.Hostname + ": ");
}


// ############################################################ CLI_Config_Check() ############################################################
String CLI_Config_Check() {

  if (DL.Hostname == "NotConfigured") {
    return "Failed - Hostname not configured";
  }
  else if (DL.MQTT_Broker == "") {
    return "Failed - MQTT Broker not configured";
  }
  else if (DL.MQTT_Port == "") {
    return "Failed - MQTT Port not configured";
  }
  else if (DL.MQTT_Username == "") {
    return "Failed - MQTT Username not configured";
  }
  else if (DL.MQTT_Password == "") {
    return "Failed - MQTT Password not configured";
  }
  else if (DL.System_Header == "") {
    return "Failed - System Header not configured";
  }

  return "Passed";
}


// ############################################################ Serial_CLI() ############################################################
void Serial_CLI() {

  // if (Indicator_LED_Configured == true) Indicator_LED(LED_Config, true);

  Serial.println("");
  Serial.println("");
  if (CLI_Config_Check() != "Passed") Serial.println("Device not configured please do so, type help to list avalible commands");
  else Serial.println("############################## SERIAL CLI ##############################");
  CLI_Print("");

  // Eternal loop for Serial CLi
  while (true) {
    Get_Serial_Input();
    Serial_CLI_Command_Check();
    delay(1);
  }
}


// ############################################################ Get_Serial_Input() ############################################################
bool Get_Serial_Input() {

  if (Serial.available() > 0) {
    String Incomming_String = Serial.readString();
    Serial.print(Incomming_String);
    CLI_Input_String = CLI_Input_String + Incomming_String;
  }


  if (CLI_Input_String.indexOf("\n") != -1) {
    CLI_Input_String.replace("\r", "");
    CLI_Input_String.replace("\n", "");

    CLI_Input_String.trim();

    CLI_Command_Complate = true;
    return true;
  }

  return false;
}


// ############################################################ Serial_CLI_Boot_Message() ############################################################
void Serial_CLI_Boot_Message(unsigned int Timeout) {
  Serial.println("Press any key to enter Serial CLI, timeout in: ");
  for (byte i = Timeout; i > 0; i--) {
    Serial.println(i);
    delay(1000);
    if (Get_Serial_Input() == true) {
      Serial_CLI();
    }
  }
  Serial.println("Timeout");
}


// ############################################################ Base_Config_Check() ############################################################
void Base_Config_Check() {

  if (DL.Hostname == "") {
    Serial_CLI();
  }

  else if (DL.MQTT_Broker == "") {
    Serial_CLI();
  }

  else if (DL.MQTT_Port == "") {
    Serial_CLI();
  }

  else if (DL.MQTT_Username == "") {
    Serial_CLI();
  }

  else if (DL.MQTT_Password == "") {
    Serial_CLI();
  }

  else if (DL.System_Header == "") {
    Serial_CLI();
  }

  else {
    Log(DL.MQTT_Topic[Topic_System] + "/Dobby", "Base config check done, all OK");
    return;
  }
}


// ############################################################ Load_MAC_Address() ############################################################
void Check_MAC_Address() {

  Serial.println("MARKER EEPROM CHECK");
  Serial.println(EEPROM.read(0), HEX);
  Serial.println(EEPROM.read(1), HEX);
  Serial.println(EEPROM.read(2), HEX);
  Serial.println(EEPROM.read(3), HEX);
  Serial.println(EEPROM.read(4), HEX);
  Serial.println(EEPROM.read(5), HEX);
}


// ############################################################ setup() ############################################################
void setup() {

  // Serial
  Serial.setTimeout(100);
  Serial.begin(115200);

  Serial_CLI_Boot_Message(Serial_CLI_Boot_Message_Timeout);

  // Load MAC Address for EEPROM
  Check_MAC_Address();

  // Load Config
  EEPROM_Config_Load();

  Base_Config_Check();

  // Dont know what this is for
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  Log(DL.MQTT_Topic[Topic_Log_Info] + "/System", "Booting DobbyMega - Version: " + String(Version) + " Free Memory: " + String(freeMemory()));

  // Start Ethernet and request DHCP
  Log(DL.MQTT_Topic[Topic_Log_Debug] + "/System", "Starting Ethernet");

  byte Temp_MAC[] = {EEPROM.read(0), EEPROM.read(1), EEPROM.read(2), EEPROM.read(3), EEPROM.read(4), EEPROM.read(5)};

  if (Ethernet.begin(Temp_MAC, 10000, 1000) == true) {
    Ethernet_Up = true;
    Log(DL.MQTT_Topic[Topic_Log_Info] + "/Ethernet", "Link up");
    Log(DL.MQTT_Topic[Topic_Log_Info] + "/Ethernet", "Got IP: " + DL.IPtoString(Ethernet.localIP()));
  }
  else {
    Ethernet_Up = false;
    Log(DL.MQTT_Topic[Topic_Log_Error] + "/System", "Link down");
  }

  // MQTT
  Log(DL.MQTT_Topic[Topic_Log_Debug] + "/System/MQTT", "Configuring");
  MQTT_Client.setServer(DL.StringToIP(DL.MQTT_Broker), DL.MQTT_Port.toInt());
  MQTT_Client.setCallback(MQTT_On_Message);

  // ACS712
  Log(DL.MQTT_Topic[Topic_Log_Info] + "/System/ACS712", "Configuring");
  Log(DL.MQTT_Topic[Topic_Log_Debug] + "/System/ACS712/1", "Calibrating");
  ACS712_1.calibrate();
  Log(DL.MQTT_Topic[Topic_Log_Debug] + "/System/ACS712/2", "Calibrating");
  ACS712_2.calibrate();
  MQTT_Subscribe(DL.MQTT_Topic[Topic_Ammeter], true, PLUS);
  Log(DL.MQTT_Topic[Topic_Log_Info] + "/System/ACS712", "Configuration compleate");


  Log(DL.MQTT_Topic[Topic_Log_Info] + "/System", "Boot compleate");

} // setup()


// ############################################################ loop() ############################################################
void loop() {

  MQTT_Loop();

  ACS712_Loop();

  Ethernet_Connection_Check();

} // loop()
