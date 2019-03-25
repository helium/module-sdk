# Makefile for user application
# Specify this directory relative to the current application.
TOCK_USERLAND_BASE_DIR = ../..

VPATH_DIRS += ./Adafruit_GPS/
C_SRCS += Adafruit_GPS.c

VPATH_DIRS += ./bosch_port/
C_SRCS += bosch_port.c

VPATH_DIRS += ./bmi160/
C_SRCS += bmi160.c bmi160_port.c

C_SRCS += $(wildcard *.c)


# Include userland master makefile. Contains rules and flags for actually
# building the application.
include $(TOCK_USERLAND_BASE_DIR)/AppMakefile.mk
