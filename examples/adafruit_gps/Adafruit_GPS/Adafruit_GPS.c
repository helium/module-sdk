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
#define MAXLINELENGTH 120

// we double buffer: read one line in and leave one for the main program
char line1[MAXLINELENGTH];
char line2[MAXLINELENGTH];
// our index into filling the current line
volatile uint8_t lineidx = 0;
// pointers to the double buffers
volatile char *currentline;
volatile char *lastline;
volatile bool recvdflag;
volatile bool inStandbyMode;
uint8_t* buf;

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
      minutes = 50 * atol(degreebuff) / 3;
      gps->longitude_fixed   = degree + minutes;
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
    // Serial.println(p);
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
      minutes = 50 * atol(degreebuff) / 3;
      gps->longitude_fixed   = degree + minutes;
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

char readGPS(struct GPS* gps) {
  char c = 0;

  if (gps->paused) return c;

  c = (char)uart_read_byte(gps->gpsSerial, buf);
  printf("%c",c);

  if (c == '\n') {
    currentline[lineidx] = 0;

    if (currentline == line1) {
      currentline = line2;
      lastline    = line1;
    } else {
      currentline = line1;
      lastline    = line2;
    }

    lineidx   = 0;
    recvdflag = true;
  }

  currentline[lineidx++] = c;
  if (lineidx >= MAXLINELENGTH)
    lineidx = MAXLINELENGTH - 1;

  return c;
}

// Initialization code used by all constructor types
void GPS_init(struct GPS* gps, uint8_t serial_no) {
  gps->gpsSerial   = serial_no;
  gps->recvdflag   = false;
  gps->paused      = false;
  gps->lineidx     = 0;
  gps->currentline = line1;
  gps->lastline    = line2;

  gps->hour = gps->minute = gps->seconds = gps->year = gps->month = gps->day =
                                                                      gps->fixquality = gps->satellites = 0; // uint8_t
  gps->lat = gps->lon = gps->mag = 0; // char
  gps->fix = false; // boolean
  gps->milliseconds = 0; // uint16_t
  gps->latitude     = gps->longitude = gps->geoidheight = gps->altitude =
                                                            gps->speed = gps->angle = gps->magvariation = gps->HDOP =
                                                                                                            0.0; // float
}

void sendCommand(struct GPS* gps, const char *str) {
  uart_writestr(gps->gpsSerial, str);
}

void pauseGPS(struct GPS* gps, bool p) {
  gps->paused = p;
}

char *lastNMEA(void) {
  recvdflag = false;
  return (char *)lastline;
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
      char *nmea = lastNMEA();
      strncpy(str, nmea, 20);
      str[19] = 0;
      i++;
      if (strstr(str, wait4me)) return true;
    }
  }

  return false;
}

bool LOCUS_StartLogger(struct GPS* gps) {
  sendCommand(gps, PMTK_LOCUS_STARTLOG);
  gps->recvdflag = false;
  return waitForSentence(gps, PMTK_LOCUS_STARTSTOPACK);
}

bool LOCUS_StopLogger(struct GPS* gps) {
  sendCommand(gps, PMTK_LOCUS_STOPLOG);
  recvdflag = false;
  return waitForSentence(gps, PMTK_LOCUS_STARTSTOPACK);
}

bool LOCUS_ReadStatus(struct GPS* gps) {
  sendCommand(gps, PMTK_LOCUS_QUERY_STATUS);

  if (!waitForSentence(gps, "$PMTKLOG"))
    return false;

  char *response = lastNMEA();
  uint16_t parsed[10];
  uint8_t i;

  for (i = 0; i < 10; i++) parsed[i] = -1;

  response = strchr(response, ',');
  for (i = 0; i < 10; i++) {
    if (!response || (response[0] == 0) || (response[0] == '*'))
      break;
    response++;
    parsed[i] = 0;
    while ((response[0] != ',') &&
           (response[0] != '*') && (response[0] != 0)) {
      parsed[i] *= 10;
      char c = response[0];
      if (isdigit(c))
        parsed[i] += c - '0';
      else
        parsed[i] = c;
      response++;
    }
  }
  gps->LOCUS_serial = parsed[0];
  gps->LOCUS_type   = parsed[1];
  if (isalpha(parsed[2])) {
    parsed[2] = parsed[2] - 'a' + 10;
  }
  gps->LOCUS_mode     = parsed[2];
  gps->LOCUS_config   = parsed[3];
  gps->LOCUS_interval = parsed[4];
  gps->LOCUS_distance = parsed[5];
  gps->LOCUS_speed    = parsed[6];
  gps->LOCUS_status   = !parsed[7];
  gps->LOCUS_records  = parsed[8];
  gps->LOCUS_percent  = parsed[9];

  return true;
}

// Standby Mode Switches
bool standbyGPS(struct GPS* gps) {
  if (inStandbyMode) {
    return false;  // Returns false if already in standby mode, so that you do not wake it up by sending commands to GPS
  }else {
    inStandbyMode = true;
    sendCommand(gps, PMTK_STANDBY);
    // return waitForSentence(PMTK_STANDBY_SUCCESS);  // don't seem to be fast enough to catch the message, or something else just is not working
    return true;
  }
}

bool wakeupGPS(struct GPS* gps) {
  if (inStandbyMode) {
    inStandbyMode = false;
    sendCommand(gps, "");  // send byte to wake it up
    return waitForSentence(gps, PMTK_AWAKE);
  }else {
    return false;    // Returns false if not in standby mode, nothing to wakeup
  }
}
