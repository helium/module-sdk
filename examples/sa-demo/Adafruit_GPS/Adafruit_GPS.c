/***********************************
   This is our GPS library

   Adafruit invests time and resources providing this open source code,
   please support Adafruit and open-source hardware by purchasing
   products from Adafruit!

   Written by Limor Fried/Ladyada for Adafruit Industries.
   BSD license, check license.txt for more information
   All text above must be included in any redistribution
 ****************************************/
#include "Adafruit_GPS.h"
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uart.h>

// how long are max NMEA lines to parse?
#define MAXLINELENGTH 124

// we double buffer: read one line in and leave one for the main program
// our index into filling the current line
// volatile uint8_t lineidx = 0;
// pointers to the double buffers
uint8_t buf[MAXLINELENGTH];

bool parseGPS(struct GPS* gps, char *nmea) {
  // do checksum check
  // first look if we even have one
  if (nmea[strlen(nmea) - 4] == '*') {
    uint16_t sum = parseHexGPS(nmea[strlen(nmea) - 3]) * 16;
    sum += parseHexGPS(nmea[strlen(nmea) - 2]);

    // check checksum
    for (uint8_t i = 2; i < (strlen(nmea) - 4); i++) {
      sum ^= nmea[i];
    }
    if (sum != 0) {
      // bad checksum :(
      return false;
    }
  }
  int32_t degree;
  long minutes;
  char degreebuff[10];
  // look for a few common sentences
  if (strstr(nmea, "$GPGGA")) {
    // found GGA
    char *p = nmea;
    // get time
    p = strchr(p, ',') + 1;
    float timef   = atof(p);
    uint32_t time = timef;
    gps->hour    = time / 10000;
    gps->minute  = (time % 10000) / 100;
    gps->seconds = (time % 100);

    gps->milliseconds = fmod(timef, 1.0) * 1000;

    // parse out latitude
    p = strchr(p, ',') + 1;
    if (',' != *p) {
      strncpy(degreebuff, p, 2);
      p += 2;
      degreebuff[2] = '\0';
      degree        = atol(degreebuff) * 10000000;
      strncpy(degreebuff, p, 2); // minutes
      p += 3; // skip decimal point
      strncpy(degreebuff + 2, p, 4);
      degreebuff[6]         = '\0';
      minutes               = 50 * atol(degreebuff) / 3;
      gps->latitude_fixed   = degree + minutes;
      gps->lat_degree       = degree;
      gps->lat_minutes      = minutes;
      gps->latitude         = degree / 100000 + minutes * 0.000006F;
      gps->latitudeDegrees  = (gps->latitude - 100 * (int)(gps->latitude / 100)) / 60.0;
      gps->latitudeDegrees += (int)(gps->latitude / 100);
    }

    p = strchr(p, ',') + 1;
    if (',' != *p) {
      if (p[0] == 'S') gps->latitudeDegrees *= -1.0;
      if (p[0] == 'N') gps->lat = 'N';
      else if (p[0] == 'S') gps->lat = 'S';
      else if (p[0] == ',') gps->lat = 0;
      else return false;
    }

    // parse out longitude
    p = strchr(p, ',') + 1;
    if (',' != *p) {
      strncpy(degreebuff, p, 3);
      p += 3;
      degreebuff[3] = '\0';
      degree        = atol(degreebuff) * 10000000;
      strncpy(degreebuff, p, 2); // minutes
      p += 3; // skip decimal point
      strncpy(degreebuff + 2, p, 4);
      degreebuff[6] = '\0';
      minutes       = 50 * atol(degreebuff) / 3;
      gps->longitude_fixed   = degree + minutes;
      gps->lon_degree           = degree;
      gps->lon_minutes          = minutes;
      gps->longitude         = degree / 100000 + minutes * 0.000006F;
      gps->longitudeDegrees  = (gps->longitude - 100 * (int)(gps->longitude / 100)) / 60.0;
      gps->longitudeDegrees += (int)(gps->longitude / 100);
    }

    p = strchr(p, ',') + 1;
    if (',' != *p) {
      if (p[0] == 'W') gps->longitudeDegrees *= -1.0;
      if (p[0] == 'W') gps->lon = 'W';
      else if (p[0] == 'E') gps->lon = 'E';
      else if (p[0] == ',') gps->lon = 0;
      else return false;
    }

    p = strchr(p, ',') + 1;
    if (',' != *p) {
      gps->fixquality = atoi(p);
    }

    p = strchr(p, ',') + 1;
    if (',' != *p) {
      gps->satellites = atoi(p);
    }

    p = strchr(p, ',') + 1;
    if (',' != *p) {
      gps->HDOP = atof(p);
    }

    p = strchr(p, ',') + 1;
    if (',' != *p) {
      gps->altitude = atof(p);
    }

    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    if (',' != *p) {
      gps->geoidheight = atof(p);
    }
    return true;
  }
  if (strstr(nmea, "$GPRMC")) {
    // found RMC
    char *p = nmea;

    // get time
    p = strchr(p, ',') + 1;
    float timef   = atof(p);
    uint32_t time = timef;
    gps->hour    = time / 10000;
    gps->minute  = (time % 10000) / 100;
    gps->seconds = (time % 100);

    gps->milliseconds = fmod(timef, 1.0) * 1000;

    p = strchr(p, ',') + 1;
    if (p[0] == 'A')
      gps->fix = true;
    else if (p[0] == 'V')
      gps->fix = false;
    else
      return false;

    // parse out latitude
    p = strchr(p, ',') + 1;
    if (',' != *p) {
      strncpy(degreebuff, p, 2);
      p += 2;
      degreebuff[2] = '\0';
      degree        = atol(degreebuff) * 10000000;
      strncpy(degreebuff, p, 2); // minutes
      p += 3; // skip decimal point
      strncpy(degreebuff + 2, p, 4);
      degreebuff[6]         = '\0';
      minutes               = 50 * atol(degreebuff) / 3;
      gps->latitude_fixed   = degree + minutes;
      gps->lat_degree           = degree;
      gps->lat_minutes          = minutes;
      gps->latitude         = degree / 100000 + minutes * 0.000006F;
      gps->latitudeDegrees  = (gps->latitude - 100 * (int)(gps->latitude / 100)) / 60.0;
      gps->latitudeDegrees += (int)(gps->latitude / 100);
    }

    p = strchr(p, ',') + 1;
    if (',' != *p) {
      if (p[0] == 'S') gps->latitudeDegrees *= -1.0;
      if (p[0] == 'N') gps->lat = 'N';
      else if (p[0] == 'S') gps->lat = 'S';
      else if (p[0] == ',') gps->lat = 0;
      else return false;
    }

    // parse out longitude
    p = strchr(p, ',') + 1;
    if (',' != *p) {
      strncpy(degreebuff, p, 3);
      p += 3;
      degreebuff[3] = '\0';
      degree        = atol(degreebuff) * 10000000;
      strncpy(degreebuff, p, 2); // minutes
      p += 3; // skip decimal point
      strncpy(degreebuff + 2, p, 4);
      degreebuff[6] = '\0';
      minutes       = 50 * atol(degreebuff) / 3;
      gps->longitude_fixed   = degree + minutes;
      gps->lon_degree           = degree;
      gps->lon_minutes          = minutes;
      gps->longitude         = degree / 100000 + minutes * 0.000006F;
      gps->longitudeDegrees  = (gps->longitude - 100 * (int)(gps->longitude / 100)) / 60.0;
      gps->longitudeDegrees += (int)(gps->longitude / 100);
    }

    p = strchr(p, ',') + 1;
    if (',' != *p) {
      if (p[0] == 'W') gps->longitudeDegrees *= -1.0;
      if (p[0] == 'W') gps->lon = 'W';
      else if (p[0] == 'E') gps->lon = 'E';
      else if (p[0] == ',') gps->lon = 0;
      else return false;
    }
    // speed
    p = strchr(p, ',') + 1;
    if (',' != *p) {
      gps->speed = atof(p);
    }

    // angle
    p = strchr(p, ',') + 1;
    if (',' != *p) {
      gps->angle = atof(p);
    }

    p = strchr(p, ',') + 1;
    if (',' != *p) {
      uint32_t fulldate = atof(p);
      gps->day   = fulldate / 10000;
      gps->month = (fulldate % 10000) / 100;
      gps->year  = (fulldate % 100);
    }
    // we dont parse the remaining, yet!
    return true;
  }

  return false;
}

// Initialization code used by all constructor types
void GPS_init(struct GPS* gps) {
  gps->recvdflag   = false;
  gps->paused      = false;
  gps->lineidx     = 0;
  gps->hour = gps->minute = gps->seconds = gps->year = gps->month = gps->day =
  gps->fixquality = gps->satellites = 0; // uint8_t
  gps->lat = gps->lon = gps->mag = 0; // char
  gps->fix = false; // boolean
  gps->milliseconds = 0; // uint16_t
  gps->latitude     = gps->longitude = gps->geoidheight = gps->altitude =
  gps->speed = gps->angle = gps->magvariation = gps->HDOP = 0.0; // float
}



void pauseGPS(struct GPS* gps, bool p) {
  gps->paused = p;
}

char *lastNMEA(struct GPS* gps) {
  gps->recvdflag = false;
  printf("%s \r\n", gps->lastline);
  return (char *)gps->lastline;
}

// read a Hex value and return the decimal equivalent
uint8_t parseHexGPS(char c) {
  if (c < '0')
    return 0;
  if (c <= '9')
    return c - '0';
  if (c < 'A')
    return 0;
  if (c <= 'F')
    return (c - 'A') + 10;
  // if (c > 'F')
  return 0;
}

bool waitForSentence(struct GPS* gps, const char *wait4me) {
  uint8_t max = MAXWAITSENTENCE;
  char str[20];

  uint8_t i = 0;
  while (i < max) {
    readGPS(gps);

    if (gps->recvdflag) {
      gps->recvdflag = false;
      char* nmea = gps->lastline;

      strncpy(str, nmea, 20);
      str[19] = 0;
      i++;
      if (strstr(str, wait4me)) return true;
    }
  }

  return false;
}


// Standby Mode Switches
bool standbyGPS(struct GPS* gps) {
  if (gps->inStandbyMode) {
    return false;  // Returns false if already in standby mode, so that you do not wake it up by sending commands to GPS
  }else {
    gps->inStandbyMode = true;
    sendCommand(gps, PMTK_STANDBY);
    // return waitForSentence(PMTK_STANDBY_SUCCESS);  // don't seem to be fast enough to catch the message, or something else just is not working
    return true;
  }
}

bool wakeupGPS(struct GPS* gps) {
  if (gps->inStandbyMode) {
    gps->inStandbyMode = false;
    sendCommand(gps, "");  // send byte to wake it up
    return waitForSentence(gps, PMTK_AWAKE);
  }else {
    return false;    // Returns false if not in standby mode, nothing to wakeup
  }
}