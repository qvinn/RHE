#include <wiringPi.h>
#include <stdio.h>

#include <vector>
#include <mutex>          // std::mutex
#include <thread>         // std::thread
#include "debug_class.h"

// gpio write 0 0 // первый 0 - это номер Пина(WiringPI), второй - это уровень(HIGH|LOW)
// gpio read 0 - Прочитать значение с Пина 0(WiringPI)

// Базовый пример

/* int main (void) {
	wiringPiSetup();
	pinMode (0, OUTPUT);	// 11
	pinMode (2, INPUT);		// 13
	int state;
	for (;;) {
	digitalWrite(0, HIGH);
	state = digitalRead(2);
	printf("State: %i\n",state);
	delay (500) ;
	digitalWrite(0, LOW);
	state = digitalRead(2);
	printf("State: %i\n",state);
	delay(500);
	}
	return 0;
} */

void blink();

int main (void) {
	Debug *gdb = new Debug();
	std::vector<int> pins_numbers{2};
	gdb->setup_pins(pins_numbers,2000,100);
	
	pinMode (0, OUTPUT);
	std::thread blink_thread(&blink);
	blink_thread.detach();
	
	gdb->start_debug();
	gdb->show_LOG();
	
	delete gdb;
	return 0;
}

void blink()
{
	for (;;) {
		digitalWrite(0, HIGH);
		delay (500) ;
		digitalWrite(0, LOW);
		delay(500);
	}
}