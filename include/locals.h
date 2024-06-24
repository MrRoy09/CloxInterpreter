#pragma once

class Local {
public:
	Token name;
	int depth;

	Local(Token name, int depth) {
		this->name = name;
		this->depth = depth;
	}
};