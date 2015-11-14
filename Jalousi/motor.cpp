//#ifndef motor_cpp
//#define motor_cpp

#include "motor.h"

void Motor::setup(int in1, int in2, int top, int button, uint8_t adress) //:
//m_in1(in1), m_in2(in2), m_top(top), m_button(button)
{
	m_in1 = in1;
	m_in2 = in2;
	m_top = top;
	m_button = button;

	Serial.begin(9600);
	Serial.println("Init");

	if (adress != 0) {
		m_pcf = new PCF8574;
		m_pcf->begin(adress);
	} else {
		m_pcf = NULL;
	}
	m_pinMode(in1, OUTPUT);
	m_pinMode(in2, OUTPUT);
	m_pinMode(top, INPUT);
	m_pinMode(button, INPUT);

	m_stop();
}

Motor::~Motor()
{
	if (m_pcf) delete m_pcf;
}

//
// вращаем вперёд
//
void Motor::motor_up()
{
	m_stop();
	if (m_pinRead(m_top) != HIGH) {
		m_up();
	} else {
		Serial.println("[i] m_top = HIGH");
	}
}

//
// вращаем назад
//
void Motor::motor_down()
{
	m_stop();
	if (m_pinRead(m_button) != HIGH) {
		m_down();
	} else {
		Serial.println("[i] m_button = HIGH");
	}
}

void Motor::motor_stop(bool stop)
{
	if (m_isUp) {
		if (m_pinRead(m_top) == HIGH) m_stop();
	} else {
		if (m_pinRead(m_button) == HIGH) {
			m_stop();
		} else {
			if (m_pinRead(m_top) == HIGH) {
				Serial.println("[!] ERROR! Ne srabotal m_botton");
				m_up();
			}
		}
	}
	if (stop) m_stop();
}

//
// крутим моторчик вперёд
//
void Motor::m_up()
{
	Serial.println("[i] up");
	m_pinWrite(m_in1, HIGH);
	m_pinWrite(m_in2, LOW);
	m_isUp = true;
}

//
// крутим моторчик назад
//
void Motor::m_down()
{
	Serial.println("[i] down");
	m_pinWrite(m_in1, LOW);
	m_pinWrite(m_in2, HIGH);
	m_isUp = false;
}

void Motor::m_stop()
{
	Serial.println("[i] stop");
	m_pinWrite(m_in1, LOW);
	m_pinWrite(m_in2, LOW);
}

void Motor::m_pinMode(uint8_t pin, uint8_t mode)
{
	if (m_pcf) m_pcf->pinMode(pin, mode);
	else pinMode(pin, mode);
}

void Motor::m_pinWrite(uint8_t pin, uint8_t val)
{
	if (m_pcf) m_pcf->digitalWrite(pin, val);
	else digitalWrite(pin, val);
}

uint8_t Motor::m_pinRead(uint8_t pin)
{
	if (m_pcf) return m_pcf->digitalRead(pin);
	else return digitalRead(pin);
}

//#endif // #ifndef motor_cpp