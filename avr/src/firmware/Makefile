-include ../../common.mk

all: rover.hex sonar.hex

rover.hex: system.a base.a

i2ctest.hex: system.a

sertest.hex: system.a

blink.hex: avr_timer.o

testdrive.hex: system.a base.a

joyrover.hex: system.a base.a

pidtest.hex: system.a base.a

sonar.hex: system.a dht.o hcsr04.o

sonartest.hex: system.a hcsr04.o dht.o

enc_test.hex: system.a base.a

odom_test.hex: system.a base.a

pid_test.hex: system.a base.a