/*
 * GCode.cpp
 *
 *  Created on: Aug 31, 2017
 *      Author: robin
 */

#include "GCode.h"

GCode::GCode() {

}

GCode::~GCode() {
}

Command GCode::getCommand(char *line) {
	Command cmd;
	cmd.type = CommandType::CMD_ERROR;
	int code = atoi(line + 1);

	// TODO: This can be split up into extra functions example: getMCommand/getGCommand
	if (line[0] == 'G' && code == 1) {
		cmd.type = CommandType::MOVE;
	} else if (line[0] == 'G' && code == 28) {
		cmd.type = CommandType::HOME;
	} else if (line[0] == 'M' && code == 1) {
		cmd.type = CommandType::PEN;
	} else if (line[0] == 'M' && code == 10) {
		cmd.type = CommandType::START;
	} else if (line[0] == 'M' && code == 4) {
		cmd.type = CommandType::LASER;
	}

	return cmd;
}

Command GCode::feed(char *line) {
	Command command = this->getCommand(line);
	if (command.type == CommandType::PEN) {
		this->parsePen(&command, line);
	} else if (command.type == CommandType::MOVE) {
		this->parseMove(&command, line);
	} else if (command.type == CommandType::LASER) {
		this->parseLaser(&command, line);
	}

	return command;
}

void GCode::parseLaser(Command *cmd, char *line) {
	char *token;
	token = strtok(line, " ");
	while (token != NULL) {
		token = strtok(NULL, " ");
		if (token != NULL) {
			cmd->power = atoi(token);
		}
	}
}

void GCode::parseMove(Command *cmd, char *line) {
	char * token;
	token = strtok(line + 1, " ");
	while (token != NULL) {
		token = strtok(NULL, " ");
		if (token != NULL) {
			if (token[0] == 'X') {
				cmd->x = atof(token + 1);
			} else if (token[0] == 'Y') {
				cmd->y = atof(token + 1);
			} else if (token[0] == 'A') {
				cmd->a = atof(token + 1);
			}
		}
	}
}

void GCode::parsePen(Command *cmd, char *line) {
	char * token;
	token = strtok(line, " ");
	while (token != NULL) {
		token = strtok(NULL, " ");
		if (token != NULL) {
			cmd->angle = atoi(token);
		}
	}
}
