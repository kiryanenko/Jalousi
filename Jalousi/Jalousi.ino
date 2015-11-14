#include <Wire\Wire.h>
#include <PCF8574\PCF8574.h>
#include <EEPROMEx\EEPROMex.h>
#include <DS3232RTC-master\DS3232RTC.h>
#include <Time.h>
#include <math.h>
#include "motor.h"

#define N_MOTORS 3													// количество жалюзей
#define PIN_INT 0													// пин прерывания	0 - 2, 1 - 3
#define ALARM_INT 1													// пин прерывания от будильника
#define SIZE_ZAP (sizeof(long) + sizeof(int) + 2 * sizeof(bool))	// размер 1 записи
#define ERROR_L 13													// пин диода, сообщающий о наличии ошибки

Motor jalousi[N_MOTORS]; 

/////////////////////////////////////////////////////// ПРЕРЫВАНИЯ /////////////////////////////////////////////////////////

volatile  bool isIntOnPin = false;
volatile  bool isIntOnAlarm = false;

char bluetoothAct = '\n';
char bluetoothFile = 0;
char bluetoothTime = 0;

void int_pin() { isIntOnPin = true; }
void int_alarm() { Serial.println("int alarm"); isIntOnAlarm = true; }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

byte getDayOfWeek() {									// Функция возвращает день недели
	byte DayOfWeek = weekday() - 1;
	return DayOfWeek += DayOfWeek == 0 ? 7 : 0;			// т.к. в weekday() неделя начинается с воскресенья
}

byte takeRazryad(long num, byte i) {					// Функция отделяющая разрад от числа
	return floor((num % (long)pow(10, i)) / pow(10, i - 1));
}

void alarm()		// функция обработки прерывания на будильнике
{
	//detachInterrupt(ALARM_INT);	
	if ((bool)EEPROM.read(SIZE_ZAP - sizeof(bool) + 1)) {		// open?
		for (int i = 0; i < N_MOTORS; i++) jalousi[i].motor_up();
	} else {
		for (int i = 0; i < N_MOTORS; i++) jalousi[i].motor_down();
	}
	if (!(bool)EEPROM.read(SIZE_ZAP - 2 * sizeof(bool) + 1)) {				// !repeat?
		long week = EEPROM.readLong(sizeof(int) + 1);
		week -= 3 * pow(10, 7 - getDayOfWeek());
		if (week == 0) {
			// удаление записи
			byte n = EEPROM.read(0) - 1;
			EEPROM.write(0, n);
			for (int i = 0; i < SIZE_ZAP; i++) {							// переносим последний в начало			
				EEPROM.write(1 + i, EEPROM.read(1 + n * SIZE_ZAP + i));				
			}
		}
	} 
	setAlarm();
	//attachInterrupt(ALARM_INT, int_alarm, RISING);
}

int numWeek()							// функция подсчета номера недели
{
	int numday = 0;
	switch (month()) {
	case 12: numday += 30;
	case 11: numday += 31;
	case 10: numday += 30;	
	case 9:  numday += 31;	
	case 8:  numday += 31;	
	case 7:  numday += 30;	
	case 6:  numday += 31;	
	case 5:  numday += 30;	
	case 4:  numday += 31;	
	case 3:  numday += (year() % 4) == 0 ? 29 : 28;
	case 2:  numday += 31;	
	case 1:  numday += 0;	
		break;
	}
	numday += day() + 1 - getDayOfWeek();
	return ceil((numday + 0.0) / 7);							// округление до ближайшего большего числа
}

byte corectDay(byte day) {								// Функция корректирующая дату
	switch (month(now()))
	{
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		if (day > 31) day -= 31;
		break;
	case 4:
	case 6:
	case 9:
	case 11:
		if (day > 30) day -= 30;
		break;
	case 2:
		if (year(now()) % 4 == 0) {
			if (day > 29) day -= 29;
		} else {
			if (day > 28) day -= 28;
		}
		break;
	}
	return day;
}

void printDateTime(time_t t)							// функция печатающая время
{
	Serial.print(hour(t));		Serial.print(" : ");
	Serial.print(minute(t));	Serial.print(" : ");
	Serial.print(second(t));	Serial.print(" | ");
	Serial.print(day(t));		Serial.print(" / ");
	Serial.print(month(t));		Serial.print(" / ");
	Serial.print(year(t));		Serial.print(" | ");
	Serial.println(getDayOfWeek(), 10);
}

void setAlarm()			// функция установки будильника
{
	//RTC.alarmInterrupt(ALARM_2, false);
	byte n = EEPROM.readByte(0);
	if (n > 0) {
		setSyncProvider(RTC.get);
		bool isChetn = (numWeek() % 2) == 0;
		byte dayW = getDayOfWeek();
		int timeNow = 100 * hour() + minute();
		long minTime = -1;
		int minI;
		for (int i = 1; i <= n * SIZE_ZAP; i += SIZE_ZAP) {		// ищем ближайший будильник
			long time1 = EEPROM.readInt(i);
			long week = EEPROM.readLong(i + sizeof(int));
			bool flag = false;
			if (week != 0) {				
				for (int j = 0; (j <= 14) && !flag; j++) {		// j это через сколько дней сработает будильник
					int currentDayW = j + dayW;					// рассматриваемый день недели
					if (j < 7) {								// ищем в первые 7 дней	
						if (currentDayW <= 7) {					// ищем на этой неделе							
							int day2 = takeRazryad(week, 8 - currentDayW);		// 8 = 7 + 1
							if ((day2 == (isChetn ? 2 : 1)) || (day2 == 3)) {
								if (j != 0 || timeNow - time1 < -1) {	// будильник сработает чуть позже чем сейчас
									time1 = 10000 * j + time1;
									flag = true;
								}
							}
						} else {								// ищем на следующей неделе
							currentDayW -= 7;
							int day2 = takeRazryad(week, 8 - currentDayW);
							if ((day2 == (!isChetn ? 2 : 1)) || (day2 == 3)) {
								time1 = 10000 * j + time1;
								flag = true;
							}
						}
					} else {									// ищем через 7 дней
						currentDayW -= 7;						// т.к. j > 7
						if (currentDayW <= 7) {					// ищем на следующей неделе							
							int day2 = takeRazryad(week, 8 - currentDayW);
							if (day2 == (!isChetn ? 2 : 1) || (day2 == 3)) {
								time1 = 10000 * j + time1;
								flag = true;
							}
						} else {								// ищем через следующую неделю
							currentDayW -= 7;
							int day2 = takeRazryad(week, 8 - currentDayW);
							if (day2 == (isChetn ? 2 : 1)) {
								time1 = 10000 * j + time1;
								flag = true;
							}
						}
					}
				}
			}
			if (flag) {
				if ((time1 < minTime) || (minTime == -1)) {
					minTime = time1;
					minI = i;
				}
			} else {
				Serial.println("ERROR! Error when read zap in EEPROM");
				digitalWrite(ERROR_L, HIGH);
			}
		}
		if (minTime != -1) {
			for (int i = 0; i < SIZE_ZAP; i++) {			// меняем местами первый и найденный
				byte b = EEPROM.read(i + minI);
				EEPROM.write(i + minI, EEPROM.read(i + 1));
				EEPROM.write(i + 1, b);
			}
			// установка будильника
			byte alarmDay = floor((minTime + 0.0) / 10000) + day();
			alarmDay = corectDay(alarmDay);
			byte alarmHour = floor((minTime % 10000) / 100);
			byte alarmMinute = minTime % 100;
			RTC.setAlarm(ALM2_MATCH_DATE, alarmMinute, alarmHour, alarmDay);
			RTC.alarm(ALARM_2);
			Serial.print("Alarm: "); Serial.print(alarmDay); Serial.print(" | "); Serial.print(alarmHour); Serial.print(" : "); Serial.println(alarmMinute);
			//RTC.alarmInterrupt(ALARM_2, true);
		} else {
			Serial.println("ERROR! numbers alarms > 0");
			digitalWrite(ERROR_L, HIGH);
		}
	} else {
		Serial.println("INFO is not alarms");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void bluetooth()			// функция обработки прерывания на bluetooth
{
	char inChar = (char)Serial.read();
	Serial.print(inChar); Serial.println(inChar, 10);
	if (bluetoothAct == '\n') {
		switch (inChar)	{
		case 'u':
		case 'd':
		case 's':
			bluetoothAct = inChar;
			break;
		case 'f':
			bluetoothAct = inChar;
			bluetoothFile = 0;
			break;
		case 't':
			bluetoothAct = inChar;
			bluetoothTime = 0;
			break;
		}
	} else {
		if ((bluetoothAct == 'u') || (bluetoothAct == 'd') || (bluetoothAct == 's')) {			// Действия для моторов?
			if ((inChar < N_MOTORS) && (inChar >= 0)) {
				switch (bluetoothAct) {
				case 'u': jalousi[inChar].motor_up();		break;
				case 'd': jalousi[inChar].motor_down();		break;
				case 's': jalousi[inChar].motor_stop(true);	break;
				}
				bluetoothAct = '\n';
			} else {
				Serial.println("Error! bluetooth error read for motors");
				digitalWrite(ERROR_L, HIGH);
			}
		} else {
			switch (bluetoothAct) {
			case 'f':
				if (bluetoothFile == 0) {
					EEPROM.write(0, inChar);
				} else {
					if (bluetoothFile <= SIZE_ZAP * EEPROM.read(0)) {
						EEPROM.write(bluetoothFile, (uint8_t)inChar);
					}
				}
				if (bluetoothFile == SIZE_ZAP * EEPROM.read(0)) {
					bluetoothAct = '\n';
					Serial.println("File downloaded");
					setAlarm();
				}
				bluetoothFile++;
				break;
			case 't':
				time_t t = now();
				switch (bluetoothTime) {
				case 0:	setTime(inChar, minute(t),  0, day(t), month(t), year(t));	break;
				case 1:	setTime(hour(t), inChar,    0, day(t), month(t), year(t));	break;
				case 2:	setTime(hour(t), minute(t), 0, inChar, month(t), year(t));	break;
				case 3: setTime(hour(t), minute(t), 0, day(t), inChar,   year(t));	break;
				case 4: setTime(hour(t), minute(t), 0, day(t), month(t), inChar + 2000);	
					RTC.set(now());
					Serial.print("Set time: "); printDateTime(now());
					bluetoothAct = '\n';
					setAlarm();
					break;
				}
				bluetoothTime++;
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
	Serial.begin(9600);
	Serial.println("Hello world!");	

	EEPROM.setMaxAllowedWrites(1000);

	setSyncProvider(RTC.get);	
	RTC.squareWave(SQWAVE_NONE);

	pinMode(ERROR_L, OUTPUT);
	digitalWrite(ERROR_L, LOW);

	delay(100);
	Serial.print("Time: "); printDateTime(now());
	
	///////// Установка пинов ////////
	jalousi[1].setup(7, 5, 1, 0, 0x20); delay(100);
	jalousi[2].setup(4, 6, 2, 3, 0x20); delay(100);
	jalousi[0].setup(3, 0, 4, 6, 0x21); delay(100);
	//////////////////////////////////
	
	pinMode(2, INPUT_PULLUP);
	pinMode(3, INPUT_PULLUP);
	RTC.alarmInterrupt(ALARM_2, true);
	attachInterrupt(PIN_INT, int_pin, RISING);
	attachInterrupt(ALARM_INT, int_alarm, FALLING);

	setAlarm();
}

void loop()
{
	if (isIntOnPin) {				// обработка прерывания на пине
		isIntOnPin = false;
		for (int i = 0; i < N_MOTORS; i++) jalousi[i].motor_stop();
	}
	if (isIntOnAlarm) {				// обработка прерывания будильника
		isIntOnAlarm = false;
		alarm();
	}
	if (Serial.available()) {
		bluetooth();
	}
}