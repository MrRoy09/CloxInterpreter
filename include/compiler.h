#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <map>
#include "chunk.h"
#include "tokens.h"
#include "parser.h"
#include "value.h"
#include "objects.h"
#include "locals.h"
#include "native_functions.h"
#include "string_pool.h"

class Compiler
{
public:
	const char *source;
	Chunk *compiling_chunk;
	std::shared_ptr<Chunk> compiling_chunk_shared;
	Scanner scanner;
	Parser parser;
	bool had_error = 0;
	std::map<TokenType, ParseRule> parser_rules_map;
	std::unordered_map<std::string, std::shared_ptr<Chunk>> *functions;
	std::unordered_map<std::string, NativeFunction> *native_functions;
	StringPool string_pool;

	Compiler(const char *source, std::unordered_map<std::string, std::shared_ptr<Chunk>> *vm_functions, std::unordered_map<std::string, NativeFunction> *native_functions);

	bool compile();

	void declaration();

	void varDeclaration();

	uint8_t parseVariable(const char *errorMessage);

	uint8_t identifierConstant(Token *name);

	void defineVariable(uint8_t global);

	void declareVariable();

	void addLocal(Token name);

	void markInitialized();

	void statement();

	bool match(TokenType type);

	bool check(TokenType type);

	void funDeclaration();

	void createFunction(std::string name, int arity);

	void call(std::string function_name, int num_arguments);

	bool check_function_call();

	void printStatement();

	void expressionStatement();

	void emitByte(int byte);

	void emitBytes(int byte1, int byte2);

	void expression();

	void parsePrecedence(Precedence precedence);

	void number();

	void string();

	void variable();

	void namedVariable(Token name);

	int resolveLocal(Token *name);

	void ifStatement();

	void whileStatement();

	void forStatement();

	void emitLoop(int start);

	int emitJump(uint8_t instruction);

	void patchJump(int offset);
	void and_();

	void or_();

	void emitConstant(Value value);

	int makeConstant(Value value);

	void grouping();

	void unary();
	void binary();
	void literal();

	void block();

	void beginScope();

	void endScope();

	bool identifiersEqual(Token *a, Token *b);
};
