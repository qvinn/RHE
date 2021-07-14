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

int blink_stop = 0;
std::mutex blink_stop_mutex;

int main (void) {
/* 	Debug *gdb = new Debug();
	std::vector<int> pins_numbers{2};
	gdb->setup_all(pins_numbers,2000,100);
	
	pinMode (0, OUTPUT);
	std::thread blink_thread(&blink);
	blink_thread.detach();
	
	gdb->start_debug_mode_1();
	gdb->show_LOG();
	
	delete gdb; */
	
	Debug *gdb = new Debug();
	std::vector<int> pins_numbers{8,9,7}; //2
	gdb->setup_all(pins_numbers,2000,100);
	
	//pinMode (0, OUTPUT);
	
	
	
	std::string cmd;
	while(1)
    {
        std::cin >> cmd;

        if(cmd == "start_debug")
        {
			std::thread gdb_thread(&Debug::start_debug_mode_2,gdb);
			gdb_thread.detach();
        } else if(cmd == "stop_debug")
        {
			gdb->stop_debug();
        } else if(cmd == "start_blink")
        {
			/* blink_stop_mutex.lock();
			blink_stop = 0;
			blink_stop_mutex.unlock();
			
			std::thread blink_thread(&blink);
			blink_thread.detach(); */
        }
		else if(cmd == "stop_blink")
        {
			/* blink_stop_mutex.lock();
			blink_stop = 1;
			blink_stop_mutex.unlock(); */
        }
		else if(cmd == "led_on")
        {
			//system("gpio write 0 1");
        }
		else if(cmd == "led_off")
        {
			//system("gpio write 0 0");
        }
    }
	
	return 0;
}

void blink()
{
/* 	for (;;) {
		blink_stop_mutex.lock();
		if(blink_stop == 1)
		{
			blink_stop_mutex.unlock();
			return;
		}
		blink_stop_mutex.unlock();
		
		digitalWrite(0, HIGH);
		delay (500);
		digitalWrite(0, LOW);
		delay(500);
	} */
}