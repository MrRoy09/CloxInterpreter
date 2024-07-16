#pragma once
#include "chunk.h"
#include <iostream>
#include "debug.h"
#include <string>
#include "compiler.h"
#include "value.h"
#include "variant"
#include <unordered_map>
#include "native_functions.h"

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
	StackFrame(const std::string name, int offset, int *instruction_pointer) : function_name(name), stack_start_offset(offset), instruction_pointer_offset(instruction_pointer)
	{
	}

	const std::string function_name;
	int stack_start_offset;
	int *instruction_pointer_offset;
};

class StackFramePool
{
public:
	StackFrame *allocate(const std::string &name, size_t offset, int *instruction_p_offset)
	{
		if (!pool.empty())
		{
			StackFrame *frame = pool.back();
			pool.pop_back();
			new (frame) StackFrame(name, offset, instruction_p_offset); // Placement new
			return frame;
		}
		return new StackFrame(name, offset, instruction_p_offset);
	}

	void deallocate(StackFrame *frame)
	{
		frame->~StackFrame();
		pool.push_back(frame);
	}

	~StackFramePool()
	{
		for (auto frame : pool)
		{
			delete frame;
		}
	}

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

	bool checkStackFrameOverflow()
	{
		if (vm_stackFrames.size() > FRAMES_MAX)
		{
			return 0;
		}
		return 1;
	}

	InterpretResult interpret(std::string source)
	{
		vm_stackFrames.reserve(StackFrameReserveSize);
		stack.reserve(StackReserveSize);
		initNativeFunctions(&vm_native_functions);
		Chunk chunk = Chunk(1);
		vm_functions["main"] = std::make_shared<Chunk>(0);
		const char *source_c_str = source.c_str();
		Compiler compiler = Compiler(source_c_str, &vm_functions, &vm_native_functions);
		compiler.compile();
		this->chunk = vm_functions["main"].get();
		this->chunk->function.funcName = "main";
		this->instruction_pointer = this->chunk->opcodes.data();
		std::string name = "main";
		vm_stackFrames.push_back(framePool.allocate(name, this->stack.size(), this->instruction_pointer));
		InterpretResult result = run();
		return result;
	}

	void runtimeError()
	{
		std::cout << std::endl;
	}
	template <typename T, typename... Args>
	void runtimeError(T first, Args... args)
	{
		std::cout << first << " ";
		runtimeError(args...);
	}

	void destroyStackFrame()
	{
		int stack_offset = (vm_stackFrames.back())->stack_start_offset;
		int *instruction_pointer = (vm_stackFrames.back())->instruction_pointer_offset;
		framePool.deallocate(vm_stackFrames.back());
		vm_stackFrames.pop_back();
		stack.erase(stack.begin() + stack_offset, stack.end());
		this->instruction_pointer = instruction_pointer;
	}
#if defined(__clang__) || defined(__GNUC__)
	InterpretResult run_computedGoTo()
	{
		static void *dispatch_table[] = {
			&&RETURN,
			&&RETURN_VALUE,
			&&CONSTANT,
			&&NIL,
			&&TRUE,
			&&FALSE,
			&&NOT,
			&&EQUAL,
			&&GREATER,
			&&LESS,
			&&NEGATE,
			&&ADD,
			&&SUB,
			&&MUL,
			&&DIV,
			&&PRINT,
			&&POP,
			&&DEFINE_GLOBAL,
			&&GET_GLOBAL,
			&&SET_GLOBAL,
			&&GET_LOCAL,
			&&SET_LOCAL,
			&&JUMP_IF_FALSE,
			&&JUMP,
			&&LOOP,
			&&CALL,
		};

		bool isNativeFn = 0;
		bool isRecursive = 0;
		int arity = 0;

		DISPATCH();
		while (1)
		{

		RETURN:
			instruction_pointer += 1;
			if (chunk->function.funcName != "main")
			{
				destroyStackFrame();
				stack.push_back(Value());
				this->chunk = vm_functions[(vm_stackFrames.back())->function_name].get();
				this->chunk->function.funcName = (vm_stackFrames.back())->function_name;
			}
			else
			{
				return INTERPRET_OK;
			}
			DISPATCH();

		RETURN_VALUE:
		{
			instruction_pointer += 1;
			Value returnValue = stack.back();
			destroyStackFrame();
			if (vm_stackFrames.back()->function_name != this->chunk->function.funcName)
			{ // check if function is recursive, if it is, no need to lookup map
				this->chunk = vm_functions[(vm_stackFrames.back())->function_name].get();
				this->chunk->function.funcName = (vm_stackFrames.back())->function_name;
			}
			stack.emplace_back(returnValue);
		}
			DISPATCH();

		CONSTANT:
			stack.emplace_back(chunk->constants[*(instruction_pointer + 1)]);
			instruction_pointer += 2;
			DISPATCH();

		NEGATE:
			if (!std::holds_alternative<double>(stack.back().value))
			{
				runtimeError("Operand must be a double");
				return INTERPRET_RUNTIME_ERROR;
			}

			{
				double val = -(stack.back().returnDouble());
				stack.pop_back();
				stack.push_back(Value(val));
				instruction_pointer += 1;
			}
			DISPATCH();

		ADD:
			if (stack.back().value.index() != (stack.end() - 2)->value.index())
			{
				runtimeError("Cannot perform addition between given types");
				instruction_pointer += 1;
				return INTERPRET_RUNTIME_ERROR;
			};
			switch (stack.back().value.index())
			{
			case 0:
			{
				runtimeError("Operation not permitted between given types");
				instruction_pointer += 1;
				return INTERPRET_RUNTIME_ERROR;
				break;
			}
			case 1:
			{
				double val1 = stack.back().returnDouble();
				stack.pop_back();
				double val2 = stack.back().returnDouble();
				stack.pop_back();
				stack.emplace_back(Value(val1 + val2));
				instruction_pointer += 1;
				break;
			}
			case 2:
			{
				std::string val1 = stack.back().returnString();
				stack.pop_back();
				std::string val2 = stack.back().returnString();
				stack.pop_back();
				stack.emplace_back(Value(val2 + val1));
				instruction_pointer += 1;
				break;
			}
			}
			DISPATCH();

		SUB:
		{
			double val1 = -stack.back().returnDouble();
			stack.pop_back();
			double val2 = -stack.back().returnDouble();
			stack.pop_back();
			stack.emplace_back(Value(val1 - val2));
			instruction_pointer += 1;
		}
			DISPATCH();

		MUL:
		{
			double val1 = -stack.back().returnDouble();
			stack.pop_back();
			double val2 = -stack.back().returnDouble();
			stack.pop_back();
			stack.push_back(Value(val1 * val2));
		}
			instruction_pointer += 1;
			DISPATCH();

		DIV:
		{
			double val1 = -stack.back().returnDouble();
			stack.pop_back();
			double val2 = -stack.back().returnDouble();
			if (val1 != 0)
			{
				stack.pop_back();
				stack.push_back(Value(val2 / val1));
				instruction_pointer += 1;
			}
			else
			{
				std::cout << "Error division by zero at line " << chunk->lines[(instruction_pointer - chunk->opcodes.data()) / sizeof(int)];
				return INTERPRET_RUNTIME_ERROR;
			}
		}
			DISPATCH();

		NIL:
			stack.push_back(Value("nil"));
			instruction_pointer += 1;
			DISPATCH();

		TRUE:
		{
			bool val = 1;
			stack.push_back(Value(val));
			instruction_pointer += 1;
		}
			DISPATCH();

		FALSE:
		{
			bool val = 0;
			stack.push_back(Value(val));
			instruction_pointer += 1;
		}
			DISPATCH();

		NOT:
			if (std::holds_alternative<bool>(stack.back().value))
			{
				bool val = stack.back().returnBool();
				stack.pop_back();
				stack.push_back(Value(!val));
				instruction_pointer += 1;
			}
			else if (std::holds_alternative<double>(stack.back().value))
			{
				bool val = stack.back().returnDouble();
				stack.pop_back();
				stack.push_back(Value(!val));
				instruction_pointer += 1;
			}
			else if (stack.back().isNill)
			{
				bool val = 1;
				stack.pop_back();
				stack.push_back(Value(val));
				instruction_pointer += 1;
			}
			else
			{
				runtimeError("Error encountered in Not operator");
				instruction_pointer += 1;

				return INTERPRET_RUNTIME_ERROR;
			}
			DISPATCH();

		EQUAL:
		{
			Value a = stack.back();
			stack.pop_back();
			Value b = stack.back();
			stack.pop_back();
			stack.push_back(Value(a.ValuesEqual(b)));
			instruction_pointer += 1;
		}
			DISPATCH();

		GREATER:
		{
			Value a = stack.back();
			stack.pop_back();
			Value b = stack.back();
			stack.pop_back();
			bool val = a.returnDouble() < b.returnDouble();
			stack.push_back(Value(val));
			instruction_pointer += 1;
		}
			DISPATCH();

		LESS:
		{
			Value a = stack.back();
			stack.pop_back();
			Value b = stack.back();
			stack.pop_back();
			bool val = a.returnDouble() > b.returnDouble();
			stack.emplace_back(Value(val));
			instruction_pointer += 1;
		}
			DISPATCH();

		PRINT:
		{
			Value value = stack.back();
			stack.pop_back();
			value.printValue();
			std::cout << "\n";
			instruction_pointer += 1;
		}
			DISPATCH();

		POP:
			stack.pop_back();
			instruction_pointer += 1;
			DISPATCH();

		DEFINE_GLOBAL:
		{
			std::string name = chunk->constants[*(instruction_pointer + 1)].returnString();
			vm_globals[name] = stack.back();
			stack.pop_back();
			instruction_pointer += 2;
		}
			DISPATCH();

		GET_GLOBAL:
		{
			std::string name = chunk->constants[*(instruction_pointer + 1)].returnString();
			Value value;
			try
			{
				stack.emplace_back(std::move(vm_globals[name]));
			}
			catch (const std::out_of_range &e)
			{
				runtimeError("Unidenfied variable name ", name);
				return INTERPRET_RUNTIME_ERROR;
			}
			instruction_pointer += 2;
		}

			DISPATCH();

		SET_GLOBAL:
		{
			std::string name = chunk->constants[*(instruction_pointer + 1)].returnString();
			vm_globals[name] = stack.back();
			instruction_pointer += 2;
		}
			DISPATCH();

		GET_LOCAL:
		{
			int slot = *(++instruction_pointer) + (vm_stackFrames.back())->stack_start_offset;
			stack.push_back(stack[slot]);
			instruction_pointer++;
		}
			DISPATCH();

		SET_LOCAL:
		{
			uint8_t slot = *(++instruction_pointer) + (vm_stackFrames.back())->stack_start_offset;
			this->stack[slot] = stack.back();
			instruction_pointer += 1;
		}
			DISPATCH();

		JUMP_IF_FALSE:
		{
			instruction_pointer += 3;
			uint16_t offset = (uint16_t)(*(instruction_pointer - 2) << 8) | (*(instruction_pointer - 1));
			if (stack.back().returnBool() == false)
			{
				instruction_pointer += offset;
			}
		}
			DISPATCH();

		JUMP:
		{
			instruction_pointer += 3;
			uint16_t offset = (uint16_t)(*(instruction_pointer - 2) << 8) | (*(instruction_pointer - 1));
			instruction_pointer += offset;
		}
			DISPATCH();

		LOOP:
		{
			instruction_pointer += 3;
			uint16_t offset = (uint16_t)(*(instruction_pointer - 2) << 8) | (*(instruction_pointer - 1));
			instruction_pointer -= offset;
		}
			DISPATCH();

		CALL:
		{
			int offset = *(instruction_pointer + 1);
			std::string name_function = this->chunk->constants[offset].returnString();
			if (chunk->function.funcName == name_function)
			{
				isRecursive = 1;
			}
			else
			{
				isRecursive = 0;
				if (vm_native_functions.count(name_function) == 0)
				{
					isNativeFn = 0;
				}
				else
				{
					isNativeFn = 1;
				}
			}

			if (isNativeFn)
			{
				NativeFn function = vm_native_functions.at(name_function).function;
				Value *arguments = stack.size() == 0 ? NULL : &stack.back() - vm_native_functions.at(name_function).arguments + 1;
				stack.emplace_back(function(vm_native_functions.at(name_function).arguments, arguments));
				instruction_pointer += 2;
			}
			else
			{
				if (!isRecursive)
				{
					this->chunk = vm_functions[name_function].get();
					arity = vm_functions[name_function].get()->function.arity;
				}

				if (!checkStackFrameOverflow())
				{
					runtimeError("StackFrame overflow");
					return INTERPRET_RUNTIME_ERROR;
				}
				if (vm_stackFrames.size() > 2)
				{
					vm_stackFrames.emplace_back(framePool.allocate(name_function, stack.size() - vm_stackFrames[1]->stack_start_offset - arity, instruction_pointer + 2));
				}
				else
				{
					vm_stackFrames.emplace_back(framePool.allocate(name_function, stack.size() - vm_stackFrames.back()->stack_start_offset - arity, instruction_pointer + 2));
				}
				instruction_pointer = chunk->opcodes.data();
			}
		}
			DISPATCH();
		}
	}

#else
	InterpretResult run_switch()
	{

		bool isNativeFn = 0;
		bool isRecursive = 0;
		int arity = 0;
		while (1)
		{
			switch (*instruction_pointer)
			{
			case OP_RETURN:
				instruction_pointer += 1;
				if (chunk->function.funcName != "main")
				{
					destroyStackFrame();
					stack.push_back(Value());
					this->chunk = vm_functions[(vm_stackFrames.back())->function_name].get();
					this->chunk->function.funcName = (vm_stackFrames.back())->function_name;
				}
				else
				{
					return INTERPRET_OK;
				}
				break;

			case OP_RETURN_VALUE:
			{
				instruction_pointer += 1;
				Value returnValue = stack.back();
				destroyStackFrame();
				if (vm_stackFrames.back()->function_name != this->chunk->function.funcName)
				{ // check if function is recursive, if it is, no need to lookup map
					this->chunk = vm_functions[(vm_stackFrames.back())->function_name].get();
					this->chunk->function.funcName = (vm_stackFrames.back())->function_name;
				}
				stack.emplace_back(returnValue);
				break;
			}

			case OP_CONSTANT:
				stack.emplace_back(chunk->constants[*(instruction_pointer + 1)]);
				instruction_pointer += 2;
				break;

			case OP_NEGATE:
			{
				if (!std::holds_alternative<double>(stack.back().value))
				{
					runtimeError("Operand must be a double");
					return INTERPRET_RUNTIME_ERROR;
				}

				double val = -(stack.back().returnDouble());
				stack.pop_back();
				stack.push_back(Value(val));
				instruction_pointer += 1;
				break;
			}

			case OP_ADD:
			{
				if (stack.back().value.index() != (stack.end() - 2)->value.index())
				{
					runtimeError("Cannot perform addition between given types");
					instruction_pointer += 1;
					return INTERPRET_RUNTIME_ERROR;
					break;
				};
				switch (stack.back().value.index())
				{
				case 0:
				{
					runtimeError("Operation not permitted between given types");
					instruction_pointer += 1;
					return INTERPRET_RUNTIME_ERROR;
					break;
				}
				case 1:
				{
					double val1 = stack.back().returnDouble();
					stack.pop_back();
					double val2 = stack.back().returnDouble();
					stack.pop_back();
					stack.emplace_back(Value(val1 + val2));
					instruction_pointer += 1;
					break;
				}
				case 2:
				{
					std::string val1 = stack.back().returnString();
					stack.pop_back();
					std::string val2 = stack.back().returnString();
					stack.pop_back();
					stack.emplace_back(Value(val2 + val1));
					instruction_pointer += 1;
					break;
				}
				}
				break;
			}
			case OP_SUB:
			{
				double val1 = -stack.back().returnDouble();
				stack.pop_back();
				double val2 = -stack.back().returnDouble();
				stack.pop_back();
				stack.emplace_back(Value(val1 - val2));
				instruction_pointer += 1;
				break;
			}

			case OP_MUL:
			{
				double val1 = -stack.back().returnDouble();
				stack.pop_back();
				double val2 = -stack.back().returnDouble();
				stack.pop_back();
				stack.push_back(Value(val1 * val2));
				instruction_pointer += 1;
				break;
			}

			case OP_DIV:
			{
				double val1 = -stack.back().returnDouble();
				stack.pop_back();
				double val2 = -stack.back().returnDouble();
				if (val1 != 0)
				{
					stack.pop_back();
					stack.push_back(Value(val2 / val1));
					instruction_pointer += 1;
				}
				else
				{
					std::cout << "Error division by zero at line " << chunk->lines[(instruction_pointer - chunk->opcodes.data()) / sizeof(int)];
					return INTERPRET_RUNTIME_ERROR;
				}
				break;
			}

			case OP_NIL:
			{
				stack.push_back(Value("nil"));
				instruction_pointer += 1;
				break;
			}

			case OP_TRUE:
			{
				bool val = 1;
				stack.push_back(Value(val));
				instruction_pointer += 1;
				break;
			}

			case OP_FALSE:
			{
				bool val = 0;
				stack.push_back(Value(val));
				instruction_pointer += 1;
				break;
			}

			case OP_NOT:
			{
				if (std::holds_alternative<bool>(stack.back().value))
				{
					bool val = stack.back().returnBool();
					stack.pop_back();
					stack.push_back(Value(!val));
					instruction_pointer += 1;

					break;
				}
				else if (std::holds_alternative<double>(stack.back().value))
				{
					bool val = stack.back().returnDouble();
					stack.pop_back();
					stack.push_back(Value(!val));
					instruction_pointer += 1;

					break;
				}
				else if (stack.back().isNill)
				{
					bool val = 1;
					stack.pop_back();
					stack.push_back(Value(val));
					instruction_pointer += 1;

					break;
				}
				else
				{
					runtimeError("Error encountered in Not operator");
					instruction_pointer += 1;

					return INTERPRET_RUNTIME_ERROR;
					break;
				}
			}
			case OP_EQUAL:
			{
				Value a = stack.back();
				stack.pop_back();
				Value b = stack.back();
				stack.pop_back();
				stack.push_back(Value(a.ValuesEqual(b)));
				instruction_pointer += 1;

				break;
			}

			case OP_GREATER:
			{
				Value a = stack.back();
				stack.pop_back();
				Value b = stack.back();
				stack.pop_back();
				bool val = a.returnDouble() < b.returnDouble();
				stack.push_back(Value(val));
				instruction_pointer += 1;

				break;
			}

			case OP_LESS:
			{
				Value a = stack.back();
				stack.pop_back();
				Value b = stack.back();
				stack.pop_back();
				bool val = a.returnDouble() > b.returnDouble();
				stack.emplace_back(Value(val));
				instruction_pointer += 1;

				break;
			}

			case OP_PRINT:
			{
				Value value = stack.back();
				stack.pop_back();
				value.printValue();
				std::cout << "\n";
				instruction_pointer += 1;
				break;
			}

			case OP_POP:
			{
				stack.pop_back();
				instruction_pointer += 1;
				break;
			}

			case OP_DEFINE_GLOBAL:
			{
				std::string name = chunk->constants[*(instruction_pointer + 1)].returnString();
				vm_globals[name] = stack.back();
				stack.pop_back();
				instruction_pointer += 2;

				break;
			}

			case OP_GET_GLOBAL:
			{
				std::string name = chunk->constants[*(instruction_pointer + 1)].returnString();
				Value value;
				try
				{
					stack.emplace_back(vm_globals[name]);
				}
				catch (const std::out_of_range &e)
				{
					runtimeError("Unidenfied variable name ", name);
					return INTERPRET_RUNTIME_ERROR;
				}
				instruction_pointer += 2;

				break;
			}

			case OP_SET_GLOBAL:
			{
				std::string name = chunk->constants[*(instruction_pointer + 1)].returnString();
				vm_globals[name] = stack.back();
				instruction_pointer += 2;
				break;
			}

			case OP_GET_LOCAL:
			{
				int slot = *(++instruction_pointer) + (vm_stackFrames.back())->stack_start_offset;
				stack.push_back(stack[slot]);
				instruction_pointer++;
				break;
			}

			case OP_SET_LOCAL:
			{
				uint8_t slot = *(++instruction_pointer) + (vm_stackFrames.back())->stack_start_offset;
				this->stack[slot] = stack.back();
				instruction_pointer += 1;
				break;
			}

			case OP_JUMP_IF_FALSE:
			{
				instruction_pointer += 3;
				uint16_t offset = (uint16_t)(*(instruction_pointer - 2) << 8) | (*(instruction_pointer - 1));
				if (stack.back().returnBool() == false)
				{
					instruction_pointer += offset;
				}
				break;
			}

			case OP_JUMP:
			{
				instruction_pointer += 3;
				uint16_t offset = (uint16_t)(*(instruction_pointer - 2) << 8) | (*(instruction_pointer - 1));
				instruction_pointer += offset;
				break;
			}
			case OP_LOOP:
			{
				instruction_pointer += 3;
				uint16_t offset = (uint16_t)(*(instruction_pointer - 2) << 8) | (*(instruction_pointer - 1));
				instruction_pointer -= offset;
				break;
			}
			case OP_CALL:
			{
				int offset = *(instruction_pointer + 1);
				std::string name_function = this->chunk->constants[offset].returnString();
				if (chunk->function.funcName == name_function)
				{
					isRecursive = 1;
				}
				else
				{
					isRecursive = 0;
					if (vm_native_functions.count(name_function) == 0)
					{
						isNativeFn = 0;
					}
					else
					{
						isNativeFn = 1;
					}
				}

				if (isNativeFn)
				{
					NativeFn function = vm_native_functions.at(name_function).function;
					Value *arguments = stack.size() == 0 ? NULL : &stack.back() - vm_native_functions.at(name_function).arguments + 1;
					stack.emplace_back(function(vm_native_functions.at(name_function).arguments, arguments));
					instruction_pointer += 2;
					break;
				}
				else
				{
					if (!isRecursive)
					{
						this->chunk = vm_functions[name_function].get();
						arity = vm_functions[name_function].get()->function.arity;
					}

					if (!checkStackFrameOverflow())
					{
						runtimeError("StackFrame overflow");
						return INTERPRET_RUNTIME_ERROR;
						break;
					}
					if (vm_stackFrames.size() > 2)
					{
						vm_stackFrames.emplace_back(framePool.allocate(name_function, stack.size() - vm_stackFrames[1]->stack_start_offset - arity, instruction_pointer + 2));
					}
					else
					{
						vm_stackFrames.emplace_back(framePool.allocate(name_function, stack.size() - vm_stackFrames.back()->stack_start_offset - arity, instruction_pointer + 2));
					}
					instruction_pointer = chunk->opcodes.data();
					break;
				}
			}
			default:
				runtimeError("Unknown Instruction Encountered");
				return INTERPRET_RUNTIME_ERROR;
				break;
			}
		}
	}

#endif

	InterpretResult run()
	{
#ifdef __GNUC__
		InterpretResult result = run_computedGoTo();

#elif __clang__
		InterpretResult result = run_computedGoTo();

#else
		InterpretResult result = run_switch();
#endif
		return result;
	}
};