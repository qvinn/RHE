#include <wiringPi.h>
#include <vector>
#include <stdio.h>

// "debug_class.h"
class Debug {

	typedef struct pinState{
		int time;
		int state;
	} pinState;
	
	
	public:
		Debug();
		void setup_pins(std::vector<int> par_number_of_pins, int par_duration_ms, int par_discrete_delay);
		void start_debug();
		void show_LOG();
	
	private:
		void clear_pin_log();
	
	int discrete_delay = 1;	// ms
	int duration_ms = 500;		// ms
	std::vector<int> number_of_pins;
	
/* 	std::vector<std::vector<pinState>> pins = {
		std::vector<pinState> pinState_pin_0,
		std::vector<pinState> pinState_pin_1,
		std::vector<pinState> pinState_pin_2,
		std::vector<pinState> pinState_pin_3,
		std::vector<pinState> pinState_pin_4,
		std::vector<pinState> pinState_pin_5,
		std::vector<pinState> pinState_pin_6,
		std::vector<pinState> pinState_pin_7,
		std::vector<pinState> pinState_pin_8,
		std::vector<pinState> pinState_pin_9,
		std::vector<pinState> pinState_pin_10,
		std::vector<pinState> pinState_pin_11,
		std::vector<pinState> pinState_pin_12,
		std::vector<pinState> pinState_pin_13,
		std::vector<pinState> pinState_pin_14
	}; */
	
/* 	std::vector<std::vector<pinState>> pins = {
		std::vector<pinState>,
		std::vector<pinState>,
		std::vector<pinState>,
		std::vector<pinState>,
		std::vector<pinState>,
		std::vector<pinState>,
		std::vector<pinState>,
		std::vector<pinState>,
		std::vector<pinState>,
		std::vector<pinState>,
		std::vector<pinState>,
		std::vector<pinState>,
		std::vector<pinState>,
		std::vector<pinState>,
		std::vector<pinState>,
		std::vector<pinState>
	}; */
	
	std::vector<std::vector<pinState>> pins;


};