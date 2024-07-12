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

typedef enum
{
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR,
} InterpretResult;

class StackFrame
{
public:
	StackFrame(const std::string name, int offset, int ipoffset) : function_name(name), stack_start_offset(offset), ip_offset(ipoffset)
	{
	}

	const std::string function_name;
	int stack_start_offset;
	int ip_offset;
};

class StackFramePool
{
public:
	StackFrame *allocate(const std::string &name, size_t offset, size_t ip)
	{
		if (!pool.empty())
		{
			StackFrame *frame = pool.back();
			pool.pop_back();
			new (frame) StackFrame(name, offset, ip); // Placement new
			return frame;
		}
		return new StackFrame(name, offset, ip);
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
	int ip;
	Chunk *chunk;
	std::vector<Value> stack;
	std::unordered_map<std::string, Value> vm_globals;
	std::unordered_map<std::string, std::shared_ptr<Chunk>> vm_functions;
	std::unordered_map<std::string, NativeFunction> vm_native_functions;
	std::vector<StackFrame *> vm_stackFrames;
	StackFramePool framePool;

	InterpretResult interpret(std::string source)
	{
		vm_stackFrames.reserve(StackFrameReserveSize);
		stack.reserve(StackReserveSize);
		initNativeFunctions(&vm_native_functions);
		Chunk chunk = Chunk(1);
		vm_functions["main"] = std::make_shared<Chunk>(0);
		const char *source_c_str = source.c_str();
		Compiler compiler = Compiler(source_c_str, &vm_functions, &vm_native_functions);
		bool compilation_result = compiler.compile();
		this->chunk = vm_functions["main"].get();
		this->chunk->function.funcName = "main";
		this->ip = 0;
		std::string name = "main";
		vm_stackFrames.push_back(framePool.allocate(name, this->stack.size(), 0));
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
		int ip_offset = (vm_stackFrames.back())->ip_offset;
		framePool.deallocate(vm_stackFrames.back());
		vm_stackFrames.pop_back();
		stack.erase(stack.begin() + stack_offset, stack.end());
		this->ip = ip_offset;
	}

	InterpretResult run()
	{

		// Used for function call, helps avoid re-assigning same variable incase of recursive function call
		bool isNativeFn = 0;
		bool isRecursive = 0;
		int arity = 0;

		int size = chunk->opcodes.size();
		while (ip < size)
		{
			int opcode = chunk->opcodes[this->ip];
			switch (opcode)
			{
			case OP_RETURN:
				ip += 1;
				if (chunk->function.funcName != "main")
				{
					destroyStackFrame();
					stack.push_back(Value());
					this->chunk = vm_functions[(vm_stackFrames.back())->function_name].get();
					this->chunk->function.funcName = (vm_stackFrames.back())->function_name;
					size = this->chunk->opcodes.size();
				}
				break;

			case OP_RETURN_VALUE:
			{
				ip += 1;
				Value returnValue = stack.back();
				destroyStackFrame();
				if (vm_stackFrames.back()->function_name != this->chunk->function.funcName)
				{ // check if function is recursive, if it is, no need to lookup map
					this->chunk = vm_functions[(vm_stackFrames.back())->function_name].get();
					this->chunk->function.funcName = (vm_stackFrames.back())->function_name;
					size = this->chunk->opcodes.size();
				}

				stack.emplace_back(returnValue);
				break;
			}

			case OP_CONSTANT:
				stack.emplace_back(chunk->constants[chunk->opcodes[this->ip + 1]]);
				ip += 2;
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
				ip += 1;
				break;
			}

			case OP_ADD:
			{
				if (stack.back().value.index() != (stack.end() - 2)->value.index())
				{
					runtimeError("Cannot perform addition between given types");
					ip += 1;
					return INTERPRET_RUNTIME_ERROR;
					break;
				};
				switch (stack.back().value.index())
				{
				case 0:
				{
					runtimeError("Operation not permitted between given types");
					ip += 1;
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
					ip += 1;
					break;
				}
				case 2:
				{
					std::string val1 = stack.back().returnString();
					stack.pop_back();
					std::string val2 = stack.back().returnString();
					stack.pop_back();
					stack.emplace_back(Value(val2 + val1));
					ip += 1;
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
				stack.emplace_back(std::move(Value(val1 - val2)));
				ip += 1;
				break;
			}

			case OP_MUL:
			{
				double val1 = -stack.back().returnDouble();
				stack.pop_back();
				double val2 = -stack.back().returnDouble();
				stack.pop_back();
				stack.push_back(Value(val1 * val2));
				ip += 1;
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
					ip += 1;
				}
				else
				{
					std::cout << "Error division by zero at line " << chunk->lines[ip];
					return INTERPRET_RUNTIME_ERROR;
				}
				break;
			}

			case OP_NIL:
			{
				stack.push_back(Value("nil"));
				ip++;

				break;
			}

			case OP_TRUE:
			{
				bool val = 1;
				stack.push_back(Value(val));
				ip++;

				break;
			}

			case OP_FALSE:
			{
				bool val = 0;
				stack.push_back(Value(val));
				ip++;

				break;
			}

			case OP_NOT:
			{
				if (std::holds_alternative<bool>(stack.back().value))
				{
					bool val = stack.back().returnBool();
					stack.pop_back();
					stack.push_back(Value(!val));
					ip++;
					break;
				}
				else if (std::holds_alternative<double>(stack.back().value))
				{
					bool val = stack.back().returnDouble();
					stack.pop_back();
					stack.push_back(Value(!val));
					ip++;
					break;
				}
				else if (stack.back().isNill)
				{
					bool val = 1;
					stack.pop_back();
					stack.push_back(Value(val));
					ip++;
					break;
				}
				else
				{
					runtimeError("Error encountered in Not operator");
					ip++;
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
				ip++;
				break;
			}

			case OP_GREATER:
			{
				Value a = stack.back();
				stack.pop_back();
				Value b = stack.back();
				stack.pop_back();
				bool val = a.returnDouble() < b.returnDouble();
				stack.push_back(std::move(Value(val)));
				ip++;
				break;
			}

			case OP_LESS:
			{
				Value a = stack.back();
				stack.pop_back();
				Value b = stack.back();
				stack.pop_back();
				bool val = a.returnDouble() > b.returnDouble();
				stack.emplace_back(std::move(Value(val)));
				ip++;
				break;
			}

			case OP_PRINT:
			{
				Value value = stack.back();
				stack.pop_back();
				value.printValue();
				printf("\n");
				ip++;
				break;
			}

			case OP_POP:
			{
				stack.pop_back();
				ip++;
				break;
			}

			case OP_DEFINE_GLOBAL:
			{
				std::string name = chunk->constants[chunk->opcodes[this->ip + 1]].returnString();
				vm_globals[name] = stack.back();
				stack.pop_back();
				ip += 2;
				break;
			}

			case OP_GET_GLOBAL:
			{
				std::string name = chunk->constants[chunk->opcodes[this->ip + 1]].returnString();
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
				ip += 2;
				break;
			}

			case OP_SET_GLOBAL:
			{
				std::string name = chunk->constants[chunk->opcodes[this->ip + 1]].returnString();
				vm_globals[name] = stack.back();
				ip += 2;
				break;
			}

			case OP_GET_LOCAL:
			{
				int slot = chunk->opcodes[++ip] + (vm_stackFrames.back())->stack_start_offset;
				stack.push_back(stack[slot]);
				ip += 1;
				break;
			}

			case OP_SET_LOCAL:
			{
				uint8_t slot = chunk->opcodes[++ip] + (vm_stackFrames.back())->stack_start_offset;
				this->stack[slot] = stack.back();
				ip += 1;
				break;
			}

			case OP_JUMP_IF_FALSE:
			{
				ip += 3;
				uint16_t offset = (uint16_t)((chunk->opcodes[ip - 2] << 8) | chunk->opcodes[ip - 1]);
				if (stack.back().returnBool() == false)
					ip += offset;
				break;
			}

			case OP_JUMP:
			{
				ip += 3;
				uint16_t offset = (uint16_t)((chunk->opcodes[ip - 2] << 8) | chunk->opcodes[ip - 1]);
				ip += offset;
				break;
			}
			case OP_LOOP:
			{
				ip += 3;
				uint16_t offset = (uint16_t)((chunk->opcodes[ip - 2] << 8) | chunk->opcodes[ip - 1]);
				ip -= offset;
				break;
			}
			case OP_CALL:
			{
				int offset = chunk->opcodes[ip + 1];
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
					ip += 2;
					break;
				}
				else
				{

					if (!isRecursive)
					{
						this->chunk = vm_functions[name_function].get();
						arity = vm_functions[name_function].get()->function.arity;
						size = this->chunk->opcodes.size();
					}
					if (!checkStackFrameOverflow())
					{
						runtimeError("StackFrame overflow");
						return INTERPRET_RUNTIME_ERROR;
						break;
					}
					if (vm_stackFrames.size() > 2)
					{
						vm_stackFrames.emplace_back(framePool.allocate(name_function, stack.size() - vm_stackFrames[1]->stack_start_offset - arity, ip + 2));
					}
					else
					{
						vm_stackFrames.emplace_back(framePool.allocate(name_function, stack.size() - vm_stackFrames.back()->stack_start_offset - arity, ip + 2));
					}
					ip = 0;
					break;
				}
			}
			default:
				runtimeError("Unknown Instruction Encountered");
				return INTERPRET_RUNTIME_ERROR;
				break;
			}
		}
		return INTERPRET_OK;
	}

	void stack_trace()
	{
		if (stack.size() == 0)
		{
			std::cout << "Stack Empty" << "\n";
			return;
		}
		for (auto it = stack.begin(); it < stack.end(); ++it)
		{
			(it->printValue());
		}
		std::cout << "\n";
	}

	bool checkStackFrameOverflow()
	{
		if (vm_stackFrames.size() > FRAMES_MAX)
		{
			return 0;
		}
		return 1;
	}
};