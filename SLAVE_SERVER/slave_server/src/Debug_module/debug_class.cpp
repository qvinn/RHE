#include "debug_class.h"

#define PIN_0 0
#define PIN_1 1
#define PIN_2 2
#define PIN_3 3
#define PIN_4 4
#define PIN_5 5
#define PIN_6 6
#define PIN_7 7
#define PIN_8 8
#define PIN_9 9
#define PIN_10 10
#define PIN_11 11
#define PIN_12 12
#define PIN_13 13
#define PIN_14 14
#define PIN_15 15

Debug::Debug()
{
	wiringPiSetup();
}

void Debug::setup_pins(std::vector<int> par_number_of_pins, int par_duration_ms, int par_discrete_delay)
{
	number_of_pins = par_number_of_pins;
	duration_ms = par_duration_ms;
	discrete_delay = par_discrete_delay;
	
	for(int i=0; i < 16; i++)
	{
		pins.push_back(std::vector<pinState>());
	}
	
	// Сконфигурируем пины
	for(int i=0; i < par_number_of_pins.size(); i++)
	{
		pinMode(par_number_of_pins.at(i), INPUT);
	}
}

void Debug::clear_pin_log()
{
	for(int i=0; i < pins.size(); i++)
	{
		pins[i] = {};
	}
}

void Debug::start_debug()
{
	// Очистим записи предыдущей отладки
	clear_pin_log();
	// Установим начальное "текущее" время для первого прохода 1ms
	int curr_time = 1;
	// Выполним заданное количество итераций
	while(curr_time < duration_ms)
	{
		// Проанализируем состояние нужных пинов в текущей итерации
		for(int i=0; i < number_of_pins.size(); i++)
		{
			// Прочитать состояние пина
			int state = digitalRead(number_of_pins.at(i));
			// Сохранить состояние пина
			pins[i].push_back(pinState{curr_time,state});
		}
	delay(discrete_delay);
	curr_time = curr_time+discrete_delay;
	}
}

void Debug::show_LOG()
{
	for(int i=0; i < pins.size(); i++)
	{
		for(int k=0; k < pins.at(i).size(); k++)
		{
			printf("Pin width num: %i, at %i ms have state: %i\n",i, pins.at(i).at(k).time,pins.at(i).at(k).state);
		}
	}
	
}


