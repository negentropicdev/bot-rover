#These flags control build parameters and can be changed. They are used for host
#targeted builds. For AVR related flags, see avr.mk
#These flags are also used for 

CXXFLAGS= -O2 -Wno-unused-result
CFLAGS= -Wall
LDFLAGS=
ARFLAGS= rc

CLEANFILES+=*.o *~ *.a

#The pseudo targets below automate settings up the flags for various
#packages. To use them, add your targets as dependencies to these targets.
#IE for an executable that uses the raspicam shared libraries and opencv:
#   opencv: cam_test
#   raspicam: cam_test
#
#   cam_test: cam_test.o

.PHONY: opencv
opencv: CXXFLAGS += $(shell pkg-config opencv --cflags)
opencv: LDFLAGS += $(shell pkg-config opencv --libs)

.PHONY: raspicam
raspicam: CXXFLAGS += -I/usr/local/include
raspicam: LDFLAGS += -L/usr/local/lib -lraspicam_cv -lraspicam
