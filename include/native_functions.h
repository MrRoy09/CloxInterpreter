#pragma once
#include "value.h"
#include "objects.h"
#include <chrono>
#include <unordered_map>
#include <string>


using NativeFn = Value(*)(int, Value*);
Value Clock(int argCount, Value* args);

class NativeFunction {
public:
	int arguments;
	NativeFn function;

	NativeFunction(int args,NativeFn function) {	
		this->arguments = args;
		this->function = function;
	}
};

void initNativeFunctions(std::unordered_map<std::string, NativeFunction>* vm_native_functions);
