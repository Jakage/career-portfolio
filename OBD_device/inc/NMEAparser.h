/*
 * NMEAparser.h
 *
 *  Created on: 1.2.2018
 *      Author: Jake
 */

#ifndef NMEAPARSER_H_
#define NMEAPARSER_H_

class NMEAparser {
public:
	// constructor, initing a few variables
	NMEAparser();
	virtual ~NMEAparser();
	/*
	 * The serial data is assembled on the fly, without using any redundant buffers.
	 * When a sentence is complete (one that starts with $, ending in EOL), all processing is done on
	 * this temporary buffer that we've built: checksum computation, extracting sentence "words" (the CSV values),
	 * and so on.
	 * When a new sentence is fully assembled using the fusedata function, the code calls parsedata.
	 * This function in turn, splits the sentences and interprets the data. Here is part of the parser function,
	 * handling both the $GPRMC NMEA sentence:
	 */
	int fusedata(char c);

	// READER functions: retrieving results, call isdataready() first
	bool isdataready();
	int getHour();
	int getMinute();
	int getSecond();
	int getDay();
	int getMonth();
	int getYear();
	float getLatitude();
	float getLongitude();
	int getSatellites();
	float getAltitude();
	float getSpeed();
	float getBearing();

private:
	bool m_bFlagRead,					// flag used by the parser, when a valid sentence has begun
		 m_bFlagDataReady;				// valid GPS fix and data available, user can call reader functions
	char tmp_words[20][15],				// hold parsed words for one given NMEA sentence [20][15]
		 tmp_szChecksum[15];			// hold the received checksum for one given NMEA sentence [15]

	// will be set to true for characters between $ and * only
	bool m_bFlagComputedCks;			// used to compute checksum and indicate valid checksum interval (between $ and * in a given sentence)
	int m_nChecksum;					// numeric checksum, computed for a given sentence
	bool m_bFlagReceivedCks;			// after getting  * we start cuttings the received checksum
	int index_received_checksum;		// used to parse received checksum

	// word cutting variables
	int m_nWordIdx,					// the current word in a sentence
		m_nPrevIdx,						// last character index where we did a cut
		m_nNowIdx;						// current character index

	// globals to store parser results
	float res_fLongitude;					// GPRMC and GPGGA
	float res_fLatitude;					// GPRMC and GPGGA
	unsigned char res_nUTCHour, res_nUTCMin, res_nUTCSec,		// GPRMC and GPGGA
				  res_nUTCDay, res_nUTCMonth, res_nUTCYear;		// GPRMC
	int res_nSatellitesUsed;			// GPGGA
	float res_fAltitude;				// GPGGA
	float res_fSpeed;					// GPRMC
	float res_fBearing;					// GPRMC

	// the parser, currently handling GPRMC and GPGGA, but easy to add any new sentences
	void parsedata();
	// aux functions
	int digit2dec(char hexdigit);
	float string2float(char* s);
	int mstrcmp(const char *s1, const char *s2);
};

#endif /* NMEAPARSER_H_ */
