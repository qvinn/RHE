#include <wiringPi.h>
// gpio write 0 0 // первый 0 - это номер Пина(WiringPI), второй - это уровень(HIGH|LOW)
// gpio read 0 - Прочитать значение с Пина 0(WiringPI)
int main (void) {
	wiringPiSetup();
	pinMode (0, OUTPUT) ;
	for (;;) {
		digitalWrite(0, HIGH);
		delay (500) ;
		digitalWrite(0, LOW);
		delay(500);
	}
	return 0;
}
