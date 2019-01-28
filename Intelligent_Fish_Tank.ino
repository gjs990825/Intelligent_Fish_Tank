#include <DallasTemperature.h> //DS18B20 library
#include <hcsr04.h>			   //HC SR04 library
#include <Servo.h>			   //Servo library

#define DISCONNECT HIGH //Relays level define
#define CONNECT LOW

//const value
const uint16_t TANK_HIGHT = 280; //Fish tank hight = 280mm

//Trigger value define
#define TRIG_BRIGHTNESS 100 //Brightness
#define TRIG_TEMPERATURE 24 //Temperature
#define TRIG_DISTANCE 170   //Distance

//Command key word definition
#define HEATER_KWD '3'
#define W_IN_KWD '0'
#define W_OUT_KWD '1'
#define LIGHT_KWD '5'
#define AIR_KWD '4'
#define FEDDER_KWD '2'

//Pin definition
#define SERVO_IO 9	 //Steering gear io pin
#define TRIG_PIN 11	//SR04 TRIG_PIN
#define ECHO_PIN 12	//SR04 ECHO_PIN
#define PHOTO_RES 0	//photoresistor
#define ONE_WIRE_BUS 7 //DS18B20 Data pin
#define HEATER 2	   //Temprature Control
#define WATER_IN 3	 //Water in
#define WATER_OUT 4	//Water out
#define CHANGE_AIR 5   //Air change
#define LIGHT 6		   //Night mode light

//Private variable
uint8_t inChar;				  //a Char to hold incoming command
boolean charComplete = false; // whether the command is receved
char Cmd_Buf[30];			  //Command buffer
int pos = 0;				  //position

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
HCSR04 hcsr04(TRIG_PIN, ECHO_PIN, 20, 4000);
Servo myservo;

//Private function declaration
void Relay_Init(void);
void Relay_Test(void);
void Relay_Control(uint8_t pin, bool sta);
void Relay_Flip(uint8_t pin);
uint8_t Get_Temperature(void);
uint16_t Get_Brightness(void);
uint16_t Get_Distance(void);
void Command_Excute(void);
void Feed_Fish(void);
void Send_Cmd(uint8_t *cmd);
void Refesh_Temperature();
void Refesh_WaterLevel(void);
void Refesh_Brightness(void);
void Refesh_ScreenInfo(void);
void Auto_Control(void);

void setup()
{
	Serial.begin(9600); //Initialize serial
	while (!Serial)
		;
	Serial.println("Serial OK!\r\n");

	Relay_Init(); //Initialize all relays

	myservo.attach(SERVO_IO);
	myservo.write(40);

	sensors.begin(); //Initialize DS18B20
	delay(200);
}

void loop()
{
	Refesh_ScreenInfo();
	Command_Excute();
	Auto_Control();
}

//Private function definition
void Auto_Control(void)
{
	uint16_t tmpdata;
	//Light control
	tmpdata = Get_Brightness();
	if (tmpdata < TRIG_BRIGHTNESS)
	{
		Relay_Control(LIGHT, CONNECT);
	}
	else if (tmpdata > (TRIG_BRIGHTNESS + 50))
	{
		Relay_Control(LIGHT, DISCONNECT);
	}

	//Temperature control
	tmpdata = Get_Temperature();
	if (tmpdata < TRIG_TEMPERATURE)
	{
		Relay_Control(HEATER, CONNECT);
	}
	else if (tmpdata > (TRIG_TEMPERATURE + 2))
	{
		Relay_Control(HEATER, DISCONNECT);
	}

	//Water level control
	tmpdata = Get_Distance();
	if (tmpdata < TRIG_DISTANCE)
	{
		Relay_Control(WATER_IN, CONNECT);
	}
	else if (tmpdata > (TRIG_DISTANCE + 20))
	{
		Relay_Control(WATER_IN, DISCONNECT);
	}
}

void Refesh_ScreenInfo(void)
{
	Refesh_Brightness();
	Refesh_WaterLevel();
	Refesh_Temperature();
}

void Refesh_Brightness(void)
{
	sprintf(Cmd_Buf, "brightness.txt=\"%d\"", Get_Brightness());
	Send_Cmd(Cmd_Buf);
}

void Refesh_WaterLevel(void)
{
	sprintf(Cmd_Buf, "water_level.val=%d", map(Get_Distance(), 0, 280, 0, 100));
	Send_Cmd(Cmd_Buf);
}

void Refesh_Temperature(void)
{
	sprintf(Cmd_Buf, "temperature.txt=\"%d\"", Get_Temperature());
	Send_Cmd(Cmd_Buf);
}

void Send_Cmd(uint8_t *cmd)
{
	Serial.write((const char *)cmd);
	Serial.write(0xff);
	Serial.write(0xff);
	Serial.write(0xff);
	memset(cmd, 0, sizeof(cmd));
}

void Feed_Fish(void)
{
	myservo.write(140);
	delay(400);
	myservo.write(100);
	delay(200);
	myservo.write(140);
	delay(200);
	myservo.write(40);
	delay(400);
}

void Command_Excute(void)
{
	//Command receved?
	if (charComplete == true)
	{
		switch (inChar)
		{
		case HEATER_KWD:
			Relay_Flip(HEATER);
			break;
		case W_IN_KWD:
			Relay_Flip(WATER_IN);
			break;
		case W_OUT_KWD:
			Relay_Flip(WATER_OUT);
			break;
		case LIGHT_KWD:
			Relay_Flip(LIGHT);
			break;
		case FEDDER_KWD:
			Feed_Fish();
			break;
		case AIR_KWD:
			Relay_Flip(CHANGE_AIR);
			break;
		default:
			break;
		}
		//Clear flag and data
		inChar = 0xff;
		charComplete = false;
	}
}

void serialEvent()
{
	while (Serial.available())
	{
		inChar = (uint8_t)Serial.read();
		//Serial.println(inChar, HEX);
		charComplete = true;
	}
}

uint16_t Get_Distance(void)
{
	int16_t dis;
	dis = hcsr04.distanceInMillimeters();
	if (dis > 0)
	{
		if ((TANK_HIGHT - (uint16_t)dis) >= 0)
		{
			return (TANK_HIGHT - (uint16_t)dis);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

uint16_t Get_Brightness(void)
{
	return (analogRead(PHOTO_RES));
}

uint8_t Get_Temperature(void)
{
	sensors.requestTemperatures();
	return (sensors.getTempCByIndex(0));
}

void Relay_Flip(uint8_t pin)
{
	digitalWrite(pin, !(digitalRead(pin)));
}

void Relay_Control(uint8_t pin, bool sta)
{
	digitalWrite(pin, sta);
}

void Relay_Init(void)
{
	uint8_t i;
	//Initialize all relay contral port
	i = 2;
	do
	{
		pinMode(i++, OUTPUT);
	} while (i <= 6);

	//Reset all relay
	i = 2;
	do
	{
		digitalWrite(i++, HIGH);
	} while (i <= 6);
}

void Relay_Test(void)
{
	uint8_t i;
	i = 2;
	do
	{
		digitalWrite(i++, LOW);
		delay(50);
	} while (i <= 6);

	i = 2;
	do
	{
		digitalWrite(i++, HIGH);
		delay(50);
	} while (i <= 6);
}
