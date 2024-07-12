#pragma once
#include <variant>
#include <iostream>
#include <stdexcept>
#include "objects.h"

class Value {
public:
	bool isNill;
	std::variant<bool, double, std::string> value;

	Value(bool value) {
		this->isNill = 0;
		this->value = value;
	}

	Value(double value) {
		this->isNill = 0;
		this->value = value;
	}

	Value(std::string string) {
		this->isNill = 0;
		this->value = string;
	}

	Value() {
		this->isNill = 1;
	}

	void printValue() {
		if (isNill) {
			std::cout << "NILL" << "\n";
			return;
		}
		switch (value.index()) {
		case 0:
			std::cout << std::get<bool>(value) << "\n";
			break;
		case 1:
			std::cout << std::get<double>(value) << "\n";
			break;
		case 2:
			std::cout << std::get<std::string>(value) << "\n";
			break;
		}
	}

	bool returnBool() {
		if (std::holds_alternative<bool>(value)) {
			return std::get<bool>(value);
		}
		else if (std::holds_alternative<double>(value)) {
			return std::get<double>(value);
		}
		else {
			throw std::bad_variant_access();
		}
	}

	double returnDouble() {
		if (std::holds_alternative<double>(value)) {
			return std::get <double>(value);
		}
		else {
			throw std::bad_variant_access();
		}
	}

	std::string returnString() {
		if (std::holds_alternative<std::string>(value)) {
			return std::get <std::string>(value);
		}
		else {
			throw std::bad_variant_access();
		}
	}

	bool ValuesEqual(Value b) {
		if (this->value.index() != b.value.index()) return false;
		if (this->isNill && b.isNill) return true;

		switch (this->value.index()) {
		case 0:
			return this->returnBool() == b.returnBool();
			break;
		case 1:
			return this->returnDouble() == b.returnDouble();
			break;
		case 2:
			return this->returnString() == b.returnString();
		}
	}
};



