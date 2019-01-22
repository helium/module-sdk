# Makefile for user application
# Specify this directory relative to the current application.
TOCK_USERLAND_BASE_DIR = ../..

VPATH_DIRS += ./Adafruit_GPS/
C_SRCS += Adafruit_GPS.c

C_SRCS += $(wildcard *.c)


# Include userland master makefile. Contains rules and flags for actually
# building the application.
include $(TOCK_USERLAND_BASE_DIR)/AppMakefile.mk
