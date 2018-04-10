/*
 * GCode.h
 *
 *  Created on: Aug 31, 2017
 *      Author: robin
 */

#ifndef GCODE_H_
#define GCODE_H_

#include <string.h>
#include <queue>
#include <stdlib.h>

#include "data/Command.h"
#include "../util/StringUtil.h"

class GCode {
public:
	GCode();
	virtual ~GCode();

	Command feed(char* line);

private:
	Command getCommand(char *line);
	void parsePen(Command *command, char *line);
	void parseMove(Command *command, char *line);
	void parseLaser(Command *cmd, char *line);
};
#endif
