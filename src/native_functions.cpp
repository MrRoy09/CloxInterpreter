#include "native_functions.h"
#include <string>
#include <unordered_map>

auto start = std::chrono::steady_clock::now();

Value Clock(int argCount, Value* args) {
	auto now = std::chrono::steady_clock::now();
	double x = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
	return Value(x);
}

NativeFunction clock_function = NativeFunction(0, Clock);

void initNativeFunctions(std::unordered_map<std::string, NativeFunction>* natives) {
	natives->insert({"clock", clock_function});
}

