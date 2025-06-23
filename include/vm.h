#pragma once
#include "chunk.h"
#include <iostream>
#include "debug.h"
#include <string>
#include "value.h"
#include "variant"
#include <unordered_map>
#include "native_functions.h"

class Compiler;

#define FRAMES_MAX 1000
#define StackFrameReserveSize 50
#define StackReserveSize 50
#define DISPATCH() goto *dispatch_table[*instruction_pointer]

typedef enum
{
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR,
} InterpretResult;

class StackFrame
{
public:
	StackFrame(const std::string name, int offset, int *instruction_pointer);

	const std::string function_name;
	int stack_start_offset;
	int *instruction_pointer_offset;
};

class StackFramePool
{
public:
	StackFrame *allocate(const std::string &name, size_t offset, int *instruction_p_offset);
	void deallocate(StackFrame *frame);
	~StackFramePool();

private:
	std::vector<StackFrame *> pool; // Pool of reusable StackFrames
};

class VM
{
public:
	int *instruction_pointer;
	Chunk *chunk;
	std::vector<Value> stack;
	std::unordered_map<std::string, Value> vm_globals;
	std::unordered_map<std::string, std::shared_ptr<Chunk>> vm_functions;
	std::unordered_map<std::string, NativeFunction> vm_native_functions;
	std::vector<StackFrame *> vm_stackFrames;
	StackFramePool framePool;

	bool checkStackFrameOverflow();
	InterpretResult interpret(std::string source);
	void runtimeError();
	template <typename T, typename... Args>
	void runtimeError(T first, Args... args)
	{
		std::cout << first << " ";
		runtimeError(args...);
	}
	void destroyStackFrame();
#if defined(__clang__) || defined(__GNUC__)
	InterpretResult run_computedGoTo();
#else
	InterpretResult run_switch();
#endif
	InterpretResult run();
};