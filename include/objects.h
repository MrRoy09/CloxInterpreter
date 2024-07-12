#pragma once
#include <string>
#include <functional>
#include <iostream>


class StringObject {
public:
	std::string string;
	StringObject(std::string string) {
		this->string = string;
	}

	std::string getString() {
		return this->string;
	}
};

class FunctionObject {
public:
	std::string funcName;
	int arity;

	FunctionObject(std::string name,int arity){
		this->funcName = name;
		this->arity = arity;
	}

	FunctionObject(std::string name) {
		this->funcName = name;
		this->arity = 0;
	}
	FunctionObject() {
		this->funcName = " ";
		this->arity = 0;
	}

	void printFunction() {
		std::cout << this->funcName<<"\n";
	}
};



