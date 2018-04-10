/*
 * Command.h
 *
 *  Created on: Aug 31, 2017
 *      Author: robin
 */

#ifndef PARSER_DATA_COMMAND_H_
#define PARSER_DATA_COMMAND_H_

enum CommandType {
	PEN, START, MOVE, HOME, LASER, CMD_ERROR
};

struct Command {
	double x, y, a;
	int angle, power;

	virtual ~Command() {}

	CommandType type;
};

struct PenCommand : public Command {
	int angle;

	PenCommand() {
		this->type = CommandType::PEN;
	}
};

struct StartCommand : public Command {
	StartCommand() {
		this->type = CommandType::START;
	}
};

struct MoveCommand : public Command {
//	double x, y, a;

	MoveCommand() {
		this->type = CommandType::MOVE;
	}
};

struct HomeCommand : public Command {
	HomeCommand() {
		this->type = CommandType::HOME;
	}
};


#endif /* PARSER_DATA_COMMAND_H_ */
