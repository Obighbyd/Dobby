// Licence:
// Releaed under The GNU General Public License v3.0

// Change log:
// See ChangeLog.txt

// Bug list:
// See BugList.txt


#include <Arduino.h>
extern "C" {
	#include "user_interface.h"
}


// ---------------------------------------- Dobby ----------------------------------------
#define Version 103003
// First didget = Software type 1-Production 2-Beta 3-Alpha
// Secound and third didget = Major version number
// Fourth to sixth = Minor version number

String Hostname = "/NotConfigured";
String System_Header = "/Dobby";
String System_Sub_Header = "";
String Config_ID = "0";
bool Config_Requested = false;

// Log level = to Topic Log X
// Topic_Log_Debug 5
// Topic_Log_Info 6
// Topic_Log_Warning 7
// Topic_Log_Error 8
// Topic_Log_Fatal 9
#define Message 253
#define Device 254

byte System_Log_Level = 6;


// ------------------------------------------------------------ WiFi ------------------------------------------------------------
#include <ESP8266WiFi.h>
#include <Ticker.h>


WiFiClient WiFi_Client;

WiFiEventHandler gotIpEventHandler;
WiFiEventHandler disconnectedEventHandler;

String WiFi_SSID;
String WiFi_Password;

bool WiFi_Disconnect_Message_Send = true;

// ------------------------------------------------------------ MQTT ------------------------------------------------------------
#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include <MB_Queue.h>

String MQTT_Hostname;

String MQTT_Broker;
String MQTT_Port;

String MQTT_Username;
String MQTT_Password;

// If the buffer size is not set then they system will not work propperly
AsyncMqttClient MQTT_Client;

#define MQTT_RX_Queue_Max_Size 100
MB_Queue MQTT_RX_Queue_Topic(MQTT_RX_Queue_Max_Size);
MB_Queue MQTT_RX_Queue_Payload(MQTT_RX_Queue_Max_Size);

#define MQTT_Reconnect_Interval 2000
Ticker MQTT_Reconnect_Ticker;

// Time between MQTT publish
#define MQTT_Publish_Interval 10
unsigned long MQTT_Publish_At;

Ticker MQTT_KeepAlive_Ticker;
unsigned long MQTT_KeepAlive_Interval = 60;

#define MQTT_State_Init 0
#define MQTT_State_Connecting 1
#define MQTT_State_Connected 2
#define MQTT_State_Disconnected 3
#define MQTT_State_Error 4
byte MQTT_State = MQTT_State_Init;

int MQTT_Last_Error;

#define NONE 0
#define PLUS 1
#define HASH 2

const byte MQTT_Topic_Number_Of = 24;

// System
#define Topic_Config 0
#define Topic_Commands 1
#define Topic_All 2
#define Topic_KeepAlive 3
#define Topic_Dobby 4
// Log
#define Topic_Log_Debug 5
#define Debug Topic_Log_Debug
#define Topic_Log_Info 6
#define Info Topic_Log_Info
#define Topic_Log_Warning 7
#define Warning Topic_Log_Warning
#define Topic_Log_Error 8
#define Error Topic_Log_Error
#define Topic_Log_Fatal 9
#define Fatal Topic_Log_Fatal
#define Topic_Log_Will 23
#define Will Topic_Log_Will
// Devices
#define Topic_Ammeter 10
#define Topic_Button 11
#define Topic_DHT 12
#define Topic_DC_Voltmeter 13
#define Topic_Dimmer 14
#define Topic_Distance 15
#define Topic_MQ 16
#define Topic_Relay 17
#define Topic_PIR 18
#define Topic_MAX31855 19
#define Topic_Switch 20
#define Topic_Flow 21
#define Topic_DS18B20 22

// System
#define Topic_Config_Text "/Config/"
#define Topic_Commands_Text "/Commands/"
#define Topic_All_Text "/All"
#define Topic_KeepAlive_Text "/KeepAlive/"
#define Topic_Dobby_Text "/Commands/Dobby/"
// Log
#define Topic_Log_Debug_Text "/Debug"
#define Topic_Log_Info_Text "/Info"
#define Topic_Log_Warning_Text "/Warning"
#define Topic_Log_Error_Text "/Error"
#define Topic_Log_Fatal_Text "/Fatal"
#define Topic_Log_Will_Text "/Will"
// Devices
#define Topic_Ammeter_Text "/Ammeter/"
#define Topic_Button_Text "/Button/"
#define Topic_DHT_Text "/DHT/"
#define Topic_DC_Voltmeter_Text "/DC_Voltmeter/"
#define Topic_Dimmer_Text "/Dimmer/"
#define Topic_Distance_Text "/Distance/"
#define Topic_MQ_Text "/MQ/"
#define Topic_Relay_Text "/Relay/"
#define Topic_PIR_Text "/PIR/"
#define Topic_MAX31855_Text "/MAX31855/"
#define Topic_Switch_Text "/Switch/"
#define Topic_Flow_Text "/Flow/"
#define Topic_DS18B20_Text "/DS18B20/"

String MQTT_Topic[MQTT_Topic_Number_Of] = {
	// System
	System_Header + Topic_Config_Text + Hostname,
	System_Header + Topic_Commands_Text + Hostname,
	System_Header + Topic_All_Text,
	System_Header + Topic_KeepAlive_Text + Hostname,
	System_Header + Topic_Dobby_Text,
	// Log
	System_Header + Topic_Log_Debug_Text + Hostname,
	System_Header + Topic_Log_Info_Text + Hostname,
	System_Header + Topic_Log_Warning_Text + Hostname,
	System_Header + Topic_Log_Error_Text + Hostname,
	System_Header + Topic_Log_Fatal_Text + Hostname,
	System_Header + Topic_Log_Will_Text + Hostname,
	// Devices
	System_Header + System_Sub_Header + Topic_Ammeter_Text + Hostname,
	System_Header + System_Sub_Header + Topic_Button_Text + Hostname,
	System_Header + System_Sub_Header + Topic_DHT_Text + Hostname,
	System_Header + System_Sub_Header + Topic_DC_Voltmeter_Text + Hostname,
	System_Header + System_Sub_Header + Topic_Dimmer_Text + Hostname,
	System_Header + System_Sub_Header + Topic_Distance_Text + Hostname,
	System_Header + System_Sub_Header + Topic_MQ_Text + Hostname,
	System_Header + System_Sub_Header + Topic_Relay_Text + Hostname,
	System_Header + System_Sub_Header + Topic_PIR_Text + Hostname,
	System_Header + System_Sub_Header + Topic_MAX31855_Text + Hostname,
	System_Header + System_Sub_Header + Topic_Switch_Text + Hostname,
	System_Header + System_Sub_Header + Topic_Flow_Text + Hostname,
	System_Header + System_Sub_Header + Topic_DS18B20_Text + Hostname,
};

bool MQTT_Topic_Subscribe_Active[MQTT_Topic_Number_Of] = {
	// System
	false,
	true,
	true,
	false,
	false,
	// Log
	false,
	false,
	false,
	false,
	false,
	// Devices
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
};

byte MQTT_Topic_Subscribe_Subtopic[MQTT_Topic_Number_Of] = {
	// System
	NONE,
	HASH,
	HASH,
	NONE,
	NONE,
	// Log
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	// Devices
	NONE,
	PLUS,
	PLUS,
	NONE,
	PLUS,
	PLUS,
	NONE,
	PLUS,
	PLUS,
	NONE,
	NONE,
	PLUS,
	NONE,
};

bool MQTT_Subscribtion_Active[MQTT_Topic_Number_Of];




// ---------------------------------------- ArduinoOTA_Setup() ----------------------------------------
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SPI.h>

bool ArduinoOTA_Active = false;


// ------------------------------------------------------------ Log() ------------------------------------------------------------
// http://github.com/MBojer/MB_Queue
#include <MB_Queue.h>
// FIX - Might need to add support for log full handling
#define Log_Max_Queue_Size 100
MB_Queue MQTT_TX_Queue_Topic(Log_Max_Queue_Size);
MB_Queue MQTT_TX_Queue_Payload(Log_Max_Queue_Size);


// ---------------------------------------- SPIFFS() ----------------------------------------
#include "FS.h"


// ------------------------------------------------------------ CLI ------------------------------------------------------------
#define Command_List_Length 25
const char* Commands_List[Command_List_Length] = {
	"check",
	"crash print",
	"crash clear",
	"crash count",
	"fs cat",
	"fs config drop",
	"fs config load",
	"fs format",
	"fs list",
	"hostname ",
	"json",
	"list",
	"mqtt broker",
	"mqtt password",
	"mqtt port",
	"mqtt user",
	"reboot",
	"save",
	"show mac",
	"show wifi",
	"shutdown",
	"system header",
	"system sub header",
	"wifi password",
	"wifi ssid"};

String CLI_Input_String;
bool CLI_Command_Complate = false;

#define Serial_CLI_Boot_Message_Timeout 3


// ------------------------------------------------------------ ESP_Reboot() ------------------------------------------------------------
Ticker ESP_Power_Ticker;


// ------------------------------------------------------------ FS_Config ------------------------------------------------------------
#include <ArduinoJson.h>

#define Config_Json_Max_Buffer_Size 2048
bool Config_json_Loaded = false;

#define FS_Confing_File_Name "/Dobby.json"

String Incompleate_Config = "";
byte Incompleate_Config_Count = 0;


// ------------------------------------------------------------ Indicator_LED() ------------------------------------------------------------
bool Indicator_LED_Configured = true;

Ticker Indicator_LED_Blink_Ticker;
Ticker Indicator_LED_Blink_OFF_Ticker;

bool Indicator_LED_State = true;

unsigned int Indicator_LED_Blink_For = 150;

byte Indicator_LED_Blinks_Active = false;
byte Indicator_LED_Blinks_Left;

#define LED_Number_Of_States 5
#define LED_OFF 0
#define LED_MQTT 1
#define LED_WiFi 2
#define LED_Config 3
#define LED_PIR 4

float Indicator_LED_State_Hertz[LED_Number_Of_States] = {0, 0.5, 1, 2.5, 0.25};
bool Indicator_LED_State_Active[LED_Number_Of_States] = {true, false, false, false, false};


// ------------------------------------------------------------ Pin_Monitor ------------------------------------------------------------
// 0 = In Use
#define Pin_In_Use 0
#define Pin_Free 1
#define Pin_SCL 2
#define Pin_SDA 3
#define Pin_OneWire 4
#define Pin_Error 255
// Action
#define Reserve_Normal 0
#define Reserve_I2C_SCL 1
#define Reserve_I2C_SDA 2
#define Check_State 3
#define Reserve_OneWire 4

#define Pin_Monitor_Pins_Number_Of 10
String Pin_Monitor_Pins_Names[Pin_Monitor_Pins_Number_Of] = {"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "A0"};
byte Pin_Monitor_Pins_List[Pin_Monitor_Pins_Number_Of] = {D0, D1, D2, D3, D4, D5, D6, D7, D8, A0};

byte Pin_Monitor_Pins_Active[Pin_Monitor_Pins_Number_Of] = {Pin_Free, Pin_Free, Pin_Free, Pin_Free, Pin_Free, Pin_Free, Pin_Free, Pin_Free, Pin_Free, Pin_Free};

String Pin_Monitor_State_Text[5] = {"In Use", "Free", "I2C SCL", "I2C SDA", "OneWire"};


// ------------------------------------------------------------ Relay ------------------------------------------------------------
#define Relay_Max_Number_Of 6

bool Relay_Configured = false;

bool Relay_On_State = false;

byte Relay_Pins[Relay_Max_Number_Of] = {255, 255, 255, 255, 255, 255};
bool Relay_Pin_Auto_Off[Relay_Max_Number_Of];
unsigned long Relay_Pin_Auto_Off_Delay[Relay_Max_Number_Of] = {0, 0, 0, 0, 0, 0};

#define OFF 0
#define ON 1
#define FLIP 2


// ------------------------------------------------------------ Relay Auto Off ------------------------------------------------------------
unsigned long Relay_Auto_OFF_At[Relay_Max_Number_Of];
bool Relay_Auto_OFF_Active[Relay_Max_Number_Of];


// ------------------------------------------------------------ Dimmer ------------------------------------------------------------
#define Dimmer_Max_Number_Of 6
bool Dimmer_Configured = false;
byte Dimmer_Pins[Dimmer_Max_Number_Of] = {255, 255, 255, 255, 255, 255};

int Dimmer_State[Dimmer_Max_Number_Of];
byte Dimmer_Procent[Dimmer_Max_Number_Of];

byte Dimmer_Fade_Jump = 20;
byte Dimmer_Fade_Jump_Delay = 40;


// ------------------------------------------------------------ Button ------------------------------------------------------------
#define Button_Max_Number_Of 6
bool Button_Configured = false;
byte Button_Pins[Button_Max_Number_Of] = {255, 255, 255, 255, 255, 255};

unsigned long Button_Ignore_Input_Untill[Button_Max_Number_Of];
unsigned int Button_Ignore_Input_For = 750; // Time in ms before a butten can triggered again

String Button_Target[Button_Max_Number_Of];


// ------------------------------------------------------------ Switch ------------------------------------------------------------
bool Switch_Configured = false;

#define Switch_Max_Number_Of 6
byte Switch_Pins[Switch_Max_Number_Of] = {255, 255, 255, 255, 255, 255};

unsigned long Switch_Ignore_Input_Untill;
unsigned long Switch_Refresh_Rate = 250; // ms between checking switch state

bool Switch_Last_State[Switch_Max_Number_Of];

String Switch_Target_ON[Switch_Max_Number_Of];
String Switch_Target_OFF[Switch_Max_Number_Of];


// ------------------------------------------------------------ MQ ------------------------------------------------------------
bool MQ_Configured = false;

byte MQ_Pin_A0 = 255;

int MQ_Current_Value = -1;
int MQ_Value_Min = -1;
int MQ_Value_Max = -1;


Ticker MQ_Ticker;
#define MQ_Refresh_Rate 100


// ------------------------------------------------------------ DHT() ------------------------------------------------------------
#include <SimpleDHT.h>
SimpleDHT22 dht22;

bool DHT_Configured;

#define DHT_Max_Number_Of 6
byte DHT_Pins[DHT_Max_Number_Of] = {255, 255, 255, 255, 255, 255};

unsigned long DHT_Last_Read = 0;						// When the sensor was last read

Ticker DHT_Ticker;

// Do not put this below 2500ms it will make the DHT22 give errors
#define DHT_Refresh_Rate 2500
float DHT_Current_Value_Humidity[DHT_Max_Number_Of] = {0, 0, 0, 0, 0, 0};
float DHT_Current_Value_Temperature[DHT_Max_Number_Of] = {0, 0, 0, 0, 0, 0};
float DHT_Min_Value_Humidity[DHT_Max_Number_Of] = {0, 0, 0, 0, 0, 0};
float DHT_Min_Value_Temperature[DHT_Max_Number_Of] = {0, 0, 0, 0, 0, 0};
float DHT_Max_Value_Humidity[DHT_Max_Number_Of] = {0, 0, 0, 0, 0, 0};
float DHT_Max_Value_Temperature[DHT_Max_Number_Of] = {0, 0, 0, 0, 0, 0};

#define DHT_Distable_At_Error_Count 10
byte DHT_Error_Counter[DHT_Max_Number_Of] = {0, 0, 0, 0, 0, 0};


// ------------------------------------------------------------ PIR ------------------------------------------------------------
bool PIR_Configured = false;

#define PIR_Max_Number_Of 6
#define PIR_State_Number_Of 3
// State 0 = Disabeled

byte PIR_Pins[PIR_Max_Number_Of] = {255, 255, 255, 255, 255, 255};

String PIR_Topic[PIR_Max_Number_Of];
String PIR_OFF_Payload[PIR_Max_Number_Of];
String PIR_ON_Payload[PIR_State_Number_Of][PIR_Max_Number_Of];

int PIR_OFF_Delay[PIR_Max_Number_Of]; // in sec
unsigned long PIR_OFF_At[PIR_Max_Number_Of];

bool PIR_ON_Send[PIR_Max_Number_Of];

// ON = Light
bool PIR_LDR_ON[PIR_Max_Number_Of];

bool PIR_Disabled[PIR_Max_Number_Of];

byte PIR_Selected_Level[PIR_Max_Number_Of] = {1, 1, 1, 1, 1, 1};


// ------------------------------------------------------------ DC Voltmeter ------------------------------------------------------------
bool DC_Voltmeter_Configured = false;

byte DC_Voltmeter_Pins = 255;

float DC_Voltmeter_R1 = 0.0; // 30000.0
float DC_Voltmeter_R2 = 0.0; // 7500.0
float DC_Voltmeter_Curcit_Voltage = 0.0; // 3.3
float DC_Voltmeter_Offset = 0.0;


// ------------------------------------------------------------ Flow ------------------------------------------------------------
bool Flow_Configured = false;

#define Flow_Max_Number_Of 6
byte Flow_Pins[Flow_Max_Number_Of] = {255, 255, 255, 255, 255, 255};

volatile unsigned long Flow_Pulse_Counter[Flow_Max_Number_Of];
volatile bool Flow_Change[Flow_Max_Number_Of];

#define Flow_Publish_Delay 2500
unsigned long Flow_Publish_At[Flow_Max_Number_Of];


// ------------------------------------------------------------ DS18B20 ------------------------------------------------------------
// Based on: https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <OneWire.h>
#include <DallasTemperature.h>

byte DS18B20_Pin = D7;
#define DS18B20_Max_Number_Of 4

OneWire DS18B20_OneWire(DS18B20_Pin);
DallasTemperature DS18B20_Sensors(&DS18B20_OneWire);

DeviceAddress DS18B20_Address[DS18B20_Max_Number_Of];
String DS18B20_Address_String[DS18B20_Max_Number_Of];

#define DS18B20_Resolution 9

bool DS18B20_Configured = false;

Ticker DS18B20_Ticker;

#define DS18B20_Refresh_Rate 2500
float DS18B20_Current[DS18B20_Max_Number_Of] = {0};
float DS18B20_Min[DS18B20_Max_Number_Of] = {0};
float DS18B20_Max[DS18B20_Max_Number_Of] = {0};

byte DS18B20_Error_Counter[DS18B20_Max_Number_Of];
#define DS18B20_Error_Max 15



// ------------------------------------------------------------ Distance ------------------------------------------------------------
#include <RunningAverage.h>
#define Distance_Max_Number_Of 3

#define Distance_Refresh_Rate 250
// Dont start reading before 4 sec after boot just to let the system compleate boot
unsigned long Distance_Sensor_Read_At = 4000;

bool Distance_Configured = false;
byte Distance_Pins_Echo[Distance_Max_Number_Of] = {255, 255, 255};
byte Distance_Pins_Trigger[Distance_Max_Number_Of] = {255, 255, 255};

int Distance_ON_At[Distance_Max_Number_Of] = {0, 0, 0};
int Distance_OFF_At[Distance_Max_Number_Of] = {0, 0, 0};
String Distance_Target_ON[Distance_Max_Number_Of];
String Distance_Target_OFF[Distance_Max_Number_Of];

bool Distance_Sensor_State[Distance_Max_Number_Of];

#define Distance_RunningAverage_Length 25
RunningAverage Distance_RA_1(Distance_RunningAverage_Length);
RunningAverage Distance_RA_2(Distance_RunningAverage_Length);
RunningAverage Distance_RA_3(Distance_RunningAverage_Length);
float Distance_Last_Reading[Distance_Max_Number_Of];

#define Distance_Sensor_Publish_Delay 1500
unsigned long Distance_Sensor_Publish_At[Distance_Max_Number_Of];


// ------------------------------------------------------------ Publish Uptime ------------------------------------------------------------
bool Publish_Uptime = false;

#define Publish_Uptime_Interval 30000
unsigned long Publish_Uptime_At = Publish_Uptime_Interval;
unsigned long Publish_Uptime_Offset;

#define Topic_Uptime System_Header + "/Uptime/" + Hostname


// ############################################################ Headers ############################################################
// ############################################################ Headers ############################################################
// ############################################################ Headers ############################################################

void MQTT_Connect();
void MQTT_Subscribe(String Topic, bool Activate_Topic, byte SubTopics);
void Rebuild_MQTT_Topics();

bool Is_Valid_Number(String str);
unsigned int HEX_Str_To_Int(String hexString);

String FS_Config_Build();
bool FS_Config_Save();
void FS_Config_Drop();
bool FS_Config_Load();
void FS_Format();

byte Pin_Monitor(byte Action, byte Pin_Number);
String Number_To_Pin(byte Pin_Number);
byte Pin_To_Number(String Pin_Name);

bool Relay(String &Topic, String &Payload);
bool Dimmer(String &Topic, String &Payload);
bool Switch(String &Topic, String &Payload);
bool DHT(String &Topic, String &Payload);
bool DC_Voltmeter(String &Topic, String &Payload);
bool PIR(String &Topic, String &Payload);
bool DS18B20(String &Topic, String &Payload);
void DS18B20_Scan();
void DS18B20_Loop();
bool Distance(String &Topic, String &Payload);
bool MQ(String &Topic, String &Payload);
void Distance_Loop();
bool Flow(String &Topic, String &Payload);

void ICACHE_RAM_ATTR Flow_Callback_0();
void ICACHE_RAM_ATTR Flow_Callback_1();
void ICACHE_RAM_ATTR Flow_Callback_2();
void ICACHE_RAM_ATTR Flow_Callback_3();
void ICACHE_RAM_ATTR Flow_Callback_4();
void ICACHE_RAM_ATTR Flow_Callback_5();

void MQ_Loop();

void MQTT_KeepAlive();
void MQTT_On_Message(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);


// ############################################################ Functions ############################################################
// ############################################################ Functions ############################################################
// ############################################################ Functions ############################################################


// ############################################################ Async_Disconnect_Reson() ############################################################
String Async_Disconnect_Reson(byte Reason) {

	if (Reason == 0) return "TCP disconnected";
	else if (Reason == 1) return "Unacceptable protocol version";
	else if (Reason == 2) return "Identifier rejected";
	else if (Reason == 3) return "Server unavailable";
	else if (Reason == 4) return "Malformed credentials";
	else if (Reason == 5) return "Not authorized";
	else return "Unknown disconnect reason";	

} // Async_Disconnect_Reson


// ############################################################ getRandomNumber() ############################################################
int getRandomNumber(int startNum, int endNum) {
	randomSeed(ESP.getCycleCount());
	return random(startNum, endNum);
} // getRandomNumber


// ############################################################ Device_Address_To_String() ############################################################
String Device_Address_To_String(DeviceAddress deviceAddress)
{
	String Return_String;

	for (uint8_t i = 0; i < 8; i++)
	{
		// zero pad the address if necessary
		if (deviceAddress[i] < 16) Return_String = Return_String + "0";
		Return_String = Return_String + String(deviceAddress[i], HEX);
	}
	// Return in uppercase
	Return_String.toUpperCase();
	// return value
	return Return_String;
} // Device_Address_To_String


// ############################################################ Log_Level_To_String() ############################################################
// Returns "-1" if Log_Level byte is not a valid string log leve
String Log_Level_To_String(byte Log_Level) {

	String Log_Level_String;

	if (Log_Level == Topic_Log_Debug)
	{
		Log_Level_String = "Debug";
	}
	else if (Log_Level == Topic_Log_Info)
	{
		Log_Level_String = "Info";
	}
	else if (Log_Level == Topic_Log_Warning)
	{
		Log_Level_String = "Warning";
	}
	else if (Log_Level == Topic_Log_Error)
	{
		Log_Level_String = "Error";
	}
	else
	{
		Log_Level_String = "-1";
	}

	return Log_Level_String;

} // Log_Level_To_String()

// ############################################################ Log_Level_To_Byte() ############################################################
// Returns -1 if string is invalid log level
int Log_Level_To_Byte(String Log_Level) {

	int Log_Level_Byte;

	if (Log_Level == "Debug")
	{
		Log_Level_Byte = Topic_Log_Debug;
	}
	else if (Log_Level == "Info")
	{
		Log_Level_Byte = Topic_Log_Info;
	}
	else if (Log_Level == "Warning")
	{
		Log_Level_Byte = Topic_Log_Warning;
	}
	else if (Log_Level == "Error")
	{
		Log_Level_Byte = Topic_Log_Error;
	}
	else
	{
		Log_Level_Byte = -1;
	}

	return Log_Level_Byte;

} // Log_Level_To_Byte()


// ############################################################ HEX_Str_To_Int() ############################################################
unsigned int HEX_Str_To_Int(String hexString) {
 
  unsigned int decValue = 0;
  int nextInt;
  
  for (unsigned int i = 0; i < hexString.length(); i++) {
    
    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    
    decValue = (decValue * 16) + nextInt;
  }
  
  return decValue;


} // HEX_Str_To_Int()


// ############################################################ Is_Valid_Number() ############################################################
bool Is_Valid_Number(String str) {
	for(byte i=0;i<str.length();i++)
	{
		if(isDigit(str.charAt(i))) return true;
	}
	return false;
} // Is_Valid_Number()


// ############################################################ IP_To_String() ############################################################
String IP_To_String(IPAddress IP_Address) {

	String Temp_String = String(IP_Address[0]) + "." + String(IP_Address[1]) + "." + String(IP_Address[2]) + "." + String(IP_Address[3]);

	return Temp_String;

} // IP_To_String
 

 // ############################################################ String_To_IP() ############################################################
IPAddress String_To_IP(String IP_String) {

	for (byte y = 0; y < 2; y++) {
		if (IP_String.indexOf(".") == -1) {
			Serial.println("Invalid ip entered");
			return IPAddress(0, 0, 0, 0);
		}
	}

	byte IP_Part[4];

	byte Dot_Location_Start = 0;
	byte Dot_Location_End = IP_String.indexOf(".");
	IP_Part[0] = IP_String.substring(Dot_Location_Start, Dot_Location_End).toInt();

	Dot_Location_Start = Dot_Location_End + 1;
	Dot_Location_End = IP_String.indexOf(".", Dot_Location_Start);
	IP_Part[1] = IP_String.substring(Dot_Location_Start, Dot_Location_End).toInt();

	Dot_Location_Start = Dot_Location_End + 1;
	Dot_Location_End = IP_String.indexOf(".", Dot_Location_Start);
	IP_Part[2] = IP_String.substring(Dot_Location_Start, Dot_Location_End).toInt();

	Dot_Location_Start = Dot_Location_End + 1;
	IP_Part[3] = IP_String.substring(Dot_Location_Start).toInt();

	for (byte i = 0; i < 4; i++) {
		if (IP_Part[i] > 255) {
			Serial.println("Invalid ip entered");
			return IPAddress(0, 0, 0, 0);
		}
	}

	return IPAddress(IP_Part[0], IP_Part[1], IP_Part[2], IP_Part[3]);
}

// ############################################################ MQTT_TX() ############################################################
void MQTT_TX(String Topic, String Payload) {
	MQTT_TX_Queue_Topic.Push(Topic);
	MQTT_TX_Queue_Payload.Push(Payload);
}
void MQTT_TX(String Topic, int Payload) {
	MQTT_TX(Topic, String(Payload));
} // MQTT_TX - Reference only

void MQTT_TX(String Topic, float Payload) {
	MQTT_TX(Topic, String(Payload));
} // MQTT_TX - Reference only

void MQTT_TX(String Topic, unsigned long Payload) {
	MQTT_TX(Topic, String(Payload));
} // MQTT_TX - Reference only


// ############################################################ Log() ############################################################
// Writes text to MQTT and Serial
void Log(byte Log_Level, String Post_Topic, String Log_Text) {

	// Log_Level_X = Topic_Level_X

	// Build Topic
	String Topic;

	// Check if Log Level is "Device" or "Message" aka 254, 253 then dont add anything before Post_Topic
	if (Log_Level == Device || Log_Level == Message)
	{
		Topic = Post_Topic;
	}
	// else add log level info to topic
	else
	{
		Topic = MQTT_Topic[Log_Level] + Post_Topic;
	}

	// Always print to serial regardless of log level
	Serial.println("TX - " + Topic + " - " + Log_Text);
	
	// Check log level
	// No reason to store messages we are not sending
	if (Log_Level >= System_Log_Level)
	{
		// Publish MQTT
		MQTT_TX(Topic, Log_Text);
	}
} // Log()


void Log(byte Log_Level, String Post_Topic, int Log_Text) {
	Log(Log_Level, Post_Topic, String(Log_Text));
} // Log - Reference only

void Log(byte Log_Level, String Post_Topic, float Log_Text) {
	Log(Log_Level, Post_Topic, String(Log_Text));
} // Log - Reference only

void Log(byte Log_Level, String Post_Topic, unsigned long Log_Text) {
	Log(Log_Level, Post_Topic, String(Log_Text));
} // Log - Reference only


// ############################################################ Log_Empthy() ############################################################
// Empythys the log, it will ignore the current MQTT state and print everything in the log to serial
// This function is used before a reboot 
void Log_Empthy() {
	while (MQTT_TX_Queue_Topic.Length() > 0)
	{
		// State message post as retained message
        if (MQTT_TX_Queue_Topic.Peek().indexOf("/State") != -1) {
          MQTT_Client.publish(MQTT_TX_Queue_Topic.Pop().c_str(), 0, true, MQTT_TX_Queue_Payload.Pop().c_str());
        }
        // Post as none retained message
        else {
          MQTT_Client.publish(MQTT_TX_Queue_Topic.Pop().c_str(), 0, false, MQTT_TX_Queue_Payload.Pop().c_str());
        }
	}
} // Log_Empthy()


// ############################################################ Log Loop() ############################################################
// Posts loofline log when connected to mqtt
void Log_Loop() {
	// Online/Offline check
	// Offline
  	if (MQTT_State != MQTT_State_Connected) return;
	// Online - Check if its time to publish
	if (MQTT_Publish_At < millis()) {
		// Print log queue
		if (MQTT_TX_Queue_Topic.Length() > 0) {
			// State message post as retained message
			if (MQTT_TX_Queue_Topic.Peek().indexOf("/State") != -1) {
				MQTT_Client.publish(MQTT_TX_Queue_Topic.Pop().c_str(), 0, true, MQTT_TX_Queue_Payload.Pop().c_str());
			}
			// Post as none retained message
			else {
				MQTT_Client.publish(MQTT_TX_Queue_Topic.Pop().c_str(), 0, false, MQTT_TX_Queue_Payload.Pop().c_str());
			}
		}
	MQTT_Publish_At = millis() + MQTT_Publish_Interval;
  	}
} // Log_Loop()


// ############################################################ Indicator_LED_Blink_OFF() ############################################################
void Indicator_LED_Blink_OFF () {
	// On Wemos High = Low ... ?
	digitalWrite(D4, HIGH);

	if (Indicator_LED_Blinks_Active == true) {
		if (Indicator_LED_Blinks_Left == 0) {
			Indicator_LED_Blinks_Active = false;
			Indicator_LED_Blink_Ticker.detach();
		}
		else Indicator_LED_Blinks_Left--;
	}
}

// ############################################################ Indicator_LED_Blink() ############################################################
void Indicator_LED_Blink_Now() {

	digitalWrite(D4, LOW); // think is has to be low to be ON
	Indicator_LED_Blink_OFF_Ticker.once_ms(Indicator_LED_Blink_For, Indicator_LED_Blink_OFF);

} // Indicator_LED_Blink()


// ############################################################ Indicator_LED_Blink() ############################################################
void Indicator_LED_Blink(byte Number_Of_Blinks) {

	Log(Info, + "/IndicatorLED", "Blinking " + String(Number_Of_Blinks) + " times");

	Indicator_LED_Blinks_Active = true;
	Indicator_LED_Blinks_Left = Number_Of_Blinks - 1;

	Indicator_LED_Blink_Now(); // for instant reaction then attach the ticket below
	Indicator_LED_Blink_Ticker.attach(1, Indicator_LED_Blink_Now);

} // Indicator_LED_Blink()


/* ############################################################ Indicator_LED() ############################################################
			Blinkes the blue onboard led based on the herts specified in a float
			NOTE: Enabling this will disable pind D4
			0 = Turn OFF
*/
void Indicator_LED(byte LED_State, bool Change_To) {

	if (Indicator_LED_Configured == false) {
		Log(Debug, "/IndicatorLED", "Indicator LED not configured");
		return;
	}

	// Set state
	Indicator_LED_State_Active[LED_State] = Change_To;

	// Of another state and LED_OFF is set to true set LED_OFF to false to enable the LED
	if (LED_State != 0 && Change_To == true) {
		Indicator_LED_State_Active[LED_OFF] = false;
	}

	bool State_Active = false;

	// Set indication
	for (byte i = 0; i < LED_Number_Of_States; i++) {
		if (Indicator_LED_State_Active[i] == true) {
			if (i != 0) {
				// Start blinking at first herts to let the most "important" state come first
				Indicator_LED_Blink_Ticker.attach(Indicator_LED_State_Hertz[LED_State], Indicator_LED_Blink_Now);
				State_Active = true;
				// Break the loop to only trigger the first active state
				break;
			}
		}
	}
	// Check if any other then LED_OFF is active if not set LED_OFF to true and disabled blink
	if (State_Active == false) {
		Indicator_LED_State_Active[LED_OFF] = true;
	}

	// Check if led OFF is true
	if (Indicator_LED_State_Active[LED_OFF] == true) {
		// Stop blinking
		Indicator_LED_Blink_Ticker.detach();
		// Turn off LED
		// NOTE: HIGH = OFF
		digitalWrite(D4, HIGH);
	}
} // Indicator_LED()


void Reboot_Now() {
	ESP.restart();
}

// ############################################################ ESP_Reboot() ############################################################
void ESP_Reboot() {

	Log(Info, "/System", "Rebooting");

	Log_Empthy();

	Serial.flush();
	// Short delay before reboot to be sure all messages gets out
	ESP_Power_Ticker.once_ms(1500, Reboot_Now);
	
	MQTT_Client.disconnect();

} // ESP_Reboot()


// ############################################################ ESP_Shutdown() ############################################################
void ESP_Shutdown() {

	Log(Warning, "/System", "Shutting down");
	MQTT_Client.disconnect();
	Serial.flush();

	ESP.deepSleep(0);

} // ESP_Shutdown()



// ############################################################ Reboot() ############################################################
void Reboot(unsigned long Reboot_In) {

	Log(Info, "/System", "Kill command issued, rebooting in " + String(Reboot_In / 1000) + " seconds");

	// Empyhy Log
	while(MQTT_TX_Queue_Topic.Queue_Is_Empthy == false)
	{
		Log_Loop();
	}

	ESP_Power_Ticker.once_ms(Reboot_In, ESP_Reboot);

} // Reboot()


// ############################################################ Shutdown() ############################################################
void Shutdown(unsigned long Shutdown_In) {

	Log(Warning, "/System", "Kill command issued, shutting down in " + String(Shutdown_In / 1000) + " seconds");

	ESP_Power_Ticker.once_ms(Shutdown_In, ESP_Shutdown);

} // Shutdown()


// ############################################################ CLI_Print() ############################################################
void CLI_Print(String Print_String) {

	Serial.println(Print_String);
	Serial.print(Hostname + ": ");
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


// ############################################################ Wait_For_Serial_Input() ############################################################
void Wait_For_Serial_Input(String Wait_String) {
	CLI_Input_String = "";

	Serial.println();
	Serial.print("Please enter new " + Wait_String + ": ");

	while (Get_Serial_Input() == false) delay(1);
}


// ############################################################ CLI_Config_Check() ############################################################
String CLI_Config_Check() {

	if (Hostname == "NotConfigured") {
		return "Failed - Hostname not configured";
	}
	else if (WiFi_SSID == "") {
		return "Failed - WiFi SSID not configured";
	}
	else if (WiFi_Password == "") {
		return "Failed - WiFi Password not configured";
	}
	else if (MQTT_Broker == "") {
		return "Failed - MQTT Broker not configured";
	}
	else if (MQTT_Port == "") {
		return "Failed - MQTT Port not configured";
	}
	else if (MQTT_Username == "") {
		return "Failed - MQTT Username not configured";
	}
	else if (MQTT_Password == "") {
		return "Failed - MQTT Password not configured";
	}
	else if (System_Header == "") {
		return "Failed - System Header not configured";
	}

	return "Passed";
}


// ############################################################ Serial_CLI_Command_Check() ############################################################
void Serial_CLI_Command_Check() {

	if (CLI_Command_Complate == false) {
		return;
	}

	CLI_Input_String.toLowerCase();

	// WiFi
	if (CLI_Input_String == "wifi ssid") {
		Wait_For_Serial_Input("wifi ssid");
		WiFi_SSID = CLI_Input_String;
		Serial.println("wifi ssid set to: " + WiFi_SSID);
	}
	else if (CLI_Input_String == "wifi password") {
		Wait_For_Serial_Input("wifi password");
		WiFi_Password = CLI_Input_String;
		Serial.println("wifi password set to: " + WiFi_Password);
	}

	// MQTT
	else if (CLI_Input_String == "mqtt broker") {
		Wait_For_Serial_Input("mqtt broker");
		MQTT_Broker = CLI_Input_String;
		Serial.println("mqtt broker set to: " + MQTT_Broker);
	}
	else if (CLI_Input_String == "mqtt port") {
		Wait_For_Serial_Input("mqtt port");
		MQTT_Port = CLI_Input_String;
		Serial.println("mqtt port set to: " + MQTT_Port);
	}
	else if (CLI_Input_String == "mqtt user") {
		Wait_For_Serial_Input("mqtt user");
		MQTT_Username = CLI_Input_String;
		Serial.println("mqtt user set to: " + MQTT_Username);
	}
	else if (CLI_Input_String == "mqtt password") {
		Wait_For_Serial_Input("mqtt password");
		MQTT_Password = CLI_Input_String;
		Serial.println("mqtt password set to: " + MQTT_Password);
	}
	else if (CLI_Input_String == "system header") {
		Wait_For_Serial_Input("system header");
		System_Header = CLI_Input_String;
		Serial.println("system header set to: " + System_Header);
	}
	else if (CLI_Input_String == "system sub header") {
		Wait_For_Serial_Input("system sub header");
		System_Sub_Header = CLI_Input_String;
		Serial.println("system sub header set to: " + System_Sub_Header);
	}

	// JSON Config
	else if (CLI_Input_String == "json") {
		Wait_For_Serial_Input("json config string");

		DynamicJsonBuffer jsonBuffer(CLI_Input_String.length() + 100);
		JsonObject& root_CLI = jsonBuffer.parseObject(CLI_Input_String);

		if (root_CLI.containsKey("Hostname")) Hostname = root_CLI.get<String>("Hostname");
		if (root_CLI.containsKey("WiFi_SSID")) WiFi_SSID = root_CLI.get<String>("WiFi_SSID");
		if (root_CLI.containsKey("WiFi_Password")) WiFi_Password = root_CLI.get<String>("WiFi_Password");
		if (root_CLI.containsKey("MQTT_Broker")) MQTT_Broker = root_CLI.get<String>("MQTT_Broker");
		if (root_CLI.containsKey("MQTT_Port")) MQTT_Port = root_CLI.get<unsigned long>("MQTT_Port");
		if (root_CLI.containsKey("MQTT_Username")) MQTT_Username = root_CLI.get<String>("MQTT_Username");
		if (root_CLI.containsKey("MQTT_Password")) MQTT_Password = root_CLI.get<String>("MQTT_Password");
		if (root_CLI.containsKey("System_Header")) System_Header = root_CLI.get<String>("System_Header");
		if (root_CLI.containsKey("System_Sub_Header")) System_Sub_Header = root_CLI.get<String>("System_Sub_Header");
	}

	// Misc
	else if (CLI_Input_String == "hostname") {
		Wait_For_Serial_Input("hostname");
		Hostname = CLI_Input_String;
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
			FS_Config_Save();

			Serial.println("Config saved to FS");
		}
	}

	else if (CLI_Input_String == "list") {
		Serial.println("");
		Serial.println("Settings List:");
		Serial.println("\t" + String(Commands_List[9]) + ": " + Hostname);
		Serial.println("\t" + String(Commands_List[24]) + ": " + WiFi_SSID);
		Serial.println("\t" + String(Commands_List[23]) + ": " + WiFi_Password);
		Serial.println("\t" + String(Commands_List[12]) + ": " + MQTT_Broker);
		Serial.println("\t" + String(Commands_List[14]) + ": " + MQTT_Port);
		Serial.println("\t" + String(Commands_List[15]) + ": " + MQTT_Username);
		Serial.println("\t" + String(Commands_List[13]) + ": " + MQTT_Password);
		Serial.println("\t" + String(Commands_List[21]) + ": " + System_Header);
		Serial.println("\t" + String(Commands_List[22]) + ": " + System_Sub_Header);
		Serial.flush();
	}


	else if (CLI_Input_String == "fs list") {
		String str = "";
		Dir dir = SPIFFS.openDir("/");
		while (dir.next()) {
			str += "\t";
			str += dir.fileName();
			str += " / ";
			str += dir.fileSize();
			str += "\r\n";
		}
		Serial.println("");
		Serial.println("\tFS File List:");
		Serial.println(str);
	}

	else if (CLI_Input_String == "fs cat") {
		Wait_For_Serial_Input("file name");
		String File_Path = CLI_Input_String;

		if (SPIFFS.exists(File_Path)) {

			File f = SPIFFS.open(File_Path, "r");
			if (f && f.size()) {

				String cat_String;

				while (f.available()){
					cat_String += char(f.read());
				}

				f.close();

				Serial.println("");
				Serial.println("cat " + File_Path + ":");
				Serial.print(cat_String);
			}
		}
	}

	else if (CLI_Input_String == "fs config load") {
		FS_Config_Load();
	}

	else if (CLI_Input_String == "fs config drop") {
		FS_Config_Drop();
	}

	else if (CLI_Input_String == "fs format") {
		FS_Format();
	}

	else if (CLI_Input_String == "check") {
		Serial.println("Config Check: " + CLI_Config_Check());
	}

	else if (CLI_Input_String == "reboot") {
		Serial.println("");
		for (byte i = 3; i > 0; i--) {
			Serial.printf("\tRebooting in: %i\r", i);
			delay(1000);
		}
		Log(Info, "/System", "Rebooting");

		delay(500);
		ESP.restart();
	}

	else if (CLI_Input_String == "shutdown") {
		Serial.println("");
		for (byte i = 3; i > 0; i--) {
			Serial.printf("\tShutting down in: %i\r", i);
			delay(1000);
		}
		Log(Info, "/Warning", "Shutdown, bye bye :-(");

		delay(500);
		ESP.deepSleep(0);
	}

	else if (CLI_Input_String == "show mac") {
		Serial.println("MAC Address: " + WiFi.macAddress());
	}
	
	else if (CLI_Input_String == "show wifi") {
		Serial.println("Connected to SSID: '" + WiFi_SSID + "' - IP: '" + IP_To_String(WiFi.localIP()) + "' - MAC Address: '" + WiFi.macAddress() + "'");
	}

	// // ######################################## Crash commands ########################################
	// // Crash print
	// else if (CLI_Input_String == "crash print") {
	// 	Fatal::print();
	// }
	// // crash clear
	// else if (CLI_Input_String == "crash clear") {
	// 	Serial.println("Crash log cleared");
	// 	Fatal::clear();
	// }
	// // crash count
	// else if (CLI_Input_String == "crash count") {
	// 	Serial.print("Crash log count: ");
	// 	Fatal::count();
	// }


	else {
		if (CLI_Input_String != "") Log(Warning, + "/CLI", "Unknown command: " + CLI_Input_String);
	}

	if (CLI_Input_String != "") CLI_Print("");
	CLI_Input_String = "";
	CLI_Command_Complate = false;
}


// ############################################################ Serial_CLI() ############################################################
void Serial_CLI() {

	if (Indicator_LED_Configured == true) Indicator_LED(LED_Config, true);

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


// ############################################################ Serial_CLI_Boot_Message() ############################################################
void Serial_CLI_Boot_Message(unsigned int Timeout) {
	for (byte i = Timeout; i > 0; i--) {
		Serial.printf("\tPress any key to enter Serial CLI, timeout in: %i \r", i);
		delay(1000);
		if (Get_Serial_Input() == true) {
			Serial_CLI();
		}
	}
	byte i = 0;
	Serial.printf("\tPress any key to enter Serial CLI, timeout in: %i \r", i);
	Serial.println("");
}


// ############################################################ FS_Config_Settings_Set(String Array) ############################################################
bool FS_Config_Settings_Set(String String_Array[], byte Devices_Max, String Payload, String Log_Post_Text, String Log_Text) {

	String Payload_String = Payload + ","; // adding "," to make for loop add up

	for (byte i = 0; i < Devices_Max; i++) {

		// Find number and set variable
		String_Array[i] = Payload_String.substring(0, Payload_String.indexOf(","));

		// Remove the value
		Payload_String = Payload_String.substring(Payload_String.indexOf(",") + 1, Payload_String.length());

		if (Payload_String.indexOf(",") == -1) break; // No more pins in string
	}

		String Publish_String = "'" + Log_Text + "' changed to: ";

		// Publish String - Pins
		if (Log_Text.indexOf(" Pins") != -1) {
			for (byte i = 0; i < Devices_Max; i++) {
				Publish_String = Publish_String + String(i + 1) + "=" + Number_To_Pin(String_Array[i].toInt());
				if (i != Devices_Max - 1) Publish_String = Publish_String + " ";
			}
		}

		// Publish String - Anything else
		else {
			for (byte i = 0; i < Devices_Max; i++) {
				Publish_String = Publish_String + String(i + 1) + "=" + String_Array[i];
				if (i != Devices_Max - 1) Publish_String = Publish_String + " ";
			}
		}

	Log(Debug, "/" + Log_Post_Text, Publish_String);
	return true;

} // FS_Config_Settings_Set()


// ############################################################ FS_Config_Settings_Set(Integer Array) ############################################################
bool FS_Config_Settings_Set(int Integer_Array[], byte Devices_Max, String Payload, String Log_Post_Text, String Log_Text) {

	String Payload_String = Payload + ","; // adding "," to make for loop add up

	for (byte i = 0; i < Devices_Max; i++) {

		// Find number and set variable
		Integer_Array[i] = Payload_String.substring(0, Payload_String.indexOf(",")).toInt();

		// Remove the value
		Payload_String = Payload_String.substring(Payload_String.indexOf(",") + 1, Payload_String.length());

		if (Payload_String.indexOf(",") == -1) break; // No more pins in string
	}

	String Publish_String = "'" + Log_Text + "' changed to: ";

	// Publish String - Pins
	if (Log_Text.indexOf(" Pins") != -1) {
		for (byte i = 0; i < Devices_Max; i++) {
			Publish_String = Publish_String + String(i + 1) + "=" + Number_To_Pin(Integer_Array[i]);
			if (i != Devices_Max - 1) Publish_String = Publish_String + " ";
		}
	}

	// Publish String - Anything else
	else {
		for (byte i = 0; i < Devices_Max; i++) {
			Publish_String = Publish_String + String(i + 1) + "=" + Integer_Array[i];
			if (i != Devices_Max - 1) Publish_String = Publish_String + " ";
		}
	}

	Log(Debug, "/" + Log_Post_Text, Publish_String);
	return true;

} // FS_Config_Settings_Set()


// ############################################################ FS_Config_Settings_Set(Float Array) ############################################################
bool FS_Config_Settings_Set(float Float_Array[], byte Devices_Max, String Payload, String Log_Post_Text, String Log_Text) {

	String Payload_String = Payload + ","; // adding "," to make for loop add up

	for (byte i = 0; i < Devices_Max; i++) {

		// Find number and set variable
		Float_Array[i] = Payload_String.substring(0, Payload_String.indexOf(",")).toFloat();

		// Remove the value
		Payload_String = Payload_String.substring(Payload_String.indexOf(",") + 1, Payload_String.length());

		if (Payload_String.indexOf(",") == -1) break; // No more pins in string
	}

	String Publish_String = "'" + Log_Text + "' changed to: ";

	for (byte i = 0; i < Devices_Max; i++) {
		Publish_String = Publish_String + String(i + 1) + "=" + Float_Array[i];
		if (i != Devices_Max - 1) Publish_String = Publish_String + " ";
	}

	Log(Debug, "/" + Log_Post_Text, Publish_String);
	return true;

} // FS_Config_Settings_Set()


// ############################################################ FS_Config_Settings_Set(Byte Array) ############################################################
bool FS_Config_Settings_Set(byte Byte_Array[], byte Devices_Max, String Payload, String Log_Post_Text, String Log_Text) {

	String Payload_String = Payload + ","; // adding "," to make for loop add up

	for (byte i = 0; i < Devices_Max; i++) {

		// Get value
		String Pin_String = Payload_String.substring(0, Payload_String.indexOf(","));

		// Check if number of pin name
		// Pin Nname
		if (Pin_String.indexOf("D") != -1) {
			Byte_Array[i] = Pin_To_Number(Payload_String.substring(0, Payload_String.indexOf(",")));
		}
		// Pin Number
		else {
			Byte_Array[i] = Payload_String.substring(0, Payload_String.indexOf(",")).toInt();
		}

		// Remove the value
		Payload_String = Payload_String.substring(Payload_String.indexOf(",") + 1, Payload_String.length());

		if (Payload_String.indexOf(",") == -1) break; // No more pins in string
	}

	String Publish_String = "'" + Log_Text + "' changed to: ";

	// Publish String - Pins
	if (Log_Text.indexOf(" Pins") != -1) {
		for (byte i = 0; i < Devices_Max; i++) {
			Publish_String = Publish_String + String(i + 1) + "=" + Number_To_Pin(Byte_Array[i]);
			if (i != Devices_Max - 1) Publish_String = Publish_String + " ";
		}
	}

	// Publish String - Anything else
	else {
		for (byte i = 0; i < Devices_Max; i++) {
			Publish_String = Publish_String + String(i + 1) + "=" + Byte_Array[i];
			if (i != Devices_Max - 1) Publish_String = Publish_String + " ";
		}
	}


	Log(Debug, "/" + Log_Post_Text, Publish_String);
	return true;


} // FS_Config_Settings_Set()


// ############################################################ FS_Config_Settings_Set(Unsigned Long Array) ############################################################
bool FS_Config_Settings_Set(unsigned long Unsigned_Long_Array[], byte Devices_Max, String Payload, String Log_Post_Text, String Log_Text) {

	String Payload_String = Payload + ","; // adding "," to make for loop add up

	for (byte i = 0; i < Devices_Max; i++) {

		// Find number and set variable
		Unsigned_Long_Array[i] = Payload_String.substring(0, Payload_String.indexOf(",")).toInt();

		// Remove the value
		Payload_String = Payload_String.substring(Payload_String.indexOf(",") + 1, Payload_String.length());

		if (Payload_String.indexOf(",") == -1) break; // No more pins in string
	}

	// Publish String
	String Publish_String = "'" + Log_Text + "' changed to: ";
	for (byte i = 0; i < Devices_Max; i++) {
		Publish_String = Publish_String + String(i + 1) + "=" + Unsigned_Long_Array[i];
		if (i != Devices_Max - 1) Publish_String = Publish_String + " ";
	}


	Log(Debug, "/" + Log_Post_Text, Publish_String);
	return true;


} // FS_Config_Settings_Set()


// ############################################################ FS_Config_Settings_Set(Boolian Array) ############################################################
bool FS_Config_Settings_Set(bool Boolian_Array[], byte Devices_Max, String Payload, String Log_Post_Text, String Log_Text) {

	String Payload_String = Payload + ","; // adding "," to make for loop add up

	for (byte i = 0; i < Devices_Max; i++) {

		// Find number and set variable
		Boolian_Array[i] = Payload_String.substring(0, Payload_String.indexOf(",")).toInt();

		// Remove the value
		Payload_String = Payload_String.substring(Payload_String.indexOf(",") + 1, Payload_String.length());

		if (Payload_String.indexOf(",") == -1) break; // No more pins in string
	}

	// Publish String
	String Publish_String = "'" + Log_Text + "' changed to: ";
	for (byte i = 0; i < Devices_Max; i++) {
		Publish_String = Publish_String + String(i + 1) + "=" + Boolian_Array[i];
		if (i != Devices_Max - 1) Publish_String = Publish_String + " ";
	}

	Log(Debug, "/" + Log_Post_Text, Publish_String);
	return true;

} // FS_Config_Settings_Set()


// #################################### FS_Config_Build() ####################################
String FS_Config_Build() {

	DynamicJsonBuffer jsonBuffer(Config_Json_Max_Buffer_Size);
	JsonObject& root_Config = jsonBuffer.createObject();

	// System
	if (Hostname != "") root_Config.set("Hostname", Hostname);
	root_Config.set("System_Header", System_Header);
	root_Config.set("System_Sub_Header", System_Sub_Header);
	root_Config.set("Config_ID", Config_ID);
	if (WiFi_SSID != "") root_Config.set("WiFi_SSID", WiFi_SSID);
	if (WiFi_Password != "") root_Config.set("WiFi_Password", WiFi_Password);
	if (MQTT_Broker != "") root_Config.set("MQTT_Broker", MQTT_Broker);
	if (MQTT_Port != "") root_Config.set("MQTT_Port", MQTT_Port);
	root_Config.set("MQTT_Username", MQTT_Username);
	root_Config.set("MQTT_Password", MQTT_Password);
	root_Config.set("MQTT_KeepAlive_Interval", MQTT_KeepAlive_Interval);

	// Devices


	String Return_String;
	root_Config.printTo(Return_String);
	return Return_String;

} // FS_Config_Build()


// #################################### FS_Config_Save() ####################################
bool FS_Config_Save() {

	File configFile = SPIFFS.open(FS_Confing_File_Name, "w");
	if (!configFile) {
		Log(Error, "/FSConfig", "Failed to open config file for writing");
		return false;
	}

	configFile.print(FS_Config_Build());
	configFile.close();

	Log(Info, "/FSConfig", "Saved to SPIFFS");

	return true;
} // FS_Config_Save()


// #################################### FS_Config_Show() ####################################
bool FS_Config_Show() {

	String File_Path = "/Dobby.json";

	if (SPIFFS.exists(File_Path)) {
		File f = SPIFFS.open(File_Path, "r");
		if (f && f.size()) {

			String cat_String;

			while (f.available()){
				cat_String += char(f.read());
			}

			f.close();

			// Publish config file
			Log(Info, "/FSConfig", cat_String);
		}
	}

	return true;

} // FS_Config_Show()


// #################################### FS_Config_Drop() ####################################
void FS_Config_Drop() {

	// Create json string to store base config
	const size_t bufferSize = JSON_ARRAY_SIZE(9) + 60;
	DynamicJsonBuffer jsonBuffer(bufferSize);
	JsonObject& root = jsonBuffer.createObject(); 

	// Generate json
	root.set("Hostname", Hostname);
	root.set("System_Header", System_Header);
	root.set("System_Sub_Header", System_Sub_Header);
	root.set("WiFi_SSID", WiFi_SSID);
	root.set("WiFi_Password", WiFi_Password);
	root.set("MQTT_Broker", MQTT_Broker);
	root.set("MQTT_Port", MQTT_Port);
	root.set("MQTT_Username", MQTT_Username);
	root.set("MQTT_Password", MQTT_Password);
	root.set("Config_ID", "0");

	File configFile = SPIFFS.open(FS_Confing_File_Name, "w");
	
	if (!configFile) {
		Log(Error, "/FSConfig", "Failed to open config file for writing");
		return;
	}

	root.printTo(configFile);
	configFile.close();

	Log(Info, "/FSConfig", "Config droped clean config saved to SPIFFS");

	return;
} // FS_Config_Drop()


// ############################################################ FS_Config_Load() ############################################################
bool FS_Config_Load() {

	// Open file
	File configFile = SPIFFS.open(FS_Confing_File_Name, "r");
	// File check
	if (!configFile) {
		Config_json_Loaded = true;
		Log(Info, "/FSConfig", "Failed to open config file");
		configFile.close();
		Serial_CLI();
		return false;
	}

	size_t size = configFile.size();
	if (size > Config_Json_Max_Buffer_Size) {
		Config_json_Loaded = true;
		Log(Error, "/FSConfig", "Config file size is too large");
		configFile.close();
		Serial_CLI();
		return false;
	}

	// Parrse json
	DynamicJsonBuffer jsonBuffer(size + 100);
	JsonObject& root = jsonBuffer.parseObject(configFile);

	// Close file
	configFile.close();

	// Load config into variables
	// ############### System ###############
	Hostname = root.get<String>("Hostname");
	System_Header = root.get<String>("System_Header");
	System_Sub_Header = root.get<String>("System_Sub_Header");
	// Rebuild topics to get naming right
	Rebuild_MQTT_Topics();

	Config_ID = root.get<String>("Config_ID");

	// Publish boot message just after rebuild
	Log(Info, "/System", "Booting Dobby - Wemos D1 Mini firmware version: " + String(Version) + " Free Memory: " + String(system_get_free_heap_size()) + " Config id: " + String(Config_ID));

	WiFi_SSID = root.get<String>("WiFi_SSID");
	WiFi_Password = root.get<String>("WiFi_Password");
	MQTT_Broker = root.get<String>("MQTT_Broker");
	MQTT_Port = root.get<String>("MQTT_Port");
	MQTT_Username = root.get<String>("MQTT_Username");
	MQTT_Password = root.get<String>("MQTT_Password");

	MQTT_KeepAlive_Interval = root.get<unsigned long>("MQTT_KeepAlive_Interval");
	

	// ############### Dimmer ###############
	if (root.get<String>("Dimmer_Pins") != "") {
		// Log event
		Log(Info, "/Dimmer", "Configuring");
		MQTT_Subscribe(MQTT_Topic[Topic_Dimmer], true, PLUS);
		FS_Config_Settings_Set(Dimmer_Pins, Dimmer_Max_Number_Of, root.get<String>("Dimmer_Pins"), "/Dimmer", "Dimmer Pins");
		// Set pinMode
		for (byte i = 0; i < Dimmer_Max_Number_Of; i++) {
			if (Dimmer_Pins[i] != 255) {
				if (Pin_Monitor(Reserve_Normal, Dimmer_Pins[i]) == Pin_Free) {
					pinMode(Dimmer_Pins[i], OUTPUT);
					analogWrite(Dimmer_Pins[i], 0);
					Dimmer_State[i] = 0;
					Dimmer_Configured = true;
					// Publish state change
					Log(Device, MQTT_Topic[Topic_Dimmer] + "/" + String(i + 1) + "/State", 0);
				}
			}
		}
		// Log event
		Log(Info, "/Dimmer", "Configuration compleate");
	}


	// ############### Relay ###############
	if (root.get<String>("Relay_On_State") != "" && root.get<String>("Relay_Pins") != "") {
		Relay_On_State = root.get<bool>("Relay_On_State");
		// Log event
		Log(Info, "/Relay", "Configuring");
		// Relay pins
		FS_Config_Settings_Set(Relay_Pins, Relay_Max_Number_Of, root.get<String>("Relay_Pins"), "/Relay", "Relay Pins");
		for (byte i = 0; i < Relay_Max_Number_Of; i++) {
			if (Relay_Pins[i] != 255 && Pin_Monitor(Reserve_Normal, Relay_Pins[i]) == Pin_Free) {
				pinMode(Relay_Pins[i], OUTPUT);
				digitalWrite(Relay_Pins[i], !Relay_On_State);
				// Publish state change
				Log(Device, MQTT_Topic[Topic_Relay] + "/" + String(i + 1) + "/State", 0);
			}
		}
		// Auto Off		
		if (root.get<String>("Relay_Pin_Auto_Off") != "" && root.get<String>("Relay_Pin_Auto_Off_Delay") != "") {
			FS_Config_Settings_Set(Relay_Pin_Auto_Off, Relay_Max_Number_Of, root.get<String>("Relay_Pin_Auto_Off"), "/Relay", "Relay Pins Auto Off");
			FS_Config_Settings_Set(Relay_Pin_Auto_Off_Delay, Relay_Max_Number_Of, root.get<String>("Relay_Pin_Auto_Off_Delay"), "/Relay", "Relay Pins Auto Off Delay");
		}
		// Subscribe to topics
		MQTT_Subscribe(MQTT_Topic[Topic_Relay], true, PLUS);
		Relay_Configured = true;
		// Log event
		Log(Info, "/Relay", "Configuration compleate");
	}


	// ############### Button ###############
	if (root.get<String>("Button_Pins") != "") {
		// Log event
		Log(Info, "/Button", "Configuring");
		FS_Config_Settings_Set(Button_Pins, Button_Max_Number_Of, root.get<String>("Button_Pins"), "/Button", "Button Pins");
		Button_Configured = true;
		// Set pinMode
		for (byte i = 0; i < Button_Max_Number_Of; i++) {
			if (Button_Pins[i] != 255) {
				if (Pin_Monitor(Reserve_Normal, Button_Pins[i]) == Pin_Free) {
					pinMode(Button_Pins[i], INPUT_PULLUP);
					// Publish state change
					Log(Device, MQTT_Topic[Topic_Button] + "/" + String(i + 1) + "/State", 0);
				}
			}
		}
		if (root.get<String>("Button_Target") != "") {
			FS_Config_Settings_Set(Button_Target, Button_Max_Number_Of, root.get<String>("Button_Target"), "/Button", "Button Target");
		}
		// Log event
		Log(Info, "/Button", "Configuration compleate");
	}


	// ############### Switch ###############
	if (root.get<String>("Switch_Pins") != "") {
		// Log event
		Log(Info, "/Switch", "Configuring");
		MQTT_Subscribe(MQTT_Topic[Topic_Switch], true, PLUS);
		FS_Config_Settings_Set(Switch_Pins, Switch_Max_Number_Of, root.get<String>("Switch_Pins"), "/Switch", "Switch Pins");

		Switch_Configured = true;
		// Set pinMode
		for (byte i = 0; i < Switch_Max_Number_Of; i++) {
			if (Switch_Pins[i] != 255) {
				if (Pin_Monitor(Reserve_Normal, Switch_Pins[i]) == Pin_Free) {
					pinMode(Switch_Pins[i], INPUT_PULLUP);
					// Read current state
					Switch_Last_State[i] = digitalRead(Switch_Pins[i]);
					// Publish state change
					Log(Device, MQTT_Topic[Topic_Switch] + "/" + String(i + 1) + "/State", 0);
				}
			}
		}

		if (root.get<String>("Switch_Target_ON") != "") {
			FS_Config_Settings_Set(Switch_Target_ON, Switch_Max_Number_Of, root.get<String>("Switch_Target_ON"), "/Switch", "Switch Target ON");
		}

		if (root.get<String>("Switch_Target_OFF") != "") {
			FS_Config_Settings_Set(Switch_Target_OFF, Switch_Max_Number_Of, root.get<String>("Switch_Target_OFF"), "/Switch", "Switch Target OFF");
		}
		// Log event
		Log(Info, "/Switch", "Configuration compleate");
	} // Switch


	// ############### MQ ###############
	if (root.get<String>("MQ_Pin_A0") != "") {
		Log(Info, "/MQ", "Configuring");

		// Check if pin is free
		if (Pin_Monitor(Reserve_Normal, Pin_To_Number(root.get<String>("MQ_Pin_A0"))) == Pin_Free) {
				// Set variable
				MQ_Pin_A0 = Pin_To_Number(root.get<String>("MQ_Pin_A0"));
				// Set pinmode
				pinMode(MQ_Pin_A0, INPUT);
				// Subscribe to topic
				MQTT_Subscribe(MQTT_Topic[Topic_MQ], true, NONE);
				// Set configured to true
				MQ_Configured = true;
				// Read once to set Min/Max referance
				MQ_Loop();
				// Set min max to current
				MQ_Value_Min = MQ_Current_Value;
				MQ_Value_Max = MQ_Current_Value;
				// Start ticker
				MQ_Ticker.attach_ms(MQ_Refresh_Rate, MQ_Loop);
				// Log configuration compleate
				Log(Info, "/MQ", "Configuration compleate");
				// Publish state change
				Log(Device, MQTT_Topic[Topic_MQ] + "/1/State", 0);
			}
		else {
			Log(Error, "/MQ", "Configuration failed pin in use");
		}
	}

	
	// ############### DHT ###############
	if (root.get<String>("DHT_Pins") != "") {
		// Fill variables
		Log(Info, "/DHT", "Configuring");
		FS_Config_Settings_Set(DHT_Pins, DHT_Max_Number_Of, root.get<String>("DHT_Pins"), MQTT_Topic[Topic_Log_Info] + "/DHT", "DHT Pins");

		// Check pins
		for (byte i = 0; i < DHT_Max_Number_Of; i++) {
			if (DHT_Pins[i] != 255 && Pin_Monitor(Reserve_Normal, DHT_Pins[i]) == Pin_Free) {
				// Subscribe
				MQTT_Subscribe(MQTT_Topic[Topic_DHT], true, PLUS);
				// Enable configuration
				DHT_Configured = true;
			}
		}
		Log(Info, "/DHT", "Configuration compleate");
	}

	// ############### PIR ###############
	if (root.get<String>("PIR_Pins") != "") {
		Log(Info, "/PIR", "Configuring PIR");

		// Set pin variables
		FS_Config_Settings_Set(PIR_Pins, PIR_Max_Number_Of, root.get<String>("PIR_Pins"), "/PIR", "PIR Pins");

		// Set Target ON variables
		if (root.get<String>("PIR_Topic") != "") {
			FS_Config_Settings_Set(PIR_Topic, PIR_Max_Number_Of, root.get<String>("PIR_Topic"), "/PIR", "PIR ON Topic");
		}

		if (root.get<String>("PIR_OFF_Payload") != "") {
			FS_Config_Settings_Set(PIR_OFF_Payload, PIR_Max_Number_Of, root.get<String>("PIR_OFF_Payload"), "/PIR", "PIR OFF Payload");
		}
		if (root.get<String>("PIR_ON_Payload_1") != "") {
			FS_Config_Settings_Set(PIR_ON_Payload[0], PIR_Max_Number_Of, root.get<String>("PIR_ON_Payload_1"), "/PIR", "PIR ON Payload State 1");
		}
		if (root.get<String>("PIR_ON_Payload_2") != "") {
			FS_Config_Settings_Set(PIR_ON_Payload[1], PIR_Max_Number_Of, root.get<String>("PIR_ON_Payload_2"), "/PIR", "PIR ON Payload State 2");
		}
		if (root.get<String>("PIR_ON_Payload_3") != "") {
			FS_Config_Settings_Set(PIR_ON_Payload[2], PIR_Max_Number_Of, root.get<String>("PIR_ON_Payload_3"), "/PIR", "PIR ON Payload State 3");
		}

		// Set OFF Delay variables
		if (root.get<String>("PIR_OFF_Delay") != "") {
			FS_Config_Settings_Set(PIR_OFF_Delay, PIR_Max_Number_Of, root.get<String>("PIR_OFF_Delay"), "/PIR", "PIR OFF Delay");
		}

		// Set pinMode
		for (byte i = 0; i < PIR_Max_Number_Of; i++) {
			if (PIR_Pins[i] != 255 && PIR_Topic[i] != "" && PIR_ON_Payload[0][i] != "" && PIR_OFF_Payload[i] != "" && PIR_OFF_Delay[i] != 0) {
				if (Pin_Monitor(Reserve_Normal, PIR_Pins[i]) == Pin_Free) {
					pinMode(Switch_Pins[i], INPUT);
				}
			}
			// Flash indicator LED if configured to indicate wormup
			if (Indicator_LED_Configured == true) Indicator_LED(LED_PIR, true);

			// Subscrib to topic
			MQTT_Subscribe(MQTT_Topic[Topic_PIR], true, PLUS);
			// Enable Configuration
			PIR_Configured = true;
		}
		// Log event
		Log(Info, "/PIR", "Configuration compleate");
	}

	// ############### DC_Voltmeter ###############
	if (root.get<String>("DC_Voltmeter_Pins") != "") {
		Log(Info, "/DC_Voltmeter", "Configuring");

		DC_Voltmeter_Pins = Pin_To_Number(root.get<String>("DC_Voltmeter_Pins"));
		// Check to see if pin is free
		if (Pin_Monitor(Reserve_Normal, DC_Voltmeter_Pins) == Pin_Free) {
			pinMode(DC_Voltmeter_Pins, INPUT);
			MQTT_Subscribe(MQTT_Topic[Topic_DC_Voltmeter], true, NONE);
		}

		if (root.get<String>("DC_Voltmeter_R1") != "") {
			DC_Voltmeter_R1 = root.get<float>("DC_Voltmeter_R1");
		}

		if (root.get<String>("DC_Voltmeter_R2") != "") {
			DC_Voltmeter_R2 = root.get<float>("DC_Voltmeter_R2");
		}

		if (root.get<String>("DC_Voltmeter_Curcit_Voltage") != "") {
			DC_Voltmeter_Curcit_Voltage = root.get<float>("DC_Voltmeter_Curcit_Voltage");
		}

		if (root.get<String>("DC_Voltmeter_Offset") != "") {
			DC_Voltmeter_Offset = root.get<float>("DC_Voltmeter_Offset");
		}

		DC_Voltmeter_Configured = true;

		Log(Info, "/DC_Voltmeter", "Configuration compleate");
	} // DC Voltmeter

	// ############### Flow ###############
	if (root.get<String>("Flow_Pins") != "") {
		Log(Info, "/Flow", "Configuring");

		MQTT_Subscribe(MQTT_Topic[Topic_Flow], true, PLUS);
		FS_Config_Settings_Set(Flow_Pins, Flow_Max_Number_Of, root.get<String>("Flow_Pins"), "Flow", "Flow Pins");

		Flow_Configured = true;
		// Set pinMode
		for (byte i = 0; i < Flow_Max_Number_Of; i++) {
			if (Flow_Pins[i] != 255) {
				if (Pin_Monitor(Reserve_Normal, Flow_Pins[i]) == Pin_Free) {
					pinMode(Flow_Pins[i], INPUT_PULLUP);
					// FIX - Find a better way to do this
					if (i == 0) attachInterrupt(Flow_Pins[i], Flow_Callback_0, FALLING);
					else if (i == 1) attachInterrupt(Flow_Pins[i], Flow_Callback_1, FALLING);
					else if (i == 2) attachInterrupt(Flow_Pins[i], Flow_Callback_2, FALLING);
					else if (i == 3) attachInterrupt(Flow_Pins[i], Flow_Callback_3, FALLING);
					else if (i == 4) attachInterrupt(Flow_Pins[i], Flow_Callback_4, FALLING);
					else if (i == 5) attachInterrupt(Flow_Pins[i], Flow_Callback_5, FALLING);
					// Publish "Boot" to /State topic to indicate reset of counter
					Log(Device, MQTT_Topic[Topic_Flow] + "/" + String(i + 1) + "/State", "Boot");
				}
			}
		}
		Log(Info, "/Flow", "Configuration compleate");
	} // Flow

	
	// ############### DS18B20 ###############
	if (root.get<bool>("DS18B20") == true) {
		// Log event
		Log(Info, "/DS18B20", "Configuring");
		// Mark as configured
		DS18B20_Configured = true;
		// Start up the library
		DS18B20_Sensors.begin();
		// Scan for devices
		DS18B20_Scan();
		// Set the setrWaitForConversion to false so DS18B20 reading does not crash the sketch
		DS18B20_Sensors.setWaitForConversion(false);
		// Start Ticker to refresh values
		DS18B20_Ticker.attach_ms(DS18B20_Refresh_Rate, DS18B20_Loop);
		// Subscribe to topics
		MQTT_Subscribe(MQTT_Topic[Topic_DS18B20], true, HASH);
		// Log event
		Log(Info, "/DS18B20", "Configuration compleate");
	} // DS18B20
	

	// ############### Distance ###############
  // FIX - Pretty up below
  if (root.get<String>("Distance_Pins_Trigger") != "") {
    Log(Info, "/Distance", "Configuring");
    FS_Config_Settings_Set(Distance_Pins_Trigger, Distance_Max_Number_Of, root.get<String>("Distance_Pins_Trigger"), "Distance", "Distance Pins Trigger");
    // Set pinMode
    for (byte i = 0; i < Distance_Max_Number_Of; i++) {
      if (Distance_Pins_Trigger[i] != 255 && Pin_Monitor(Reserve_Normal, Distance_Pins_Trigger[i]) == Pin_Free) {
        pinMode(Distance_Pins_Trigger[i], OUTPUT);
        Distance_Configured = true;
      }
    }

    if (root.get<String>("Distance_Pins_Echo") != "") {
      FS_Config_Settings_Set(Distance_Pins_Echo, Distance_Max_Number_Of, root.get<String>("Distance_Pins_Echo"), MQTT_Topic[Topic_Log_Info] + "Distance", "Distance Pins Echo");
      // Set pinMode
      for (byte i = 0; i < Distance_Max_Number_Of; i++) {
        if (Distance_Pins_Echo[i] != 255 && Pin_Monitor(Reserve_Normal, Distance_Pins_Echo[i]) == Pin_Free) {
          pinMode(Distance_Pins_Echo[i], INPUT);
        }
      }
    }

    if (root.get<String>("Distance_ON_At") != "") {
      FS_Config_Settings_Set(Distance_ON_At, Distance_Max_Number_Of, root.get<String>("Distance_ON_At"), "Distance", "Distance ON At");
    }
    if (root.get<String>("Distance_OFF_At") != "") {
      FS_Config_Settings_Set(Distance_OFF_At, Distance_Max_Number_Of, root.get<String>("Distance_OFF_At"), "Distance", "Distance OFF At");
    }
    if (root.get<String>("Distance_Target_ON") != "") {
      FS_Config_Settings_Set(Distance_Target_ON, Distance_Max_Number_Of, root.get<String>("Distance_Target_ON"), "Distance", "Distance Target ON");
    }
    if (root.get<String>("Distance_Target_OFF") != "") {
      FS_Config_Settings_Set(Distance_Target_OFF, Distance_Max_Number_Of, root.get<String>("Distance_Target_OFF"), "Distance", "Distance Target OFF");
    }

    // Subscribe to topic
    MQTT_Subscribe(MQTT_Topic[Topic_Distance], true, PLUS);
    Log(Info, "/Distance", "Configuration compleate");
  }

	// ############### Publish Uptime ###############
  if (root.get<String>("Publish Uptime") != "") {
    	Log(Info, "/PublishUptime", "Configuring");
		Publish_Uptime = root.get<bool>("Publish Uptime");

		if (Publish_Uptime == true) {
			Log(Debug, "/PublishUptime", "Publish Uptime interval set to: " + String(Publish_Uptime_Interval / 1000));
			// Set "Boot" to indicate counter starts from 0
			Log(Message, Topic_Uptime, "Boot");
		}
    	Log(Info, "/PublishUptime", "Configuration compleate");
	}

	return true;

} // FS_Config_Load()


// ############################################################ FS_List() ############################################################
void FS_List() {
	String str = "";
	Dir dir = SPIFFS.openDir("/");
	while (dir.next()) {
		str += dir.fileName();
		str += " / ";
		str += dir.fileSize();
		str += "\r\n";
	}
	Log(Info, "/FS/List", str);
} // FS_List


// ############################################################ FS_Format() ############################################################
void FS_Format() {
	Log(Debug, "/FS/Format", "SPIFFS Format started ... NOTE: Please wait 30 secs for SPIFFS to be formatted");
	SPIFFS.format();
	Log(Info, "/FS/Format", "SPIFFS Format compleate");
} // FS_Format()


// ############################################################ FS_cat() ############################################################
bool FS_cat(String File_Path) {

	if (SPIFFS.exists(File_Path)) {

		File f = SPIFFS.open(File_Path, "r");
		if (f && f.size()) {

			String cat_String;

			while (f.available()){
				cat_String += char(f.read());
			}

			f.close();

			Log(Info, "/FS/cat", cat_String);
			return true;
		}
	}

	return false;
} // FS_cat()


// ############################################################ FS_del() ############################################################
bool FS_del(String File_Path) {

	if (SPIFFS.exists(File_Path)) {
		if (SPIFFS.remove(File_Path) == true) {
			Log(Info, "/FS/del", File_Path);
			return true;
		}
		else {
			Log(Error, "/FS/del", "Unable to delete: " + File_Path);
			return false;
		}
	}
	return false;
} // FS_cat()


// ################################### FS_Commands() ###################################
bool FS_Commands(String Payload) {

	if (Payload == "Format") {
		FS_Format();
		return true;
	}
	else if (Payload == "List") {
		FS_List();
		return true;
	}
	else if (Payload.indexOf("cat /") != -1) {
		FS_cat(Payload.substring(String("cat ").length(), Payload.length()));
		return true;
	}
	else if (Payload.indexOf("del /") != -1) {
		FS_del(Payload.substring(String("del ").length(), Payload.length()));
		return true;
	}

	return false;
}


// ############################################################ Base_Config_Check() ############################################################
void Base_Config_Check() {

	if (Hostname == "" || Hostname == "/NotConfigured") {
		Serial_CLI();
	}

	if (WiFi_SSID == "") {
		Serial_CLI();
	}

	else if (WiFi_Password == "") {
		Serial_CLI();
	}

	else if (MQTT_Broker == "") {
		Serial_CLI();
	}

	else if (MQTT_Port == "") {
		Serial_CLI();
	}

	else if (MQTT_Username == "") {
		Serial_CLI();
	}

	else if (MQTT_Password == "") {
		Serial_CLI();
	}

	else if (System_Header == "") {
		Serial_CLI();
	}

	else {
		Log(Debug, "/Dobby", "Base config check done, all OK");
		return;
	}
}


// ############################################################ ArduinoOTA_Setup() ############################################################
void ArduinoOTA_Setup() {

	ArduinoOTA.setHostname(Hostname.c_str());
	ArduinoOTA.setPassword("StillNotSinking");

	ArduinoOTA.onStart([]() {
		ArduinoOTA_Active = true;
		MQTT_KeepAlive_Ticker.detach();
		String type;
		if (ArduinoOTA.getCommand() == U_FLASH) {
			type = "sketch";
		} else { // U_SPIFFS
			type = "filesystem";
		}

		// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
		Log(Info, "/ArduinoOTA", "Start updating " + type);
	});

	ArduinoOTA.onEnd([]() {
		if (ArduinoOTA.getCommand() == U_FLASH) {
			// If a new sketch was uploaded then reboot asap
			Log(Info, "/ArduinoOTA", "Sketch upload compleate. Rebooting");
			while(MQTT_TX_Queue_Topic.Queue_Is_Empthy == false){
				Log_Loop();
			}
			Serial.flush();
			MQTT_Client.disconnect();
		} else { // U_SPIFFS
			Log(Info, "/ArduinoOTA", "File uploaded to filesystem");
		}

		ArduinoOTA_Active = false;
	});

	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});

	ArduinoOTA.onError([](ota_error_t error) {
		if (error == OTA_AUTH_ERROR) {
			Log(Warning, "/ArduinoOTA", "Auth Failed");
		} else if (error == OTA_BEGIN_ERROR) {
			Log(Warning, "/ArduinoOTA", "Begin Failed");
		} else if (error == OTA_CONNECT_ERROR) {
			Log(Warning, "/ArduinoOTA", "Connect Failed");
		} else if (error == OTA_RECEIVE_ERROR) {
			Log(Warning, "/ArduinoOTA", "Receive Failed");
		} else if (error == OTA_END_ERROR) {
			Log(Warning, "/ArduinoOTA", "End Failed");
		}
		ArduinoOTA_Active = false;
	});

	ArduinoOTA.begin();

} // ArduinoOTA_Setup()


// ############################################################ WiFi_On_Connect() ############################################################
void WiFi_On_Connect() {
		// Log event
		Log(Info, "/WiFi", "Connected to SSID: '" + WiFi_SSID + "' - IP: '" + IP_To_String(WiFi.localIP()) + "' - MAC Address: '" + WiFi.macAddress() + "'");
		// Disable indicator led
		Indicator_LED(LED_WiFi, false);
		WiFi_Disconnect_Message_Send = false;
		// Just a slight delay to make sure everything is up before connecting to mqtt
		MQTT_Reconnect_Ticker.once_ms(250, MQTT_Connect);
		// FTP Server
		//username, password for ftp.	set ports in ESP8266FtpServer.h	(default 21, 50009 for PASV)
		// FTP_Server.begin("dobby","heretoserve");

} // WiFi_On_Connect()


// ############################################################ WiFi_Setup() ############################################################
void WiFi_Setup() {

	bool WiFi_Reset_Required = false;

	WiFi.hostname(Hostname);
	Log(Debug, "/WiFi", "Set Hostname to: " + Hostname);

	if (WiFi.getMode() != WIFI_STA) {
		Log(Debug, "/WiFi", "Changing 'mode' to 'WIFI_STA'");
		WiFi.mode(WIFI_STA);
		WiFi_Reset_Required = true;
	}

	if (WiFi.getAutoConnect() != true) {
		WiFi.setAutoConnect(true);
		Log(Debug, "/WiFi", "Changing 'AutoConnect' to 'true'");
	}

	if (WiFi.getAutoReconnect() != true) {
		WiFi.setAutoReconnect(true);
		Log(Debug, "/WiFi", "Changing 'AutoReconnect' to 'true'");
	}

	if (WiFi.SSID() != WiFi_SSID) {
		WiFi.SSID();
		Log(Debug, "/WiFi", "Changing 'SSID' to '" + WiFi_SSID + "'");
		WiFi_Reset_Required = true;
	}

	// WiFi_Client.setNoDelay(true);

	if (WiFi_Reset_Required == true) {
		Log(Debug, "/WiFi", "Reset required");
		WiFi.disconnect(false);
	}

	// Callbakcs
	gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event) {
		WiFi_On_Connect();
	});

	disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
		// Check if the disconnected message have already been send if it has do nothing.
		if (WiFi_Disconnect_Message_Send == false) {
			
			// Do nothing if its within the first 5 sec of boot
			if (millis() < 5000) {
				return;
			}
			// Reset Subscribtion
			for (byte i = 0; i < MQTT_Topic_Number_Of; i++) MQTT_Subscribtion_Active[i] = false;
			// Log event
			Log(Warning, "/WiFi", "Disconnected from SSID: " + WiFi_SSID);
			// Turn on led
			Indicator_LED(LED_WiFi, true);
			// Note event
			WiFi_Disconnect_Message_Send = true;
		}
	});

	// Check if WiFi is up
	// WiFi connected
	if (WiFi.status() != WL_CONNECTED) {
		Log(Debug, "/WiFi", "Connecting to SSID: " + WiFi_SSID);
		WiFi.begin(WiFi_SSID.c_str(), WiFi_Password.c_str());
	}
	// WiFi not connected
	else {
		WiFi_On_Connect();
	}

	// Set wifi to persistent so the device reconnects asap
	WiFi.persistent(true);

	Log(Debug, "/WiFi", "Configuration compleate");


} // WiFi_Setup()


// ############################################################ MQTT_Config_Check() ############################################################
bool MQTT_Config_Check(String &Payload) {
	// Generate a string with required setting and then check against it
	String Settings_List[8] = {"Hostname", "WiFi_SSID", "WiFi_Password", "MQTT_Broker", "MQTT_Port", "MQTT_Username", "MQTT_Password", "System_Header"};
	// Generate a string for logging	
	String Missing_String;
	
	for (byte i = 0; i < 8; i++)
	{
		// Check for each entry in the missing list
		if (Payload.indexOf("\"" + Settings_List[i] + "\":") == -1)
		{
			// Add entry to missing string if found
			Missing_String = Missing_String + Settings_List[i] + " ";
		}
	}

	if (Missing_String != "")
	{
		Log(Error, "/Config", "Config check failed, missing: " + Missing_String);
		// Reset Incompleate_Config and Incompleate_Config_Count
		Incompleate_Config = "";
		Incompleate_Config_Count = 0;
		return false;
	}
	
	return true;

} // MQTT_Config_Check()


// ############################################################ MQTT_Config() ############################################################
bool MQTT_Config(String Payload) {

	// Check config has the minimum required setting
	if (MQTT_Config_Check(Payload) == false) {
		return false;
	}

	File configFile = SPIFFS.open(FS_Confing_File_Name, "w");

	if (!configFile) {
		Log(Error, "/Config", "Failed to open config file for writing");
		return false;
	}

	configFile.print(Payload);
	configFile.close();

	Log(Info, "/Config", "Saved to SPIFFS reboot required, rebooting in 2 seconds");
	
	Reboot(2000);

	return true;

} // MQTT_Config


// ############################################################ MQTT_On_Connect() ############################################################
void MQTT_On_Connect(bool sessionPresent) {
	// Log event
	Log(Info, "/MQTT", "Connected to Broker: '" + MQTT_Broker + "'");

	Indicator_LED(LED_MQTT, false);

	// Subscribe to topics
	for (byte i = 0; i < MQTT_Topic_Number_Of; i++) {
		MQTT_Subscribe(MQTT_Topic[i], MQTT_Topic_Subscribe_Active[i], MQTT_Topic_Subscribe_Subtopic[i]);
	}
	// Start MQTT_KeepAlive
	MQTT_KeepAlive_Ticker.attach(MQTT_KeepAlive_Interval, MQTT_KeepAlive);

	if (MQTT_State == MQTT_State_Init) {
		// Request config
		Log(Message, MQTT_Topic[Topic_Dobby] + "Config", Hostname + "," + Config_ID + ",MQTT"); // Request config
	}
	
	// Change MQTT State
	MQTT_State = MQTT_State_Connected;

} // MQTT_On_Connect()


// ############################################################ MQTT_Connect() ############################################################
void MQTT_Connect() {

	// No reason to try to connect to broker while software is getting uploaded
	if (ArduinoOTA_Active == true) return;

	// Only send log connecting event once
	if (MQTT_State != MQTT_State_Connecting)
	{
		// Log event
		Log(Debug, "/MQTT", "Connecting to Broker: '" + MQTT_Broker + "' ...");
		// Try to connecto to mqtt broker
		MQTT_Client.connect();
	}
	else if (MQTT_State != MQTT_State_Init)
	{
		// Log event
		Log(Debug, "/MQTT", "Connecting to Broker: '" + MQTT_Broker + "' ...");
		// Try to connecto to mqtt broker
		MQTT_Client.connect();
		// Change MQTT State
		MQTT_State = MQTT_State_Connecting;
	}

} // MQTT_Connect()


// ############################################################ MQTT_On_Disconnect() ############################################################
void MQTT_On_Disconnect(AsyncMqttClientDisconnectReason reason) {
	// Only log event if been connected to broker already
	if (MQTT_State != MQTT_State_Init && MQTT_State != MQTT_State_Disconnected) {
		// Log event
		Log(Error, "/MQTT", "Disconnected from Broker: '" + MQTT_Broker + "' Reason: " + Async_Disconnect_Reson(static_cast<int>(reason)));
		// Change MQTT State
		MQTT_State = MQTT_State_Disconnected;
		// Stop keepalive ticket
		MQTT_KeepAlive_Ticker.detach();
		// Change indicator led to reflect MQTT disconnected
		Indicator_LED(LED_MQTT, true);
		// Mark topics and unsubscribe
		for (byte i = 0; i < MQTT_Topic_Number_Of; i++) MQTT_Subscribtion_Active[i] = false;
	}
	// If wifi is connected try to connect again in a bit
	if (WiFi.isConnected() == true) {
		MQTT_Reconnect_Ticker.once_ms(MQTT_Reconnect_Interval, MQTT_Connect);
	}

} // MQTT_On_Disconnect()


// ############################################################ High_Low_String() ############################################################
void Rebuild_MQTT_Topics() {
	// System
	MQTT_Topic[Topic_Config] = System_Header + Topic_Config_Text + Hostname;
	MQTT_Topic[Topic_Commands] = System_Header + Topic_Commands_Text + Hostname;
	MQTT_Topic[Topic_All] = System_Header + Topic_All_Text;
	MQTT_Topic[Topic_KeepAlive] = System_Header + Topic_KeepAlive_Text + Hostname;
	MQTT_Topic[Topic_Dobby] = System_Header + Topic_Dobby_Text;
	// Log
	MQTT_Topic[Topic_Log_Debug] = System_Header + "/Log/" + Hostname + Topic_Log_Debug_Text;
	MQTT_Topic[Topic_Log_Info] = System_Header + "/Log/" + Hostname + Topic_Log_Info_Text;
	MQTT_Topic[Topic_Log_Warning] = System_Header + "/Log/" + Hostname + Topic_Log_Warning_Text;
	MQTT_Topic[Topic_Log_Error] = System_Header + "/Log/" + Hostname + Topic_Log_Error_Text;
	MQTT_Topic[Topic_Log_Fatal] = System_Header + "/Log/" + Hostname + Topic_Log_Fatal_Text;
	MQTT_Topic[Topic_Log_Will] = System_Header + "/Log/" + Hostname + Topic_Log_Will_Text;
	// Devices
	MQTT_Topic[Topic_Ammeter] = System_Header + System_Sub_Header + Topic_Ammeter_Text + Hostname;
	MQTT_Topic[Topic_Button] = System_Header + System_Sub_Header + Topic_Button_Text + Hostname;
	MQTT_Topic[Topic_DHT] = System_Header + System_Sub_Header + Topic_DHT_Text + Hostname;
	MQTT_Topic[Topic_DC_Voltmeter] = System_Header + System_Sub_Header + Topic_DC_Voltmeter_Text + Hostname;
	MQTT_Topic[Topic_Dimmer] = System_Header + System_Sub_Header + Topic_Dimmer_Text + Hostname;
	MQTT_Topic[Topic_Distance] = System_Header + System_Sub_Header + Topic_Distance_Text + Hostname;
	MQTT_Topic[Topic_MQ] = System_Header + System_Sub_Header + Topic_MQ_Text + Hostname;
	MQTT_Topic[Topic_Relay] = System_Header + System_Sub_Header + Topic_Relay_Text + Hostname;
	MQTT_Topic[Topic_PIR] = System_Header + System_Sub_Header + Topic_PIR_Text + Hostname;
	MQTT_Topic[Topic_MAX31855] = System_Header + System_Sub_Header + Topic_MAX31855_Text + Hostname;
	MQTT_Topic[Topic_Switch] = System_Header + System_Sub_Header + Topic_Switch_Text + Hostname;
	MQTT_Topic[Topic_Flow] = System_Header + System_Sub_Header + Topic_Flow_Text + Hostname;
	MQTT_Topic[Topic_DS18B20] = System_Header + System_Sub_Header + Topic_DS18B20_Text + Hostname;
}


// ############################################################ MQTT_Subscribe() ############################################################
void MQTT_Subscribe(String Topic, bool Activate_Topic, byte SubTopics) {

	if (ArduinoOTA_Active == true) {
		return;
	}

	byte Topic_Number = 255;

	for (byte i = 0; i < MQTT_Topic_Number_Of; i++) {
		if (Topic == MQTT_Topic[i]) {
			Topic_Number = i;
			break;
		}
	}
	if (Topic_Number == 255) {
		Log(Error, "/MQTT", "Unknown Subscribe Topic: " + Topic);
		return;
	}

	MQTT_Topic_Subscribe_Active[Topic_Number] = Activate_Topic;
	MQTT_Topic_Subscribe_Subtopic[Topic_Number] = SubTopics;

	// Check if MQTT is connected
	if (MQTT_Client.connected() == false) {
		// if not then do nothing
		return;
	}

	String Subscribe_String = MQTT_Topic[Topic_Number];

	if (MQTT_Subscribtion_Active[Topic_Number] == false && MQTT_Topic_Subscribe_Active[Topic_Number] == true) {

		// Check if already subscribed
		if (MQTT_Subscribtion_Active[Topic_Number] == true && MQTT_Topic_Subscribe_Active[Topic_Number] == true) {
			Log(Warning, "/MQTT", "Already subscribed to Topic: " + Subscribe_String);
			return;
		}
		// Add # or + to topic
		if (MQTT_Topic_Subscribe_Subtopic[Topic_Number] == PLUS) Subscribe_String = Subscribe_String + "/+";
		else if (MQTT_Topic_Subscribe_Subtopic[Topic_Number] == HASH) Subscribe_String = Subscribe_String + "/#";

		// Try to subscribe
		if (MQTT_Client.subscribe(Subscribe_String.c_str(), 0)) {
			Log(Info, "/MQTT", "Subscribing to Topic: " + Subscribe_String + " ... OK");
			MQTT_Subscribtion_Active[Topic_Number] = true;
		}
		// Log failure
		else {
			Log(Error, "/MQTT", "Subscribing to Topic: " + Subscribe_String + " ... FAILED");
		}
	}
}


// ############################################################ MQTT_Setup() ############################################################
void MQTT_Setup() {

	// All varuables used belows has to be specified as global variables!!!!
	// or elses the Async_MQTT_Client FUCKS up ever so badly

	MQTT_Client.onConnect(MQTT_On_Connect);
	MQTT_Client.onDisconnect(MQTT_On_Disconnect);
	MQTT_Client.onMessage(MQTT_On_Message);

	MQTT_Client.setServer(String_To_IP(MQTT_Broker), MQTT_Port.toInt());
	MQTT_Client.setCredentials(MQTT_Username.c_str(), MQTT_Password.c_str());

	// MQTT_Hostname has to have a random part because the devices reboot so fast that the broker gets confused
	MQTT_Hostname = Hostname + "-" + String(getRandomNumber(10000, 19999));
	MQTT_Client.setClientId(MQTT_Hostname.c_str());

	// setWill seems to make the async-mqtt-client unstable
	MQTT_Client.setWill(MQTT_Topic[Topic_Log_Will].c_str(), 0, false, "Disconnected");

} // MQTT_Setup()

// ############################################################ MQTT_KeepAlive() ############################################################
void MQTT_KeepAlive() {

	// If the MQTT Client is not connected no reason to send try to send a keepalive
	// Dont send keepalives during updates
	if (MQTT_State != MQTT_State_Connected || ArduinoOTA_Active == true) return;

	// Create json buffer
	DynamicJsonBuffer jsonBuffer(220);
	JsonObject& root_KL = jsonBuffer.createObject();

	// encode json string
	root_KL.set("Hostname", Hostname);
	root_KL.set("IP", IP_To_String(WiFi.localIP()));
	root_KL.set("Uptime", millis());
	root_KL.set("FreeMemory", system_get_free_heap_size());
	root_KL.set("Software", Version);
	root_KL.set("IP", IP_To_String(WiFi.localIP()));
	root_KL.set("RSSI", WiFi.RSSI());
	root_KL.set("Hardware", String(ARDUINO_BOARD));

	String KeepAlive_String;

	root_KL.printTo(KeepAlive_String);

	Log(Message, MQTT_Topic[Topic_KeepAlive], KeepAlive_String);

} // MQTT_KeepAlive()


// ############################################################ WiFi_Signal() ############################################################
// Post the devices WiFi Signal Strength
void WiFi_Signal() {

	Log(Info, "/WiFi", "Signal Strength: " + String(WiFi.RSSI()));

} // WiFi_Signal()


// ############################################################ IP_Show() ############################################################
// Post the devices IP information
void IP_Show() {

	String IP_String;

	IP_String = "IP Address: " + IP_To_String(WiFi.localIP());
	IP_String = IP_String + " Subnetmask: " + IP_To_String(WiFi.subnetMask());
	IP_String = IP_String + " Gateway: " + IP_To_String(WiFi.gatewayIP());
	IP_String = IP_String + " DNS Server: " + IP_To_String(WiFi.dnsIP());
	IP_String = IP_String + " MAC Address: " + WiFi.macAddress();

	Log(Info, "/IP", IP_String);
} // IP_Show()


// ############################################################ Pin_Monitor_State() ############################################################
void Pin_Monitor_State() {

	String Return_String;

	for (byte i = 0; i < Pin_Monitor_Pins_Number_Of; i++) {
		Return_String = Return_String + Pin_Monitor_Pins_Names[i] + ": " + Pin_Monitor_State_Text[Pin_Monitor_Pins_Active[i]];

		if (i != Pin_Monitor_Pins_Number_Of - 1) {
			Return_String = Return_String + " - ";
		}
	}

	Log(Info, "/PinMonitor", Return_String);
} // Pin_Monitor_State()


// ############################################################ Pin_Monitor_Map() ############################################################
void Pin_Monitor_Map() {

	String Pin_String =
		"Pin Map: D0=" + String(D0) +
		" D1=" + String(D1) +
		" D2=" + String(D2) +
		" D3=" + String(D3) +
		" D4=" + String(D4) +
		" D5=" + String(D5) +
		" D6=" + String(D6) +
		" D7=" + String(D7) +
		" D8=" + String(D8) +
		" A0=" + String(A0);

		Log(Info, "/PinMap", Pin_String);

} // Pin_Monitor_Map()


// ############################################################ Pin_Monitor_String() ############################################################
// Will return false if pin is in use or invalid
String Number_To_Pin(byte Pin_Number) {

	if (Pin_Number == 16) return "D0";
	else if (Pin_Number == 5) return "D1";
	else if (Pin_Number == 4) return "D2";
	else if (Pin_Number == 0) return "D3";
	else if (Pin_Number == 2) return "D4";
	else if (Pin_Number == 14) return "D5";
	else if (Pin_Number == 12) return "D6";
	else if (Pin_Number == 13) return "D7";
	else if (Pin_Number == 15) return "D8";
	else if (Pin_Number == 17) return "A0";
	else if (Pin_Number == 255) return "Unconfigured";
	else return "Unknown Pin";

} // Pin_Monitor_String()


byte Pin_To_Number(String Pin_Name) {

	if (Pin_Name == "D0") return 16;
	else if (Pin_Name == "D1") return 5;
	else if (Pin_Name == "D2") return 4;
	else if (Pin_Name == "D3") return 0;
	else if (Pin_Name == "D4") return 2;
	else if (Pin_Name == "D5") return 14;
	else if (Pin_Name == "D6") return 12;
	else if (Pin_Name == "D7") return 13;
	else if (Pin_Name == "D8") return 15;
	else if (Pin_Name == "A0") return 17;
	else if (Pin_Name == "Unconfigured") return 255;
	else return 254;

} // Pin_Monitor_String()


// ############################################################ Pin_Monitor() ############################################################
// Will return false if pin is in use or invalid
byte Pin_Monitor(byte Action, byte Pin_Number) {
	// 0 = In Use
	// 1 = Free
	// 2 = Free / In Use - I2C - SCL
	// 3 = Free / In Use - I2C - SDA
	// 4 = Free / In Use - OneWire
	// #define Pin_In_Use 0
	// #define Pin_Free 1
	// #define Pin_SCL 2
	// #define Pin_SDA 3
	// #define Pin_OneWire 4
	// #define Pin_Error 255

	// Pin_Number
	// 0 = Reserve - Normal Pin
	// 1 = Reserve - I2C SCL
	// 2 = Reserve - I2C SDA
	// 3 = State
	// 4 = Reserve - OneWire
	// #define Reserve_Normal 0
	// #define Reserve_I2C_SCL 1
	// #define Reserve_I2C_SDA 2
	// #define Check_State 3
	// #define Reserve_OneWire 4


	// Check if pin has been set
	if (Pin_Number == 255) {
		Log(Error, "/PinMonitor", "Pin not set");
		return Pin_Error;
	}


	// Check if pin has been set
	// Pin_to_Number returns 254 if unknown pin name
	if (Pin_Number == 254) {
		Log(Error, "/PinMonitor", "Unknown Pin Name given");
		return Pin_Error;
	}


	// Find seleced Pin
	byte Selected_Pin = 255;
	for (byte i = 0; i < Pin_Monitor_Pins_Number_Of; i++) {
		if (Pin_Number == Pin_Monitor_Pins_List[i]) {
			Selected_Pin = i;
			break;
		}
	}

	// Known pin check
	if (Selected_Pin == 255) {
		Log(Error, "/PinMonitor", "Pin number: " + String(Pin_Number) + " Pin Name: " + Number_To_Pin(Pin_Number) + " not on pin list");
		return Pin_Error;
	}

	// Reserve a normal pin
	if (Action == Reserve_Normal) {
		// Check if pin is free
		// Pin is free
		if (Pin_Monitor_Pins_Active[Selected_Pin] == Pin_Free) {
			// Reserve pin
			Pin_Monitor_Pins_Active[Selected_Pin] = Pin_In_Use;
			// Log event
			Log(Debug, "/PinMonitor", "Pin " + Number_To_Pin(Pin_Number) + " is free");

			// Disable indicator led if pin D4 is used
			if (Number_To_Pin(Pin_Number) == "D4") {
				Log(Debug, "/PinMonitor", "Pin D4 used, disabling indicator LED");

				// Detatch the tickers
				Indicator_LED_Blink_Ticker.detach();
				Indicator_LED_Blink_OFF_Ticker.detach();
				// Set port low for good mesure, and yes high is low
				digitalWrite(D4, HIGH);
				// Disable indicator LED
				Indicator_LED_Configured = false;
			}

			// Return Pin Free
			return Pin_Free;
		}
		// Pin is use return what it is used by
		else {
			// Log event
			Log(Error, "/PinMonitor", "Pin " + Number_To_Pin(Pin_Number) + " is in use");
			// Return state
			return Pin_Monitor_Pins_Active[Selected_Pin];
		}
	}

	// Reserve a I2C SCL Pin
	else if (Action == Reserve_I2C_SCL) {
		// Pin is free
		if (Pin_Monitor_Pins_Active[Selected_Pin] == Pin_Free) {
			// Reserve pin
			Pin_Monitor_Pins_Active[Selected_Pin] = Pin_SCL;
			// Log event
			Log(Debug, "/PinMonitor", "Pin " + Number_To_Pin(Pin_Number) + " is free");
			// Return state
			return Pin_SCL;
		}
		// Pin already used as I2C SCL
		else if (Pin_Monitor_Pins_Active[Selected_Pin] == Pin_SCL) {
			// Log event
			Log(Debug, "/PinMonitor", "Pin " + Number_To_Pin(Pin_Number) + " already used as I2C SCL");
			// Return state
			return Pin_SCL;
		}
		// Pin is in use
		else {
			// Log event
			Log(Error, "/PinMonitor", "Pin " + Number_To_Pin(Pin_Number) + " is in use");
			// Return state
			return Pin_Monitor_Pins_Active[Selected_Pin];
		}
	}

	// Reserve a I2C SDA Pin
	else if (Action == Reserve_I2C_SDA) {
		// Pin is free
		if (Pin_Monitor_Pins_Active[Selected_Pin] == Pin_Free) {
			// Reserve pin
			Pin_Monitor_Pins_Active[Selected_Pin] = Pin_SDA;
			// Log event
			Log(Debug, "/PinMonitor", "Pin " + Number_To_Pin(Pin_Number) + " is free");
			// Return state
			return Pin_SDA;
		}
		// Pin already used as I2C SDA
		else if (Pin_Monitor_Pins_Active[Selected_Pin] == Pin_SDA) {
			// Log event
			Log(Debug, "/PinMonitor", "Pin " + Number_To_Pin(Pin_Number) + " already used as I2C SDA");
			// Return state
			return Pin_SDA;
		}
		// Pin is in use
		else {
			// Log event
			Log(Error, "/PinMonitor", "Pin " + Number_To_Pin(Pin_Number) + " is in use");
			// Return state
			return Pin_Monitor_Pins_Active[Selected_Pin];
		}
	}

	// Reserve a OneWire Pin
	else if (Action == Reserve_OneWire) {
		// Pin is free
		if (Pin_Monitor_Pins_Active[Selected_Pin] == Pin_Free) {
			// Reserve pin
			Pin_Monitor_Pins_Active[Selected_Pin] = Pin_OneWire;
			// Log event
			Log(Debug, "/PinMonitor", "Pin " + Number_To_Pin(Pin_Number) + " is free");
			// Return state
			return Pin_OneWire;
		}
		// Pin already used as OneWire
		else if (Pin_Monitor_Pins_Active[Selected_Pin] == Pin_OneWire) {
			// Log event
			Log(Debug, "/PinMonitor", "Pin " + Number_To_Pin(Pin_Number) + " already used as OneWire");
			// Return state
			return Pin_OneWire;
		}
		// Pin is in use
		else {
			// Log event
			Log(Error, "/PinMonitor", "Pin " + Number_To_Pin(Pin_Number) + " is in use");
			// Return state
			return Pin_Monitor_Pins_Active[Selected_Pin];
		}
	}

	// Check pin current state
	else if (Action == Check_State) {
		return Pin_Monitor_Pins_Active[Selected_Pin];
	}

	// FIX - Add error handling for wrong "Action"

	// Some error handling
	Log(Error, "/PinMonitor", "Reached end of loop with no hit this should not happen");
	return Pin_Error;
} // Pin_Monitor

// Refferance only - Pin Name
byte Pin_Monitor(byte Action, String Pin_Name) {
	return Pin_Monitor(Action, Pin_To_Number(Pin_Name));
} // Pin_Monitor - Refferance only - Pin Name


// ############################################################ MQTT_On_Message() ############################################################
void MQTT_On_Message(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
		// Add to incomming aka rx queue
		MQTT_RX_Queue_Topic.Push(topic);
		MQTT_RX_Queue_Payload.Push(String(payload).substring(0, len));
} // MQTT_On_Message()



	
float getVPP() {
  float result;
  
  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here
  int minValue = 1024;          // store min value here
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue = analogRead(A0);
       // see if you have a new maxValue
       if (readValue > maxValue) 
       {
           /*record the maximum sensor value*/
           maxValue = readValue;
       }
       if (readValue < minValue) 
       {
           /*record the maximum sensor value*/
           minValue = readValue;
       }
   }
   
   // Subtract min from max
   result = ((maxValue - minValue) * 3.0)/1024.0;
      
   return result;
 }



// ################################### MQTT_Commands() ###################################
bool MQTT_Commands(String &Topic, String &Payload) {

	bool Unknown_Command = false;

	// Ignore none commands
	if (Topic.indexOf(MQTT_Topic[Topic_Commands]) == -1) {
		return false;
	}

	Topic.replace(MQTT_Topic[Topic_Commands] + "/", "");


	// ############################### Config ###############################
	// Cache config here so we dont need to store the config in memory
	if (Topic == "Config") {
		// If the message is larger to large then it will be split into multipled messages.
		// Check for } at the end of the message to see if we got the last of the config.
		// If not store it and set marker for incompleate config

		// Compleate config or end of incompleate config recived
		if (Payload.indexOf("}") != -1)
		{
			// Compleate config recived, noting is store in Incompleate_Config
			if (Incompleate_Config == "")
			{
				// Log event
				Log(Info, "/Config", "MQTT config received");
				MQTT_Config(Payload);
			}
			// Last part of incompleate config recived
			else
			{
				// Log event
				Log(Info, "/Config", "MQTT config last part: " + String(Incompleate_Config_Count + 1) + " received applying config");
				// Store config
				MQTT_Config(Incompleate_Config + Payload);
			}
		}
		// Incompleate config recived aka only part of the config
		else
		{
			// Add to counter
			Incompleate_Config_Count = Incompleate_Config_Count + 1;
			// Log event
			Log(Debug, "/Config", "MQTT config part: " + String(Incompleate_Config_Count) + " received");
			// Save part of config
			Incompleate_Config = Incompleate_Config + Payload;
		}
		
	}

	// ############################### DS18B20 - Scan ###############################
	else if (Topic == "DS18B20")
	{	
		if (Payload == "Scan") 
		{	
			DS18B20_Scan();
		}
		else
		{
			Unknown_Command = true;
		}
		
	}

	// ############################### Log Level ###############################
	else if (Topic == "LogLevel")
	{	
		// Convert string level to byte level
		byte Log_Level_Byte = Log_Level_To_Byte(Payload);
		// Check if the log level string = a valid log level byte
		if (Log_Level_To_Byte(Payload) != -1)
		{
			if (Log_Level_Byte != System_Log_Level)	
			{
				Log(Info, "/LogLevel", "Changed to: " + Payload);
				System_Log_Level = Log_Level_Byte;
			}
			else
			{
				Log(Info, "/LogLevel", "Already set to: " + Payload);
			}
		}
		else
		{
			Log(Info, "/LogLevel", "Unknown log level: " + Payload);
		}
	}
	
	else if (Topic == "Power") {
		if (Payload.indexOf("Reboot") != -1) {
			Reboot(10000);
		}
		else if (Payload.indexOf("Shutdown") != -1) {
			Shutdown(10000);
		}
		else Unknown_Command = true;
	}

	else if (Topic == "FS") {
		if (FS_Commands(Payload) == true) return true;
		else Unknown_Command = true;
	}

	else if (Topic == "Blink") {
		Indicator_LED_Blink(Payload.toInt());
	}

	// ######################## Publish_Uptime ########################
	else if (Topic == "PublishUptime") {
		
		Payload.toLowerCase();

		if (Payload == "reset")
		{
			// Save current state to offset so it can be subtracted from millis
			Publish_Uptime_Offset = millis();
			// Publish reset so it gets logged in the db for counter use
			Log(Message, Topic_Uptime, "Reset");
			// Log event
			Log(Info, "/PublishUptime", "Reset");
		}
	}

	else if (Topic == "Hostname") {
		Hostname = Payload;
		FS_Config_Drop();
		Log(Info, "/System", "Hostname changed to: '" + Hostname + "' Reboot required rebooting in 2 seconds");
		Reboot(2000);
	}

	else if (Topic == "Version") {
		if (Payload == "Show") Log(Info, "/System", "Running Dobby - Wemos D1 Mini firmware version: " + String(Version));
		// if (Payload == "Update") Version_Update();
		else Unknown_Command = true;
	}

	else if (Topic == "IP") {
		if (Payload == "Show") IP_Show();
		else Unknown_Command = true;
	}

	// else if (Topic == "Dimmer/FadeJump") {
	//	 Dimmer_Fade_Jump = Payload.toInt();
	//	 Log(Debug, "/Dimmer", "Dimmer Fade Jump changed to: " + String(Dimmer_Fade_Jump));
	// }

	// else if (Topic == "Dimmer/FadeJumpDelay") {
	//	 Dimmer_Fade_Jump_Delay = Payload.toInt();
	//	 Log(Debug, "/Dimmer", "Dimmer Fade Jump Delay changed to: " + String(Dimmer_Fade_Jump_Delay));
	// } // Dimmer


	else if (Topic == "FSConfig") {
		if (Payload == "Save") FS_Config_Save();
		else if (Payload == "Show") FS_Config_Show();
		else if (Payload == "Drop") FS_Config_Drop();
		else if (Payload == "Request")
		{
			// Log event
			Log(Debug, "/System/", "Requesting config");
			// Request config
			Log(Message, MQTT_Topic[Topic_Dobby] + "Config", Hostname + "," + Config_ID + ",MQTT");
		}
		else if (Payload == "Request -f")
		{
			// Log event
			Log(Debug, "/System/", "Requesting config with config id 0");
			// Request config
			Log(Message, MQTT_Topic[Topic_Dobby] + "Config", Hostname + ",0,MQTT");
		}
		else Unknown_Command = true;
	}


	else if (Topic == "PinMonitor") {
		if (Payload == "State") Pin_Monitor_State();
		if (Payload == "Map") Pin_Monitor_Map();
		else Unknown_Command = true;
	}


	else if (Topic == "WiFi") {
		if (Payload == "Signal") WiFi_Signal();
		else Unknown_Command = true;
	}


	// else if (Topic == "Crash") {
	// 	if (Payload == "http") {
	// 		Log(Info, "/Crash", "HTTP server started: http://" + IP_To_String(WiFi.localIP()) + "/");
	// 		Crash_Server.begin(80);
	// 		Crash_Server_Enabeled = true;
	// 	}
	// 	else if (Payload == "Count") {
	// 		Log(Info, "/Crash", "Error count: " + String(Fatal::count()));
	// 		Crash_Server.stop();
	// 		Crash_Server_Enabeled = false;
	// 	}
	// 	else if (Payload == "Clear") {
	// 		Log(Info, "/Crash", "Cleared");
	// 		Fatal::clear();
	// 	}
	// 	else Unknown_Command = true;
	// }


	else if (Topic == "Test") {

		Log(Message, "/Test", "MARKER");

		if (Payload.indexOf("Crash") == -1)
		{
			Serial.println(1 / 0);
		}
		 
		
		Serial.println(ESP.getResetReason());
		Serial.println(ESP.getResetInfo());


		// Serial.println("Attempting to divide by zero ...");
        // int result, zero;
        // zero = 0;
        // result = 1 / zero;
        // Serial.print("Result = ");
        // Serial.println(result);
	

		// Serial.println(Payload);
		// int mVperAmp = 185; // use 100 for 20A Module and 66 for 30A Module
		// double Voltage = 0;
		// double VRMS = 0;
		// double AmpsRMS = 0;
		// Voltage = getVPP();
		// VRMS = (Voltage/2.0) *0.707; 
		// AmpsRMS = (VRMS * 1000)/mVperAmp;
		// Serial.print(AmpsRMS);
		// Serial.println(" Amps RMS");
		// Log("/Test", String(AmpsRMS));
	
	} // Test

	if (Unknown_Command == true) {
		Log(Debug, "/Commands", "Unknown command. " + Topic + " - " + Payload);
		return false;
	}

	return true;

} // MQTT_Commands()


// ############################################################ MQTT_Queue_Check() ############################################################
void MQTT_Queue_Check() {

	if (MQTT_RX_Queue_Topic.Queue_Is_Empthy == false) {

		String Topic = MQTT_RX_Queue_Topic.Pop();
		String Payload = MQTT_RX_Queue_Payload.Pop();

		// Log incomming message to serial
		Serial.println("RX - " + Topic + " - " + Payload);

		if (MQTT_Commands(Topic, Payload) == true) return;
		else if (Relay(Topic, Payload) == true) return;
		else if (Dimmer(Topic, Payload) == true) return;
		else if (Switch(Topic, Payload) == true) return;
		else if (DHT(Topic, Payload) == true) return;
		else if (DC_Voltmeter(Topic, Payload) == true) return;
		else if (PIR(Topic, Payload) == true) return;
		else if (DS18B20(Topic, Payload) == true) return;
		else if (Distance(Topic, Payload) == true) return;
		else if (MQ(Topic, Payload) == true) return;
		else if (Flow(Topic, Payload) == true) return;
	}
	
} // MQTT_Queue_Check()




// ############################################################ Crash_Loop() ############################################################
// void Crash_Loop() {
// 	// Do nothing if OTA running and if crash server is not enabeled
// 	if (Crash_Server_Enabeled == false || ArduinoOTA_Active == true) {
// 		return;
// 	}

// 	// read line by line what the client (web browser) is requesting
// 	WiFiClient client = Crash_Server.available();
// 	if (client) {
// 		Log(Debug, "/Crash", "Client connected");
// 		while (client.connected()) {
// 			// read line by line what the client (web browser) is requesting
// 			if (client.available()) {
// 				String line = client.readStringUntil('\r');
// 				// look for the end of client's request, that is marked with an empty line
// 				if (line.length() == 1 && line[0] == '\n')
// 				{
// 					// send response header to the web browser
// 					client.print("HTTP/1.1 200 OK\r\n");
// 					client.print("Content-Type: text/plain\r\n");
// 					client.print("Connection: close\r\n");
// 					client.print("\r\n");
// 					// send crash information to the web browser
// 					Fatal::print(client);
// 					break;
// 				}
// 			}
// 		}
// 		delay(1); // give the web browser time to receive the data
// 				  // close the connection:
// 		client.stop();
// 		Log(Debug, "/Crash", "Client disconnected");
// 		// Stop Crash server
// 		Crash_Server.stop();
// 		// Disable crash loop
// 		Crash_Server_Enabeled = false;
// 	}

// 	// // read the keyboard

// 	// if (Serial.available()) {
// 	// 	switch (Serial.read()) {
// 	// 		case 'p':
// 	// 			Fatal::print();
// 	// 			break;
// 	// 		case 'c':
// 	// 			Fatal::clear();
// 	// 			Serial.println("All clear");
// 	// 			break;
// 	// 		case 'r':
// 	// 			Serial.println("Reset");
// 	// 			ESP.reset();
// 	// 			// nothing will be saved in EEPROM
// 	// 			break;
// 	// 		case 't':
// 	// 			Serial.println("Restart");
// 	// 			ESP.restart();
// 	// 			// nothing will be saved in EEPROM
// 	// 			break;
// 	// 		case 'h':
// 	// 			Serial.printf("Hardware WDT");
// 	// 			ESP.wdtDisable();
// 	// 			while (true) {
// 	// 				; // block forever
// 	// 			}
// 	// 			// nothing will be saved in EEPROM
// 	// 			break;
// 	// 		case 's':
// 	// 			Serial.printf("Software WDT (exc.4)");
// 	// 			while (true) {
// 	// 				; // block forever
// 	// 			}
// 	// 			break;
// 	// 		case '0':
// 	// 			Serial.println("Divide by zero (exc.0)");
// 	// 			{
// 	// 				int zero = 0;
// 	// 				volatile int result = 1 / 0;
// 	// 			}
// 	// 			break;
// 	// 		case 'e':
// 	// 			Serial.println("Read from NULL ptr (exc.28)");
// 	// 			{
// 	// 				volatile int result = *(int *)NULL;
// 	// 			}
// 	// 			break;
// 	// 		case 'w':
// 	// 			Serial.println("Write to NULL ptr (exc.29)");
// 	// 			*(int *)NULL = 0;
// 	// 			break;
// 	// 		case '\n':
// 	// 		case '\r':
// 	// 			break;
// 	// 		default:
// 	// 			_menu();
// 	// 			break;
// 	// 	}
// 	// }



// 	// WiFiClient Web_Client = Crash_Server.available();
// 	// if (Web_Client) {
// 	// 	Log(Debug, "/Crash", "Client connected");
// 	// 	// an http request ends with a blank line
// 	// 	bool currentLineIsBlank = true;
// 	// 	while (Web_Client.connected()) {
// 	// 		if (Web_Client.available()) {
// 	// 			char c = Web_Client.read();
// 	// 			// if you've gotten to the end of the line (received a newline
// 	// 			// character) and the line is blank, the http request has ended,
// 	// 			// so you can send a reply
// 	// 			if (c == '\n' && currentLineIsBlank) {
// 	// 				// send a standard http response header
// 	// 				Web_Client.println("HTTP/1.1 200 OK");
// 	// 				Web_Client.println("Content-Type: text/html");
// 	// 				Web_Client.println("Connection: close");  // the connection will be closed after completion of the response
// 	// 				Web_Client.println("Refresh: 5");  // refresh the page automatically every 5 sec
// 	// 				Web_Client.println();
// 	// 				Web_Client.println("<!DOCTYPE HTML>");
// 	// 				Web_Client.println("<html>");
// 	// 				// output the value of each analog input pin
// 	// 				SaveCrash.print(Web_Client);
// 	// 				// for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
// 	// 				// 	int sensorReading = analogRead(analogChannel);
// 	// 				// 	Web_Client.print("analog input ");
// 	// 				// 	Web_Client.print(analogChannel);
// 	// 				// 	Web_Client.print(" is ");
// 	// 				// 	Web_Client.print(sensorReading);
// 	// 				// 	Web_Client.println("<br />");
// 	// 				// }
// 	// 				Web_Client.println("</html>");
// 	// 				break;
// 	// 			}
// 	// 			if (c == '\n') {
// 	// 				// you're starting a new line
// 	// 				currentLineIsBlank = true;
// 	// 			} 
// 	// 			else if (c != '\r') {
// 	// 				// you've gotten a character on the current line
// 	// 				currentLineIsBlank = false;
// 	// 			}
// 	// 		}
// 	// 	}
// 	// 	Log(Debug, "/Crash", "Client disconnected");
// 	// }
// 	// delay(1); // give the web browser time to receive the data
// 	// // close the connection:
// 	// Web_Client.stop();

// 	//  // read line by line what the client (web browser) is requesting
// 	// if (Web_Client)
// 	// {
// 	// 	Log(Debug, "/Crash", "Client connected");
// 	// 	while (Web_Client.connected())
// 	// 	{
// 	// 		// read line by line what the client (web browser) is requesting
// 	// 		if (Web_Client.available())
// 	// 		{
// 	// 			String line = Web_Client.readStringUntil('\r');
// 	// 			// look for the end of client's request, that is marked with an empty line
// 	// 			if (line.length() == 1 && line[0] == '\n')
// 	// 			{
// 	// 			// send response header to the web browser
// 	// 			Web_Client.print("HTTP/1.1 200 OK\r\n");
// 	// 			Web_Client.print("Content-Type: text/plain\r\n");
// 	// 			Web_Client.print("Connection: close\r\n");
// 	// 			Web_Client.print("\r\n");
// 	// 			// send crash information to the web browser
// 	// 			SaveCrash.print(Web_Client);
// 	// 			break;
// 	// 			}
// 	// 		}
// 	// 	}
// 	// 	delay(2); // give the web browser time to receive the data
// 	// 	// close the connection:
// 	// 	Web_Client.stop();
// 	// 	Log(Debug, "/Crash", "Client connected");
// 	// }

// } // Crash_Loop()



// ############################################################ Dimmer_Fade() ############################################################
void Dimmer_Fade(byte Selected_Dimmer, byte State_Procent) {

	int State_Current = Dimmer_State[Selected_Dimmer - 1];

	int State_Target = State_Procent;
	float Temp_Float = State_Target * 0.01;
	State_Target = Temp_Float * 1023;
	Dimmer_State[Selected_Dimmer - 1] = State_Target;

	unsigned long Fade_Wait_Till = millis();

	while (State_Current != State_Target) {

		while (millis() < Fade_Wait_Till) {
			delay(1);
		}

		// Last jump +
		if (State_Current < State_Target && State_Target - State_Current <= Dimmer_Fade_Jump) {
			analogWrite(Dimmer_Pins[Selected_Dimmer - 1], State_Target);
			delay(5);
			break;
		}

		// Last jump -
		else if (State_Target < State_Current && State_Current - State_Target <= Dimmer_Fade_Jump) {
			analogWrite(Dimmer_Pins[Selected_Dimmer - 1], State_Target);
			delay(5);
			break;
		}

		// +
		else if (State_Current < State_Target) {
			State_Current += Dimmer_Fade_Jump;
			analogWrite(Dimmer_Pins[Selected_Dimmer - 1], State_Current);
		}

		// -
		else {
			State_Current -= Dimmer_Fade_Jump;
			analogWrite(Dimmer_Pins[Selected_Dimmer - 1], State_Current);
		}

		Fade_Wait_Till = millis() + Dimmer_Fade_Jump_Delay;
	} // while

	Dimmer_Procent[Selected_Dimmer - 1] = State_Procent;
	
	Log(Device, MQTT_Topic[Topic_Dimmer] + "/" + String(Selected_Dimmer) + "/State", Dimmer_Procent[Selected_Dimmer - 1]);

} // Dimmer_Fade()


// ############################################################ Dimmer() ############################################################
bool Dimmer(String &Topic, String &Payload) {

	if (Dimmer_Configured == false) {
		return false;
	}

	if (Topic.indexOf(MQTT_Topic[Topic_Dimmer]) != -1) {

		Topic.replace(MQTT_Topic[Topic_Dimmer] + "/", "");

		byte Selected_Dimmer = Topic.toInt();

		// Ignore all requests thats larger then Dimmer_Max_Number_Of
		if (Selected_Dimmer >= 1 && Selected_Dimmer <= Dimmer_Max_Number_Of) {
			// State request
			if (Payload.indexOf("?") != -1) {

				int State_Current = Dimmer_State[Selected_Dimmer - 1];
				float Temp_Float = State_Current * 0.01;
				State_Current = Temp_Float * 1023;

				Log(Device, MQTT_Topic[Topic_Dimmer] + "/" + String(Selected_Dimmer) + "/State", State_Current);
			}

			else {
				int State_Target = Payload.toInt();

				// if value = current state turn off
				if (Dimmer_Procent[Selected_Dimmer - 1] == State_Target) {
					Dimmer_Fade(Selected_Dimmer, 0);
					return true;
				}

				if (Dimmer_State[Selected_Dimmer - 1] != State_Target) {
					Dimmer_Fade(Selected_Dimmer, State_Target);
					return true;
				}

			}
		}
	}
	return false;
} // Dimmer()


// ############################################################ Relay_Auto_OFF_Check() ############################################################
void Relay_Auto_OFF_Check(byte Selected_Relay) {

	if (Relay_Pin_Auto_Off_Delay[Selected_Relay - 1] != false) {
		Relay_Auto_OFF_At[Selected_Relay - 1] = millis() + Relay_Pin_Auto_Off_Delay[Selected_Relay - 1];
		Relay_Auto_OFF_Active[Selected_Relay - 1] = true;
	}
} // _Relay_Auto_OFF_Check()


// ############################################################ Relay() ############################################################
bool Relay(String &Topic, String &Payload) {

	if (Relay_Configured == false) {
		return false;
	}

	else if (Topic.indexOf(MQTT_Topic[Topic_Relay]) != -1) {

		if (Payload.length() > 1) Payload = Payload.substring(0, 1); // "Trim" length to avoid some wird error

		String Relay_String = Topic;
		Relay_String.replace(MQTT_Topic[Topic_Relay] + "/", "");

		byte Selected_Relay = Relay_String.toInt();

		// Ignore all requests thats larger then _Relay_Max_Number_Of
		if (Selected_Relay < Relay_Max_Number_Of) {
			// State request
			if (Payload == "?") {
				String State_String;
				if (digitalRead(Relay_Pins[Selected_Relay - 1]) == Relay_On_State) State_String += "1";
				else State_String += "0";
				Log(Device, MQTT_Topic[Topic_Relay] + "/" + String(Selected_Relay) + "/State", State_String);
				return true;
			}

			else if(Is_Valid_Number(Payload) == true) {
				byte State = Payload.toInt();

				if (State > 2) {
					Log(Error, "/Relay", "Relay - Invalid command entered");
					return true;
				}

				bool State_Digital = false;
				if (State == ON) State_Digital = Relay_On_State;
				else if (State == OFF) State_Digital = !Relay_On_State;
				else if (State == FLIP) State_Digital = !digitalRead(Relay_Pins[Selected_Relay - 1]);

				if (Selected_Relay <= Relay_Max_Number_Of && digitalRead(Relay_Pins[Selected_Relay - 1]) != State_Digital) {
					digitalWrite(Relay_Pins[Selected_Relay - 1], State_Digital);
					if (State_Digital == Relay_On_State) {
						Log(Device, MQTT_Topic[Topic_Relay] + "/" + String(Selected_Relay) + "/State", String(ON));
						Relay_Auto_OFF_Check(Selected_Relay);
					}
					else {
						Log(Device, MQTT_Topic[Topic_Relay] + "/" + String(Selected_Relay) + "/State", String(OFF));
					}
				}
			}
		}
	}
	return false;
} // Relay()


// ############################################################ Relay_Auto_OFF_Loop() ############################################################
void Relay_Auto_OFF_Loop() {

	for (byte i = 0; i < Relay_Max_Number_Of; i++) {

		if (Relay_Auto_OFF_Active[i] == true && Relay_Pin_Auto_Off_Delay != 0) {
			if (Relay_Auto_OFF_At[i] < millis()) {

				if (digitalRead(Relay_Pins[i]) == Relay_On_State) {
					digitalWrite(Relay_Pins[i], !Relay_On_State);
					Log(Device, MQTT_Topic[Topic_Relay] + "/" + String(i + 1) + "/State", "Relay " + String(i + 1) + " Auto OFF");
					Log(Device, MQTT_Topic[Topic_Relay] + "/" + String(i + 1) + "/State", String(OFF));
				}

				Relay_Auto_OFF_Active[i] = false;
			}
		}
	}
} // Relay_Auto_OFF()


// ############################################################ Relay_Auto_OFF() ############################################################
void Relay_Auto_OFF(byte Relay_Number) {
	if (digitalRead(Relay_Pins[Relay_Number - 1]) == Relay_On_State) {
		digitalWrite(Relay_Pins[Relay_Number - 1], !Relay_On_State);

		Log(Device, MQTT_Topic[Topic_Relay] + "/" + String(Relay_Number + 1) + "/State", String(OFF));
	}
} // _Relay_Auto_OFF()


	// ############################################################ Button_Pressed_Check() ############################################################
byte Button_Pressed_Check() {
	for (byte i = 0; i < Button_Max_Number_Of; i++) {
		if (Button_Pins[i] != 255) {
			if (Button_Ignore_Input_Untill[i] < millis()) {
				if (digitalRead(Button_Pins[i]) == LOW) {
					Log(Device, MQTT_Topic[Topic_Button] + "/" + String(i + 1), "Pressed");
					Button_Ignore_Input_Untill[i] = millis() + Button_Ignore_Input_For;
					return i;
				}
			}
		}
	}
	return 254;
} // Button_Pressed_Check()


// ############################################################ Button_Loop() ############################################################
bool Button_Loop() {

	if (Button_Configured == true || ArduinoOTA_Active == true) {

		byte Button_Pressed = Button_Pressed_Check();

		if (Button_Pressed == 254 || Button_Pressed == 255) {
			// 255 = Unconfigured Pin
			// 254 = No button pressed
			return false;
		}

		if (Button_Pressed < Button_Max_Number_Of) {

			String Topic = Button_Target[Button_Pressed].substring(0, Button_Target[Button_Pressed].indexOf("&"));
			String Payload = Button_Target[Button_Pressed].substring(Button_Target[Button_Pressed].indexOf("&") + 1, Button_Target[Button_Pressed].length());

			Log(Device, Topic, Payload);

			return true;
		}
	}

	return false;

} // Button_Loop


// ############################################################ Flow Callbacks() ############################################################
// FIX - See if there is a better way to do this then the 6 callbacks
void ICACHE_RAM_ATTR Flow_Callback_0() {
  Flow_Pulse_Counter[0] ++;
  Flow_Change[0] = true;
}
void ICACHE_RAM_ATTR Flow_Callback_1() {
  Flow_Pulse_Counter[1] ++;
  Flow_Change[1] = true;
}
void ICACHE_RAM_ATTR Flow_Callback_2() {
  Flow_Pulse_Counter[2] ++;
  Flow_Change[2] = true;
}
void ICACHE_RAM_ATTR Flow_Callback_3() {
  Flow_Pulse_Counter[3] ++;
  Flow_Change[3] = true;
}
void ICACHE_RAM_ATTR Flow_Callback_4() {
  Flow_Pulse_Counter[4] ++;
  Flow_Change[4] = true;
}
void ICACHE_RAM_ATTR Flow_Callback_5() {
  Flow_Pulse_Counter[5] ++;
  Flow_Change[5] = true;
}



// ############################################################ Flow() ############################################################
bool Flow(String &Topic, String &Payload) {

	if (Flow_Configured == false) {
		return false;
	}

	// Check topic
	else if (Topic.indexOf(MQTT_Topic[Topic_Flow]) != -1) {

		String Flow_String = Topic;
		Flow_String.replace(MQTT_Topic[Topic_Flow] + "/", "");

		byte Selected_Flow = Flow_String.toInt();

		// Ignore all requests thats larger then _Flow_Max_Number_Of
		if (Selected_Flow > Flow_Max_Number_Of) {
			return true;
		}

		if (Payload == "Reset" || Payload == "reset")
		{
			// Set flow counter to 0
			Flow_Pulse_Counter[Selected_Flow - 1] = 0;
			// Publish reset so it gets logged in the db for counter use
			Log(Device, MQTT_Topic[Topic_Flow] + "/" + String(Selected_Flow) + "/State", "Reset");
			// Log event
			Log(Debug, "/Flow/" + String(Selected_Flow), "Reset");
		}
		
	}
	
	return true;

} // Flow()



// ############################################################ Flow_Loop() ############################################################
void Flow_Loop() {

	if (Flow_Configured == false) {
		return;
	}

	for (byte i = 0; i < Flow_Max_Number_Of; i++) {
		if (Flow_Publish_At[i] < millis() && Flow_Change[i] == true) {
			
			Log(Device, MQTT_Topic[Topic_Flow] + "/" + String(i + 1) + "/State", String(Flow_Pulse_Counter[i]));
	
			Flow_Change[i] = false;
			Flow_Publish_At[i] = millis() + Flow_Publish_Delay;
		}		
	}
} // Flow_Loop()


// ############################################################ Switch() ############################################################
bool Switch(String &Topic, String &Payload) {

	if (Switch_Configured == false) {
		return false;
	}

	// Check topic
	else if (Topic.indexOf(MQTT_Topic[Topic_Switch]) != -1) {

		if (Payload.length() > 1) Payload = Payload.substring(0, 1); // "Trim" length to avoid some wird error

		String Switch_String = Topic;
		Switch_String.replace(MQTT_Topic[Topic_Switch] + "/", "");

		byte Selected_Switch = Switch_String.toInt();

		// Ignore all requests thats larger then _Switch_Max_Number_Of
		if (Selected_Switch < Switch_Max_Number_Of) {
			// State request
			if (Payload == "?") {
				String State_String;
				// 0 = ON
				if (digitalRead(Switch_Pins[Selected_Switch - 1]) == 0) State_String += "1";
				else State_String += "0";
				Log(Device, MQTT_Topic[Topic_Switch] + "/" + String(Selected_Switch) + "/State", State_String);
				return true;
			}
			// else if (Payload == "Toggle")
			// {
			// 	// Flow switch state

			// 	// Log event
			// 	Log(Device, MQTT_Topic[Topic_Switch] + "/" + String(Selected_Switch) + "/State", State_String);
			// }
			
		}
	}

	return false;
}

// ############################################################ Switch_Loop() ############################################################
bool Switch_Loop() {

	// Check if switches is configured
	if (Switch_Configured == false || ArduinoOTA_Active == true) {
		return false;
	}

	// Check if its time to refresh
	if (Switch_Ignore_Input_Untill < millis()) {

		// The Wemos seems to reset if hammered during boot so dont read for the fist 7500 sec
		if (millis() < 7500) {
			return false;
		}

		for (byte i = 0; i < Switch_Max_Number_Of; i++) {

			// If pin in use
			if (Switch_Pins[i] != 255) {
				// Check if state chenged
				bool Switch_State = digitalRead(Switch_Pins[i]);

				if (Switch_State != Switch_Last_State[i]) {

					// Set last state
					Switch_Last_State[i] = Switch_State;

					String Topic;
					String Payload;

					// Publish switch state - Flipping output to make it add up tp 1 = ON
					Log(Device, MQTT_Topic[Topic_Switch] + "/" + String(i + 1) + "/State", !Switch_State);

					// OFF
					if (Switch_State == 1) {
						Topic = Switch_Target_OFF[i].substring(0, Switch_Target_OFF[i].indexOf("&"));
						Payload = Switch_Target_OFF[i].substring(Switch_Target_OFF[i].indexOf("&") + 1, Switch_Target_OFF[i].length());
					}
					// ON
					else {
						Topic = Switch_Target_ON[i].substring(0, Switch_Target_ON[i].indexOf("&"));
						Payload = Switch_Target_ON[i].substring(Switch_Target_ON[i].indexOf("&") + 1, Switch_Target_ON[i].length());
					}
					
					// Queue target message
					Log(Device, Topic, Payload);
				}
			}
		}
		Switch_Ignore_Input_Untill = millis() + Switch_Refresh_Rate;
	}
	return true;
} // Switch_Loop()


// ############################################################ MQ_Loop() ############################################################
void MQ_Loop() {

	// Do nothing if its not configured
	if (MQ_Configured == false || ArduinoOTA_Active == true) {
		return;
	}

	// // Set currernt value
	MQ_Current_Value = analogRead(MQ_Pin_A0);
	// // Check min/max
	MQ_Value_Min = min(MQ_Current_Value, MQ_Value_Min);
	MQ_Value_Max = max(MQ_Current_Value, MQ_Value_Max);;

}	// MQ_Loop()


// ############################################################ MQ() ############################################################
bool MQ(String &Topic, String &Payload) {

	// Do nothing if its not configured
	if (MQ_Configured == false) {
		return false;
	}

	// If Values = -1 no readings resived form sensor so disabling it
	else if (MQ_Current_Value == -1) {
		// Disable sensor
		MQ_Configured = false;
		Log(Error, "/MQ", "Never got a readings from the sensor disabling it.");
		// Detatch ticket
		MQ_Ticker.detach();
		// Return
		return false;
	}

	// Check topic
	else if (Topic == MQTT_Topic[Topic_MQ]) {

		// State request
		if (Payload == "?") {
			Log(Device, MQTT_Topic[Topic_MQ] + "/State", String(MQ_Current_Value));
			return true;
		}
		// Min/Max request
		else if (Payload == "json") {

			// Create json buffer
			DynamicJsonBuffer jsonBuffer(80);
			JsonObject& root_MQ = jsonBuffer.createObject();

			// encode json string
			root_MQ.set("Current", MQ_Current_Value);
			root_MQ.set("Min", MQ_Value_Min);
			root_MQ.set("Max", MQ_Value_Max);

			// Reset values
			MQ_Value_Min = MQ_Current_Value;
			MQ_Value_Max = MQ_Current_Value;

			String MQ_String;

			root_MQ.printTo(MQ_String);

			Log(Device, MQTT_Topic[Topic_MQ] + "/json/State", MQ_String);

			return true;
		}
	}
	return false;
} // MQ


// ############################################################ DHT_Loop() ############################################################
void DHT_Loop() {

	if (DHT_Configured == false || ArduinoOTA_Active == true) {
		return;
	}

	for (byte Sensor_Number = 0; Sensor_Number < DHT_Max_Number_Of; Sensor_Number++) {
		if (DHT_Pins[Sensor_Number] != 255) {

			unsigned long DHT_Current_Read = millis();

			// Check if its ok to read from the sensor
			if (DHT_Current_Read - DHT_Last_Read >= DHT_Refresh_Rate) {

				int Error_Code = SimpleDHTErrSuccess;
				// Read sensors values
				Error_Code = dht22.read2(DHT_Pins[Sensor_Number], &DHT_Current_Value_Temperature[Sensor_Number], &DHT_Current_Value_Humidity[Sensor_Number], NULL);

				if (Error_Code != SimpleDHTErrSuccess) {
					if (DHT_Error_Counter[Sensor_Number] > DHT_Distable_At_Error_Count) {
						// Disable sensor pin
						DHT_Pins[Sensor_Number] = 255;

						bool DHT_Still_Active = false;

						// Check if any workind DHT sensors left
						for (byte ii = 0; ii < DHT_Max_Number_Of; ii++) {
							if (DHT_Pins[ii] != 255) {
								DHT_Still_Active = true;
							}
						}

						// If no sensors left disable DHT
						if (DHT_Still_Active == false) {
							DHT_Configured = false;
							Log(Error, "/DHT", "No DHT22 sensors enabled, disabling DHT");
						}
					}
					else {
						DHT_Error_Counter[Sensor_Number] = DHT_Error_Counter[Sensor_Number] + 1;
					}
				}
				// All ok
				else {
					// Reset error counter
					DHT_Error_Counter[Sensor_Number] = 0;
					// Reset default
					// If Humidity is 0 then its assumed the sensors havent read yet
					if (DHT_Min_Value_Humidity[Sensor_Number] == 0) {
						DHT_Min_Value_Temperature[Sensor_Number] = DHT_Current_Value_Temperature[Sensor_Number];
						DHT_Max_Value_Temperature[Sensor_Number] = DHT_Current_Value_Temperature[Sensor_Number];
						DHT_Min_Value_Humidity[Sensor_Number] = DHT_Current_Value_Humidity[Sensor_Number];
						DHT_Max_Value_Humidity[Sensor_Number] = DHT_Current_Value_Humidity[Sensor_Number];
					}
					// Save Min/Max
					else {
						DHT_Min_Value_Temperature[Sensor_Number] = min(DHT_Min_Value_Temperature[Sensor_Number], DHT_Current_Value_Temperature[Sensor_Number]);
						DHT_Max_Value_Temperature[Sensor_Number] = max(DHT_Max_Value_Temperature[Sensor_Number], DHT_Current_Value_Temperature[Sensor_Number]);
						DHT_Min_Value_Humidity[Sensor_Number] = min(DHT_Min_Value_Humidity[Sensor_Number], DHT_Current_Value_Humidity[Sensor_Number]);
						DHT_Max_Value_Humidity[Sensor_Number] = max(DHT_Max_Value_Humidity[Sensor_Number], DHT_Current_Value_Humidity[Sensor_Number]);
					}
				}

				// Save the last time you read the sensor
				DHT_Last_Read = millis();
			}
		}
	}

} // DHT_Loop()


// ############################################################ DHT() ############################################################
bool DHT(String &Topic, String &Payload) {

	if (DHT_Configured == false) {
		return false;
	}

	if (Topic.indexOf(MQTT_Topic[Topic_DHT]) != -1) {

		Topic.replace(MQTT_Topic[Topic_DHT] + "/", "");

		byte Selected_DHT = Topic.toInt() - 1;

		if (Topic.toInt() <= DHT_Max_Number_Of) {

			if (Payload == "?") {
				Log(Device, MQTT_Topic[Topic_DHT] + "/" + String(Selected_DHT + 1) + "/Humidity", String(DHT_Current_Value_Humidity[Selected_DHT]));
				Log(Device, MQTT_Topic[Topic_DHT] + "/" + String(Selected_DHT + 1) + "/Temperature", String(DHT_Current_Value_Temperature[Selected_DHT]));
				return true;
			}

			else if (Payload == "json") {

				// Create json buffer
				DynamicJsonBuffer jsonBuffer(80);
				JsonObject& root_DHT = jsonBuffer.createObject();

				// encode json string
				root_DHT.set("Current Humidity", DHT_Current_Value_Humidity[Selected_DHT]);
				root_DHT.set("Current Temperature", DHT_Current_Value_Temperature[Selected_DHT]);
				root_DHT.set("Min Humidity", DHT_Min_Value_Humidity[Selected_DHT]);
				root_DHT.set("Max Humidity", DHT_Max_Value_Humidity[Selected_DHT]);
				root_DHT.set("Min Temperature", DHT_Min_Value_Temperature[Selected_DHT]);
				root_DHT.set("Max Temperature", DHT_Max_Value_Temperature[Selected_DHT]);

				String DHT_String;

				// Build json
				root_DHT.printTo(DHT_String);

				Log(Device, MQTT_Topic[Topic_DHT] + "/" + String(Topic.toInt()) + "/json/State", DHT_String);

				// Reset Min/Max
				DHT_Min_Value_Temperature[Selected_DHT] = DHT_Current_Value_Temperature[Selected_DHT];
				DHT_Max_Value_Temperature[Selected_DHT] = DHT_Current_Value_Temperature[Selected_DHT];
				DHT_Min_Value_Humidity[Selected_DHT] = DHT_Current_Value_Humidity[Selected_DHT];
				DHT_Max_Value_Humidity[Selected_DHT] = DHT_Current_Value_Humidity[Selected_DHT];

				return true;
			}
		}
	}
	return false;
} // DHT()


// ############################################################ PIR() ############################################################
void PIR_Loop() {

	// The PIR needs 60 seconds to worm up
		if (PIR_Configured == false || millis() < 60000 || ArduinoOTA_Active == true) {
		return;
	}

	// Disable PIR Warmup blink
	Indicator_LED(LED_PIR, false);

	// Check each PIR
	for (byte i = 0; i < PIR_Max_Number_Of; i++) {

		if (PIR_Pins[i] != 255) {

			// If LDR is on aka its brigh or the PIR is desabled then return
			if (PIR_Disabled[i] == true || PIR_LDR_ON[i] == true) {
				if (PIR_ON_Send[i] == true) {
					Log(Device, PIR_Topic[i], PIR_OFF_Payload[i]);
					PIR_ON_Send[i] = false;
				}

				return;
			}

			bool PIR_Current_State = digitalRead(PIR_Pins[i]);

			// Current state check
			// HIGH = Motion detected
			if (PIR_Current_State == HIGH) {
				// Check if timer is running by checking if PIR_OFF_At[i] < millis()
				if (PIR_ON_Send[i] == false) {
					// Publish ON
					Log(Device, PIR_Topic[i], PIR_ON_Payload[PIR_Selected_Level[i]][i]);
					PIR_ON_Send[i] = true;
				}
				// Reset timer
				PIR_OFF_At[i] = millis() + (PIR_OFF_Delay[i] * 1000);
			}

			else {
				// Check if timer have elapsed
				if (millis() < PIR_OFF_At[i]) {
					return;
				}
				if (PIR_ON_Send[i] == true) {
					// Publish OFF
					Log(Device, PIR_Topic[i], PIR_OFF_Payload[i]);
					PIR_ON_Send[i] = false;
				}
			}
		}
	}

} // PIR_Loop()


// ############################################################ PIR() ############################################################
bool PIR(String &Topic, String &Payload) {

	if (PIR_Configured == false) {
		return false;
	}

	if (Topic.indexOf(MQTT_Topic[Topic_PIR]) != -1) {

		Topic.replace(MQTT_Topic[Topic_PIR] + "/", "");

		byte Selected_PIR = Topic.toInt() - 1;

		if (Topic.toInt() <= PIR_Max_Number_Of) {

			// Find current PIR state
			String PIR_State;

			// PIR is on if millis() < PIR_OFF_At[i] aka timer running
			// ON
			if (millis() < PIR_OFF_At[Selected_PIR]) {
				PIR_State = "1";
			}
			// OFF
			else {
				PIR_State = "0";
			}

			if (Payload == "?") {
				Log(Device, MQTT_Topic[Topic_PIR] + "/" + String(Selected_PIR + 1) + "/State", PIR_State);
				return true;
			}

			else if (Payload.indexOf("LDR ") != -1) {

				Payload.replace("LDR ", "");

				PIR_LDR_ON[Selected_PIR] = Payload.toInt();

				Log(Device, MQTT_Topic[Topic_PIR] + "/" + String(Selected_PIR + 1) + "/LDR/State", PIR_LDR_ON[Selected_PIR]);
				return true;
			}

			else if (Payload.indexOf("Level ") != -1) {
				// Get selection
				Payload.replace("Level ", "");

				PIR_Selected_Level[Selected_PIR] = Payload.toInt();

				// FIX - Add level check has to be = to max level

				Log(Device, MQTT_Topic[Topic_PIR] + "/" + String(Selected_PIR + 1) + "/Level/State", Payload);
				return true;
			}

			else if (Payload == "Enable") {
				PIR_Disabled[Selected_PIR] = false;
				Log(Device, MQTT_Topic[Topic_PIR] + "/" + String(Selected_PIR + 1) + "/Disabled/State", false);
				return true;
			}
			else if (Payload == "Disable") {
				PIR_Disabled[Selected_PIR] = true;
				Log(Device, MQTT_Topic[Topic_PIR] + "/" + String(Selected_PIR + 1) + "/Disabled/State", true);
				return true;
			}

			else if (Payload == "json") {
				// Create json buffer
				DynamicJsonBuffer jsonBuffer(190);
				JsonObject& root_PIR = jsonBuffer.createObject();

				// encode json string
				root_PIR.set("State", PIR_State);
				root_PIR.set("Level", PIR_Selected_Level[Selected_PIR]);
				root_PIR.set("Level Value", PIR_ON_Payload[PIR_Selected_Level[Selected_PIR] - 1][Selected_PIR]);
				root_PIR.set("LDR", PIR_LDR_ON[Selected_PIR]);
				root_PIR.set("Disabled", PIR_Disabled[Selected_PIR]);

				String PIR_String;

				root_PIR.printTo(PIR_String);

				Log(Device, MQTT_Topic[Topic_PIR] + "/" + String(Selected_PIR + 1) + "/json/State", PIR_String);

				return true;
			}

		}
	}
	return false;
}


// ############################################################ DC_Voltmeter() ############################################################
bool DC_Voltmeter(String &Topic, String &Payload) {

  if (Topic.indexOf(MQTT_Topic[Topic_DC_Voltmeter]) != -1) {
	if (DC_Voltmeter_Configured == false) {
		return true;
	}

    if (Payload.indexOf("?") != -1) {

      // read the value at analog input
      int value = analogRead(DC_Voltmeter_Pins);
      float vout = (value * DC_Voltmeter_Curcit_Voltage) / 1023.0;
      float vin = vout / (DC_Voltmeter_R2 / (DC_Voltmeter_R1 + DC_Voltmeter_R2));
      vin = vin + DC_Voltmeter_Offset;

      Log(Device, MQTT_Topic[Topic_DC_Voltmeter] + "/State", String(vin));
      return true;
    }
  }

  return false;
}





// ############################################################ DS18B20_Scan() ############################################################
void DS18B20_Scan() {
	// Reset as stated in maunal
	DS18B20_OneWire.reset_search();
	// Get device count
	byte Device_Count = DS18B20_Sensors.getDeviceCount();
	// Check if there is any devices on the bus
	if (Device_Count != 0)
	{
		// report parasite power requirements
		if (DS18B20_Sensors.isParasitePowerMode()) Log(Debug, "/DS18B20", "Parasite power is: ON");
		else Log(Debug, "/DS18B20", "Parasite power is: OFF");
		// Loop over avalible devices
		for (byte i = 0; i < Device_Count; i++)
		{
			// Check if we reached max supported sensors aka no more variable space
			if (i >= DS18B20_Max_Number_Of)
			{
				// Log event
				Log(Debug, "/DS18B20", "Can't handle more sensors");
				continue;
			}
			// Get the sensor address
			if (DS18B20_Sensors.getAddress(DS18B20_Address[i], i))
			{
				// Convert the address array to string
				DS18B20_Address_String[i] = Device_Address_To_String(DS18B20_Address[i]);
				// Log event
				Log(Info, "/DS18B20", "Device found on address: " + DS18B20_Address_String[i]);
				// Set Resolution
				DS18B20_Sensors.setResolution(DS18B20_Address[i], DS18B20_Resolution);
			}
		}
	}
	else
	{
		// Log event
		Log(Info, "/DS18B20", "No divices found on bus, run 'Scan' to check again. Did you remember the 4.7K pullup resistor?");
	}
} // DS18B20_Scan





// ############################################################ DS18B20() ############################################################
bool DS18B20(String &Topic, String &Payload) {

	if (ArduinoOTA_Active == true) {
		return false;
	}

	// Check for individualt sensors
	if (Topic.indexOf(MQTT_Topic[Topic_DS18B20]) != -1) {

		// Ignore messages ending in "/State" so we dont act on our own messages
		if (Topic.substring(Topic.length() - 6, Topic.length()) == "/State")
		{
			return true;
		}

		// Strip first part of topic
		Topic.replace(MQTT_Topic[Topic_DS18B20] + "/", "");

		byte Selected_DS18B20 = 254;

		// Check if Topic is in DS18B20_Address_String
		for (byte i = 0; i < DS18B20_Max_Number_Of; i++)
		{
			// Address found in DS18B20_Address_String
			if (Topic == DS18B20_Address_String[i])
			{
				// Save Selected device
				Selected_DS18B20 = i;
			}
				
		}
		// Check that we found a device
		if (Selected_DS18B20 == 254)
		{
			Log(Warning, "/DS18B20", "Unknown Sensor address: " + String(Topic));
			return true;
		}
		
		if (Selected_DS18B20 <= DS18B20_Max_Number_Of) {

			// if (Payload == "reset errors") {
			// 	// Reset error counter
			// 	DS18B20_Error_Counter[Selected_DS18B20] = 0;
			// 	// Enable sensor if its disabled
			// 	DS18B20_Configured[Selected_DS18B20] = true;
			// 	// Log event
			// 	return true;
			// }

			// // Check if sensor is configured
			// if (DS18B20_Configured[Selected_DS18B20] == false) {
			// 	return true;
			// }

			if (Payload == "?") {
				Log(Device, MQTT_Topic[Topic_DS18B20] + "/" + DS18B20_Address_String[Selected_DS18B20] + "/State", String(DS18B20_Current[Selected_DS18B20]));
				return true;
			}

			else if (Payload == "json") {
				// Create json buffer
				DynamicJsonBuffer jsonBuffer(80);
				JsonObject& root_DS18B20 = jsonBuffer.createObject();

				// encode json string
				root_DS18B20.set("Current Temperature", DS18B20_Current[Selected_DS18B20]);
				root_DS18B20.set("Min Temperature", DS18B20_Min[Selected_DS18B20]);
				root_DS18B20.set("Max Temperature", DS18B20_Max[Selected_DS18B20]);

				String DS18B20_String;

				// Build json
				root_DS18B20.printTo(DS18B20_String);

				Log(Device, MQTT_Topic[Topic_DS18B20] + "/" + DS18B20_Address_String[Selected_DS18B20] + "/json/State", DS18B20_String);

				// Reset Min/Max
				DS18B20_Min[Selected_DS18B20] = DS18B20_Current[Selected_DS18B20];
				DS18B20_Max[Selected_DS18B20] = DS18B20_Current[Selected_DS18B20];

				return true;
			}
		}
	}
	return false;
} // DS18B20()


// ############################################################ DS18B20_Loop() ############################################################
void DS18B20_Loop() {

	// Do nothing if OTA is active
	// Wait for the device to boot before start reading
	if (ArduinoOTA_Active == true || MQTT_State == MQTT_State_Init) return;
	// Check if we are done reading, if not return
	if (DS18B20_Sensors.isConversionComplete() == false)
	{
		return;
	}
	// Request values
	DS18B20_Sensors.requestTemperatures();
	// Read values
	for (byte i = 0; i < DS18B20_Max_Number_Of; i++)
	{
		// Check if sensor is disabled
		if (DS18B20_Address_String[i] == "")
		{
			continue;
		}
		// Check error counter
		if (DS18B20_Error_Counter[i] > DS18B20_Error_Max)
		{
			// Disable the sensor
			DS18B20_Address_String[i] = "";
			// Log event
			Log(Debug, "/DS18B20", "Sensor with address: " + DS18B20_Address_String[i] + " Disabled due to max error counter: " + String(DS18B20_Error_Counter[i]));
		}
		// Creat var to hold current reading
		float Current_Reading = DS18B20_Sensors.getTempCByIndex(i);
		// float Current_Reading = DS18B20_Sensors.getTempC(DS18B20_Address[i]);
		// Check if sensor was disconnected
		if (Current_Reading == DEVICE_DISCONNECTED_C)
		{
			// Log event
			Log(Debug, "/DS18B20", "Sensor with address: " + DS18B20_Address_String[i] + " Disconnected. Error count: " + String(DS18B20_Error_Counter[i]));
			// Add to error counter
			DS18B20_Error_Counter[i] = DS18B20_Error_Counter[i] + 1;
		}
		else
		{
			// Get value
			DS18B20_Current[i] = Current_Reading;
			DS18B20_Min[i] = min(DS18B20_Min[i], DS18B20_Current[i]);
			DS18B20_Max[i] = max(DS18B20_Max[i], DS18B20_Current[i]);
			// Reset error counter
			DS18B20_Error_Counter[i] = 0;
		}
	}
} // DS18B20_Loop()


// ############################################################ Echo() ############################################################
// Reads the distance
float Echo(byte Echo_Pin_Trigger, byte Echo_Pin_Echo) {

  // Clears the Echo_Pin_Trigger
  digitalWrite(Echo_Pin_Trigger, LOW);
  delayMicroseconds(2);

  // Sets the Echo_Pin_Trigger on HIGH state for 10 micro seconds
  digitalWrite(Echo_Pin_Trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(Echo_Pin_Trigger, LOW);

  // Reads the Echo_Pin_Echo, returns the sound wave travel time in microseconds
  unsigned long Duration = pulseIn(Echo_Pin_Echo, HIGH, 75000);

  // Calculating the distance
  float Distance = (Duration / 2) * 0.0343;

  return Distance;

} // Echo


// ############################################################ Distance_Loop() ############################################################
void Distance_Loop() {

  if (Distance_Configured == false || ArduinoOTA_Active == true) {
    return;
  }

	if (Distance_Sensor_Read_At < millis()) {
  	for (byte i = 0; i < Distance_Max_Number_Of; i++) {
			// Check if both pns is set
			if (Distance_Pins_Trigger[i] != 255 || Distance_Pins_Echo[i] != 255) {
				// Read distance
				Distance_Last_Reading[i] = Echo(Distance_Pins_Trigger[i], Distance_Pins_Echo[i]);
				// Set running averidge
				if (i == 0) Distance_RA_1.addValue(Distance_Last_Reading[i]);
				if (i == 1) Distance_RA_2.addValue(Distance_Last_Reading[i]);
				if (i == 2) Distance_RA_3.addValue(Distance_Last_Reading[i]);
			}
			// Check if distace sensor is configured
			if (Distance_ON_At[i] != 0) {
				// if so then check if actions is needed

				// Check if pubish delay is active, if so do nothing
				if (Distance_Sensor_Publish_At[i] > millis()) {
					return;
				}

				// If Distance_ON_Reversed is true then ON is the lowest number
				bool Distance_ON_Reversed = false;

				String Topic;
				String Payload;
				String Changed_State_To;

				float Current_Average;

				if (i == 0) Current_Average = Distance_RA_1.getAverage();
				else if (i == 1) Current_Average = Distance_RA_2.getAverage();
				else if (i == 2) Current_Average = Distance_RA_3.getAverage();

				// Deturman if the ON target is smaller or larger then the OFF target
				if (Distance_OFF_At[i] < Distance_ON_At[i]) {
					Distance_ON_Reversed = true;
				}

				// ON_At is the smalles number
				if (Distance_ON_Reversed == true) {
					// Check if reading is smaller than Distance_ON_At
					if (Distance_ON_At[i] <= Current_Average) {
						// Check if sensor is already on
						// Sensor not on
						if (Distance_Sensor_State[i] == false) {
							// Build Topic and Payload
							Topic = Distance_Target_ON[i].substring(0, Distance_Target_ON[i].indexOf("&"));
							Payload = Distance_Target_ON[i].substring(Distance_Target_ON[i].indexOf("&") + 1, Distance_Target_ON[i].length());
							Changed_State_To = "ON";
							// Change sensor state to on
							Distance_Sensor_State[i] = true;
						}
					}

					// Check if reading is larger than Distance_OFF_AT
					if (Distance_OFF_At[i] >= Current_Average) {
						// Check if sensor is already on
						// Sensor not on
						if (Distance_Sensor_State[i] == true) {
							// Build Topic and Payload
							Topic = Distance_Target_OFF[i].substring(0, Distance_Target_OFF[i].indexOf("&"));
							Payload = Distance_Target_OFF[i].substring(Distance_Target_OFF[i].indexOf("&") + 1, Distance_Target_OFF[i].length());
							Changed_State_To = "OFF";
							// Change sensor state to on
							Distance_Sensor_State[i] = false;
						}
					}
				}

				// ON_At is the largest number
				else if (Distance_ON_Reversed == false) {
					// Check if reading is larger than Distance_ON_At
					if (Distance_ON_At[i] >= Current_Average) {
						// Check if sensor is already on
						// Sensor not on
						if (Distance_Sensor_State[i] == false) {
							// Build Topic and Payload
							Topic = Distance_Target_ON[i].substring(0, Distance_Target_ON[i].indexOf("&"));
							Payload = Distance_Target_ON[i].substring(Distance_Target_ON[i].indexOf("&") + 1, Distance_Target_ON[i].length());
							Changed_State_To = "ON";
							// Change sensor state to on
							Distance_Sensor_State[i] = true;
						}
					}

					// Check if reading is larger than Distance_OFF_AT
					else if (Distance_OFF_At[i] <= Current_Average) {
						// Check if sensor is already on
						// Sensor not on
						if (Distance_Sensor_State[i] == true) {
							// Build Topic and Payload
							Topic = Distance_Target_OFF[i].substring(0, Distance_Target_OFF[i].indexOf("&"));
							Payload = Distance_Target_OFF[i].substring(Distance_Target_OFF[i].indexOf("&") + 1, Distance_Target_OFF[i].length());
							Changed_State_To = "OFF";
							// Change sensor state to on
							Distance_Sensor_State[i] = false;
						}
					}
				}

				if (Topic != "") {
					// Publish sensor state change to sensor
					Log(Device, Topic, Payload);
					// Log event
					Log(Device, MQTT_Topic[Topic_Distance] + "/Sensor/State", "'Distance Sensor' " + String(i + 1) + " Triggered " + Changed_State_To + " at: " + String(Current_Average));
					// Set publish delay
					Distance_Sensor_Publish_At[i] = millis() + Distance_Sensor_Publish_Delay;
				}
			}
		}
	}
} // Distance_Loop()

// ############################################################ Distance() ############################################################
// Handles MQTT Read request
bool Distance(String &Topic, String &Payload) {

  if (Topic.indexOf(MQTT_Topic[Topic_Distance]) != -1) {
    // Ignore state publish from localhost
    if (Topic.indexOf("/State") == -1)  {

      Topic.replace(MQTT_Topic[Topic_Distance] + "/", "");

      if (Topic.toInt() <= 0 || Topic.toInt() > Distance_Max_Number_Of) {
        Log(Error, "/Distance", "Distance Sensor " + Topic + " is not a valid distance sensor");
        return true;
      }

      if (Distance_Pins_Trigger[Topic.toInt() - 1] == 255 || Distance_Pins_Echo[Topic.toInt() - 1] == 255) {
        Log(Error, "/Distance", "Distance Sensor " + Topic + " is not configured");
        return true;
      }

      // Returns RunningAveridge
      if (Payload.indexOf("?") != -1) {
        float Distance = 0;

        if (Topic.toInt() == 1) Distance = Distance_RA_1.getAverage();
        else if (Topic.toInt() == 2) Distance = Distance_RA_2.getAverage();
        else if (Topic.toInt() == 3) Distance = Distance_RA_3.getAverage();

        Log(Device, MQTT_Topic[Topic_Distance] + "/" + Topic + "/State", String(Distance));
        return true;
      }
      // Returns only last reading
      else if (Payload.indexOf("Raw") != -1) {
        Log(Device, MQTT_Topic[Topic_Distance] + "/" + Topic + "/Raw/State", String(Distance_Last_Reading[Topic.toInt() - 1]));
        return true;
      }
      // Returns sensor state
      else if (Payload.indexOf("Sensor") != -1) {
        Log(Device, MQTT_Topic[Topic_Distance] + "/" + Topic + "/Sensor/State", String(Distance_Sensor_State[Topic.toInt() - 1]));
        return true;
      }
    }
  }

  return false;
} // Distance()


// ############################################################ Publish_Uptime_Loop() ############################################################
void Publish_Uptime_Loop() {

	if (Publish_Uptime == false || ArduinoOTA_Active == true) {
		return;
	}
	// Check if its time to publish
	if (Publish_Uptime_At < millis()) {
		// Log event
		Log(Message, Topic_Uptime, millis() - Publish_Uptime_Offset);
		// Reset delay
		Publish_Uptime_At = millis() + Publish_Uptime_Interval;
		// Log event
		Log(Debug, "/PublishUptime", "Publish: " + String(millis() - Publish_Uptime_Offset));
	}

} // Publish_Uptime_Loop()


// ############################################################ setup() ############################################################
void setup() {
	
	// ------------------------------ Serial ------------------------------
	Serial.setTimeout(100);
	Serial.begin(115200);
	Serial.println();

	if (String(ARDUINO_BOARD) != "PLATFORMIO_D1_MINI") {
		Serial.println("Incompatible board detected. Firmware only compatible with Wemos D1 Mini");
		while(true == true){
			delay(1337);
		}
	}
	
	// ------------------------------ FS Config ------------------------------
	SPIFFS.begin();

	Serial_CLI_Boot_Message(Serial_CLI_Boot_Message_Timeout);

	FS_Config_Load();

	Base_Config_Check();


	// ------------------------------ Indicator_LED ------------------------------
	if (Pin_Monitor(Check_State, D4) == Pin_Free) {		
		Indicator_LED_Configured = true;
		pinMode(D4, OUTPUT);
		Indicator_LED(LED_Config, true);
	}

	
	// ------------------------------ MQTT ------------------------------
	MQTT_Setup();


	// ------------------------------ WiFi ------------------------------
	WiFi_Setup();


	// ------------------------------ ArduinoOTA ------------------------------
	ArduinoOTA_Setup();

	// Disable indicator led
	Indicator_LED(LED_Config, false);

	// Log event
	Log(Info, "/System", "Boot compleate");

} // setup()


// ############################################################ loop() ############################################################
void loop() {

	// MQTT
	// Check if there is incomming messages in the queue
	MQTT_Queue_Check();

	// OTA
	ArduinoOTA.handle();

	// Devices
	Relay_Auto_OFF_Loop();
	Button_Loop();
	Switch_Loop(); 
	DHT_Loop();
	PIR_Loop();
	Flow_Loop();
	Distance_Loop();

	Log_Loop();

	Publish_Uptime_Loop();

	// Needs to be there to prevent watchdog reset
	delay(0);

} // loop()
