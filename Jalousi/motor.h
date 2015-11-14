/*
* robocraft.h
*
* RoboCraft - library for RoboCraft.ru project
* RoboCraft - библиотека для проекта RoboCraft.ru
*
*
* Written by noonv, August 2009.
*/

#ifndef motor_h
#define motor_h

//#include "Arduino.h"
#include <Wire\Wire.h>
#include <PCF8574\PCF8574.h>

class Motor
{
	int m_in1;			// INPUT1
	int m_in2;			// INPUT2
	int m_top;
	int m_button;
	PCF8574 *m_pcf;

	bool m_isUp;	

public:
	void setup(int in1, int in2, int top, int button, uint8_t adress = 0);
	~Motor();
	void motor_up();
	void motor_down();
	void motor_stop(bool stop = false);

private:
	void m_up();
	void m_down();
	void m_stop();
	void m_pinMode(uint8_t pin, uint8_t mode);
	void m_pinWrite(uint8_t pin, uint8_t val);
	uint8_t m_pinRead(uint8_t pin);
};

#endif // #ifndef robocraft_h