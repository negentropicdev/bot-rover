#include "../system/avr_serial.h"
#include "../system/avr_timer.h"

#include "../device/hcsr04.h"
#include "../device/dht.h"

#include <avr/io.h>
#include <avr/interrupt.h>

void init() {
    initTimer();
    initSerial(115200);

    sei();
}

int main() {
    init();

    HCSR04 sonar (&PORTD, &DDRD, &PIND, 3);

    DHT dht(&PORTD, &DDRD, &PIND, 2);

    float temp = 20;
    float humidity = 50;
    float scale = (331.4 + (0.606 * temp) + (0.0124 * humidity)) / 20000.0;

    unsigned long curMillis = millis();

    unsigned long dhtPeriod = 5000;
    unsigned long lastDht = curMillis;

    unsigned long rangePeriod = 100;
    unsigned long lastRange = curMillis - 5;

    bool d = true;
    bool r = true;

    while(1) {
        curMillis = millis();

        if (curMillis - lastDht >= dhtPeriod && d) {
            lastDht += dhtPeriod;
            
            int8_t res = dht.read();

            sei();

            if (res == DHT_OK) {
                temp = dht.getTemp();
                humidity = dht.getHumidity();
                
                scale = (331.4 + (0.606 * temp) + (0.0124 * humidity)) / 20000.0;

                printDec(temp);
                printf("C ");
                printDec(humidity);
                printf("%%\n");
            } else {
                printf("##%d\n", res);
            }
        }

        if (curMillis - lastRange >= rangePeriod && r) {
            lastRange += rangePeriod;

            unsigned long echo = sonar.range();

            float dist = echo * 2.375 * scale;
            printDec(dist);
            printf("cm\n");
        }
    }
}