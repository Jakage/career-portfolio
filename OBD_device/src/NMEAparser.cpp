/*
 * NMEAparser.cpp
 *
 *  Created on: 1.2.2018
 *      Author: Jake
 */

#include "NMEAparser.h"
#include <stdio.h>
#include <stdlib.h>
#include <cmath>

NMEAparser::NMEAparser() {
	m_bFlagRead = false; //are we in a sentence?
	m_bFlagDataReady = false; //is data available?
};

NMEAparser::~NMEAparser(){
	// TODO Auto-generated destructor stub
}

/*
 * The serial data is assembled on the fly, without using any redundant buffers.
 * When a sentence is complete (one that starts with $, ending in EOL), all processing is done on
 * this temporary buffer that we've built: checksum computation, extracting sentence "words" (the CSV values),
 * and so on.
 * When a new sentence is fully assembled using the fusedata function, the code calls parsedata.
 * This function in turn, splits the sentences and interprets the data. Here is part of the parser function,
 * handling both the $GPRMC NMEA sentence:
 */
int NMEAparser::fusedata(char c) {
	if (c == '$') {
		m_bFlagRead = true;
		// init parser vars
		m_bFlagComputedCks = false;
		m_nChecksum = 0;
		// after getting * we start cuttings the received m_nChecksum
		m_bFlagReceivedCks = false;
		index_received_checksum = 0;
		// word cutting variables
		m_nWordIdx = 0; m_nPrevIdx = 0; m_nNowIdx = 0;
	}

	if (m_bFlagRead) {
		// check ending
		if (c == '\r' || c == '\n') {
			// catch last ending item too
			tmp_words[m_nWordIdx][m_nNowIdx - m_nPrevIdx] = 0;
			m_nWordIdx++;
			// cut received m_nChecksum
			tmp_szChecksum[index_received_checksum] = 0;
			// sentence complete, read done
			m_bFlagRead = false;
			// parse
			parsedata();
		} else {
			// computed m_nChecksum logic: count all chars between $ and * exclusively
			if (m_bFlagComputedCks && c == '*') m_bFlagComputedCks = false;
			if (m_bFlagComputedCks) m_nChecksum ^= c;
			if (c == '$') m_bFlagComputedCks = true;
			// received m_nChecksum
			if (m_bFlagReceivedCks)  {
				tmp_szChecksum[index_received_checksum] = c;
				index_received_checksum++;
			}
			if (c == '*') m_bFlagReceivedCks = true;
			// build a word
			tmp_words[m_nWordIdx][m_nNowIdx - m_nPrevIdx] = c;
			if (c == ',') {
				tmp_words[m_nWordIdx][m_nNowIdx - m_nPrevIdx] = 0;
				m_nWordIdx++;
				m_nPrevIdx = m_nNowIdx;
			}
			else m_nNowIdx++;
		}
	}
	return m_nWordIdx;
}


/*
 * parse internal tmp_ structures, fused by pushdata, and set the data flag when done
 */
void NMEAparser::parsedata() {
	int received_cks = 16*digit2dec(tmp_szChecksum[0]) + digit2dec(tmp_szChecksum[1]);
	//uart1.Send("seq: [cc:%X][words:%d][rc:%s:%d]\r\n", m_nChecksum,m_nWordIdx, tmp_szChecksum, received_cks);
	// check checksum, and return if invalid!
	if (m_nChecksum != received_cks) {
		//m_bFlagDataReady = false;
		return;
	}
	/* $GPGGA
	 * $GPGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
	 * ex: $GPGGA,230600.501,4543.8895,N,02112.7238,E,1,03,3.3,96.7,M,39.0,M,,0000*6A,
	 *
	 * WORDS:
	 *  1    = UTC of Position
	 *  2    = Latitude
	 *  3    = N or S
	 *  4    = Longitude
	 *  5    = E or W
	 *  6    = GPS quality indicator (0=invalid; 1=GPS fix; 2=Diff. GPS fix)
	 *  7    = Number of satellites in use [not those in view]
	 *  8    = Horizontal dilution of position
	 *  9    = Antenna altitude above/below mean sea level (geoid)
	 *  10   = Meters  (Antenna height unit)
	 *  11   = Geoidal separation (Diff. between WGS-84 earth ellipsoid and mean sea level.
	 *      -geoid is below WGS-84 ellipsoid)
	 *  12   = Meters  (Units of geoidal separation)
	 *  13   = Age in seconds since last update from diff. reference station
	 *  14   = Diff. reference station ID#
	 *  15   = Checksum
	 */
	if (mstrcmp(tmp_words[0], "$GPGGA") == 0) {
		// Check GPS Fix: 0=no fix, 1=GPS fix, 2=Dif. GPS fix
		if (tmp_words[6][0] == '0') {
			// clear data
			res_fLatitude = 0;
			res_fLongitude = 0;
			m_bFlagDataReady = false;
			return;
		}
		// parse time
		res_nUTCHour = digit2dec(tmp_words[1][0]) * 10 + digit2dec(tmp_words[1][1]);
		res_nUTCMin = digit2dec(tmp_words[1][2]) * 10 + digit2dec(tmp_words[1][3]);
		res_nUTCSec = digit2dec(tmp_words[1][4]) * 10 + digit2dec(tmp_words[1][5]);
		// parse latitude and longitude in NMEA format
		res_fLatitude = string2float(tmp_words[2]);
		res_fLongitude = string2float(tmp_words[4]);
		// get decimal format
		if (tmp_words[3][0] == 'S') res_fLatitude  *= -1.0;
		if (tmp_words[5][0] == 'W') res_fLongitude *= -1.0;
		float degrees = trunc(res_fLatitude / 100.0f);
		float minutes = res_fLatitude - (degrees * 100.0f);
		res_fLatitude = degrees + minutes / 60.0f;
		degrees = trunc(res_fLongitude / 100.0f);
		minutes = res_fLongitude - (degrees * 100.0f);
		res_fLongitude = degrees + minutes / 60.0f;

		// parse number of satellites
		res_nSatellitesUsed = (int)string2float(tmp_words[7]);

		// parse altitude
		res_fAltitude = string2float(tmp_words[9]);

		// data ready
		m_bFlagDataReady = true;
	}

	/* $GPRMC
	 * note: a siRF chipset will not support magnetic headers.
	 * $GPRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a*hh
	 * ex: $GPRMC,230558.501,A,4543.8901,N,02112.7219,E,1.50,181.47,230213,,,A*66,
	 *
	 * WORDS:
	 *  1	 = UTC of position fix
	 *  2    = Data status (V=navigation receiver warning)
	 *  3    = Latitude of fix
	 *  4    = N or S
	 *  5    = Longitude of fix
	 *  6    = E or W
	 *  7    = Speed over ground in knots
	 *  8    = Track made good in degrees True, Bearing This indicates the direction that the device is currently moving in,
	 *       from 0 to 360, measured in “azimuth”.
	 *  9    = UT date
	 *  10   = Magnetic variation degrees (Easterly var. subtracts from true course)
	 *  11   = E or W
	 *  12   = Checksum
	 */
	if (mstrcmp(tmp_words[0], "$GPRMC") == 0) {
		// Check data status: A-ok, V-invalid
		if (tmp_words[2][0] == 'V') {
			// clear data
			res_fLatitude = 0;
			res_fLongitude = 0;
			m_bFlagDataReady = false;
			return;
		}
		// parse time
		res_nUTCHour = digit2dec(tmp_words[1][0]) * 10 + digit2dec(tmp_words[1][1]);
		res_nUTCMin = digit2dec(tmp_words[1][2]) * 10 + digit2dec(tmp_words[1][3]);
		res_nUTCSec = digit2dec(tmp_words[1][4]) * 10 + digit2dec(tmp_words[1][5]);
		// parse latitude and longitude in NMEA format
		res_fLatitude = string2float(tmp_words[3]);
		res_fLongitude = string2float(tmp_words[5]);
		// get decimal format
		if (tmp_words[4][0] == 'S') res_fLatitude  *= -1.0;
		if (tmp_words[6][0] == 'W') res_fLongitude *= -1.0;
		float degrees = trunc(res_fLatitude / 100.0f);
		float minutes = res_fLatitude - (degrees * 100.0f);
		res_fLatitude = degrees + minutes / 60.0f;
		degrees = trunc(res_fLongitude / 100.0f);
		minutes = res_fLongitude - (degrees * 100.0f);
		res_fLongitude = degrees + minutes / 60.0f;
		//parse speed
		// The knot (pronounced not) is a unit of speed equal to one nautical mile (1.852 km) per hour
		res_fSpeed = string2float(tmp_words[7]);
		res_fSpeed *= 1.852; // convert to km/h
		// parse bearing
		res_fBearing = string2float(tmp_words[8]);
		// parse UTC date
		res_nUTCDay = digit2dec(tmp_words[9][0]) * 10 + digit2dec(tmp_words[9][1]);
		res_nUTCMonth = digit2dec(tmp_words[9][2]) * 10 + digit2dec(tmp_words[9][3]);
		res_nUTCYear = digit2dec(tmp_words[9][4]) * 10 + digit2dec(tmp_words[9][5]);

		// data ready
		m_bFlagDataReady = true;
	}
}
/*
 * returns base-16 value of chars '0'-'9' and 'A'-'F';
 * does not trap invalid chars!
 */
int NMEAparser::digit2dec(char digit) {
	if (int(digit) >= 65) return int(digit) - 55;
	else return int(digit) - 48;
}

/* returns base-10 value of zero-terminated string
 * that contains only chars '+','-','0'-'9','.';
 * does not trap invalid strings!
 */
float NMEAparser::string2float(char* s) {
	long  integer_part = 0;
	float decimal_part = 0.0;
	float decimal_pivot = 0.1;
	bool isdecimal = false, isnegative = false;

	char c;
	while ( ( c = *s++) )  {
		// skip special/sign chars
		if (c == '-') { isnegative = true; continue; }
		if (c == '+') continue;
		if (c == '.') { isdecimal = true; continue; }

		if (!isdecimal) {
			integer_part = (10 * integer_part) + (c - 48);
		} else {
			decimal_part += decimal_pivot * (float)(c - 48);
			decimal_pivot /= 10.0;
		}
	}
	// add integer part
	decimal_part += (float)integer_part;

	// check negative
	if (isnegative)  decimal_part = - decimal_part;

	return decimal_part;
}

int NMEAparser::mstrcmp(const char *s1, const char *s2) {
	while((*s1 && *s2) && (*s1 == *s2))
		s1++,s2++;
	return *s1 - *s2;
}

bool NMEAparser::isdataready() {
	return m_bFlagDataReady;
}

int NMEAparser::getHour() {
	return res_nUTCHour;
}
int NMEAparser::getMinute() {
	return res_nUTCMin;
}
int NMEAparser::getSecond() {
	return res_nUTCSec;
}
int NMEAparser::getDay() {
	return res_nUTCDay;
}
int NMEAparser::getMonth() {
	return res_nUTCMonth;
}
int NMEAparser::getYear() {
	return res_nUTCYear;
}

float NMEAparser::getLatitude() {
	return res_fLatitude;
}

float NMEAparser::getLongitude() {
	return res_fLongitude;
}

int NMEAparser::getSatellites() {
	return res_nSatellitesUsed;
}

float NMEAparser::getAltitude() {
	return res_fAltitude;
}

float NMEAparser::getSpeed() {
	return res_fSpeed;
}

float NMEAparser::getBearing() {
	return res_fBearing;
}


