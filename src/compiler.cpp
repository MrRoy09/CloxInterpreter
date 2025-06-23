#include "compiler.h"
#include <cstring>

Compiler::Compiler(const char *source, std::unordered_map<std::string, std::shared_ptr<Chunk>> *vm_functions, std::unordered_map<std::string, NativeFunction> *native_functions) : parser(source, &scanner)
{
	this->source = source;
	this->functions = vm_functions;
	this->native_functions = native_functions;
	this->compiling_chunk = functions->at("main").get();
	this->scanner.start = source;
	this->scanner.current = source;
	this->scanner.line = 0;
	parser_rules_map[TOKEN_LEFT_PAREN] = {std::bind(&Compiler::grouping, this), NULL, PREC_NONE};
	parser_rules_map[TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_COMMA] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_DOT] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_MINUS] = {std::bind(&Compiler::unary, this), std::bind(&Compiler::binary, this), PREC_TERM};
	parser_rules_map[TOKEN_PLUS] = {NULL, std::bind(&Compiler::binary, this), PREC_TERM};
	parser_rules_map[TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_SLASH] = {NULL, std::bind(&Compiler::binary, this), PREC_FACTOR};
	parser_rules_map[TOKEN_STAR] = {NULL, std::bind(&Compiler::binary, this), PREC_FACTOR};
	parser_rules_map[TOKEN_BANG] = {std::bind(&Compiler::unary, this), NULL, PREC_NONE};
	parser_rules_map[TOKEN_BANG_EQUAL] = {NULL, std::bind(&Compiler::binary, this), PREC_COMPARISON};
	parser_rules_map[TOKEN_EQUAL] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_EQUAL_EQUAL] = {NULL, std::bind(&Compiler::binary, this), PREC_COMPARISON};
	parser_rules_map[TOKEN_GREATER] = {NULL, std::bind(&Compiler::binary, this), PREC_COMPARISON};
	parser_rules_map[TOKEN_GREATER_EQUAL] = {NULL, std::bind(&Compiler::binary, this), PREC_COMPARISON};
	parser_rules_map[TOKEN_LESS] = {NULL, std::bind(&Compiler::binary, this), PREC_COMPARISON};
	parser_rules_map[TOKEN_LESS_EQUAL] = {NULL, std::bind(&Compiler::binary, this), PREC_COMPARISON};
	parser_rules_map[TOKEN_IDENTIFIER] = {std::bind(&Compiler::variable, this), NULL, PREC_NONE};
	parser_rules_map[TOKEN_STRING] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_NUMBER] = {std::bind(&Compiler::number, this), NULL, PREC_NONE};
	parser_rules_map[TOKEN_AND] = {NULL, std::bind(&Compiler::and_, this), PREC_AND};
	parser_rules_map[TOKEN_CLASS] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_ELSE] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_FALSE] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_FOR] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_FUN] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_IF] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_NIL] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_OR] = {NULL, std::bind(&Compiler::or_, this), PREC_OR};
	parser_rules_map[TOKEN_PRINT] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_RETURN] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_SUPER] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_THIS] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_TRUE] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_VAR] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_WHILE] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_ERROR] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_EOF] = {NULL, NULL, PREC_NONE};
	parser_rules_map[TOKEN_TRUE] = {std::bind(&Compiler::literal, this), NULL, PREC_NONE};
	parser_rules_map[TOKEN_FALSE] = {std::bind(&Compiler::literal, this), NULL, PREC_NONE};
	parser_rules_map[TOKEN_NIL] = {std::bind(&Compiler::literal, this), NULL, PREC_NONE};
	parser_rules_map[TOKEN_BANG_EQUAL] = {NULL, std::bind(&Compiler::binary, this), PREC_EQUALITY};
	parser_rules_map[TOKEN_EQUAL_EQUAL] = {NULL, std::bind(&Compiler::binary, this), PREC_EQUALITY};
	parser_rules_map[TOKEN_GREATER] = {NULL, std::bind(&Compiler::binary, this), PREC_COMPARISON};
	parser_rules_map[TOKEN_GREATER_EQUAL] = {NULL, std::bind(&Compiler::binary, this), PREC_COMPARISON};
	parser_rules_map[TOKEN_LESS] = {NULL, std::bind(&Compiler::binary, this), PREC_COMPARISON};
	parser_rules_map[TOKEN_LESS_EQUAL] = {NULL, std::bind(&Compiler::binary, this), PREC_COMPARISON};
	parser_rules_map[TOKEN_STRING] = {std::bind(&Compiler::string, this), NULL, PREC_NONE};
}

bool Compiler::compile()
{
	parser.advance();
	while (!match(TOKEN_EOF))
	{
		declaration();
	}
	emitByte(OP_RETURN);
	return !(this->parser.had_error);
}

void Compiler::declaration()
{
	if (match(TOKEN_VAR))
	{
		varDeclaration();
	}
	else
	{
		statement();
	}
}

void Compiler::varDeclaration()
{
	uint8_t global = parseVariable("Expect variable name.");

	if (match(TOKEN_EQUAL))
	{
		if (check_function_call())
		{
			std::string function_name = parser.current.getText();
			parser.advance();
			parser.consume(TOKEN_LEFT_PAREN, "Expect ( after function call");
			int num_arguments = 0;
			if (!match(TOKEN_RIGHT_PAREN))
			{
				expression();
				num_arguments++;
				while (match(TOKEN_COMMA))
				{
					num_arguments++;
					expression();
				}
				parser.consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
			}
			call(function_name, num_arguments);
		}
		else
		{
			expression();
		}
	}
	else
	{
		emitByte(OP_NIL);
	}
	parser.consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

	defineVariable(global);
}

uint8_t Compiler::parseVariable(const char *errorMessage)
{
	parser.consume(TOKEN_IDENTIFIER, errorMessage);
	declareVariable();
	if (this->compiling_chunk->scopeDepth > 0)
		return 0;
	return identifierConstant(&parser.previous);
}

uint8_t Compiler::identifierConstant(Token *name)
{
	std::string identifierName = parser.previous.getText();
	Value value = Value(identifierName);
	return makeConstant(value);
}

void Compiler::defineVariable(uint8_t global)
{
	if (this->compiling_chunk->scopeDepth > 0)
	{
		markInitialized();
		return;
	}
	emitBytes(OP_DEFINE_GLOBAL, global);
}

void Compiler::declareVariable()
{
	if (this->compiling_chunk->scopeDepth == 0)
		return;

	Token name = parser.previous;
	if (this->compiling_chunk->locals.size() > this->compiling_chunk->localCount)
	{
		for (int i = this->compiling_chunk->localCount; i >= 0; i--)
		{
			Local *local = this->compiling_chunk->locals[i].get();
			if (local->depth != -1 && local->depth < this->compiling_chunk->scopeDepth)
			{
				break;
			}
			if (identifiersEqual(&name, &local->name))
			{
				std::cout << "Same variable name exists in this scope. " << "\n";
				break;
			}
		}
	}

	addLocal(name);
}

void Compiler::addLocal(Token name)
{
	this->compiling_chunk->locals.push_back(std::make_unique<Local>(name, -1));
	this->compiling_chunk->localCount++;
}

void Compiler::markInitialized()
{
	this->compiling_chunk->locals[this->compiling_chunk->localCount - 1]->depth = this->compiling_chunk->scopeDepth;
}

void Compiler::statement()
{
	if (match(TOKEN_PRINT))
	{
		printStatement();
	}
	else if (match(TOKEN_FUN))
	{
		funDeclaration();
	}

	else if (match(TOKEN_IF))
	{
		ifStatement();
	}

	else if (match(TOKEN_WHILE))
	{
		whileStatement();
	}

	else if (match(TOKEN_FOR))
	{
		forStatement();
	}
	else if (match(TOKEN_LEFT_BRACE))
	{
		beginScope();
		block();
		endScope();
	}
	else if (match(TOKEN_RETURN) && this->compiling_chunk->function.funcName != "main")
	{
		if (match(TOKEN_SEMICOLON))
		{
			emitByte(OP_RETURN);
		}
		else
		{
			expression();
			parser.consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
			emitByte(OP_RETURN_VALUE);
		}
	}
	else
	{
		expressionStatement();
	}
}

bool Compiler::match(TokenType type)
{
	if (!check(type))
		return false;
	parser.advance();
	return true;
}

bool Compiler::check(TokenType type)
{
	return parser.current.type == type;
}

void Compiler::funDeclaration()
{
	uint8_t global = parseVariable("Expect function name.");
	std::string func_name = parser.previous.getText();
	int arity = 0;

	// Save the current compilation context
	Chunk *enclosing = this->compiling_chunk;

	this->compiling_chunk_shared = std::make_shared<Chunk>(10); // 10 is the id for function chunks
	this->compiling_chunk = this->compiling_chunk_shared.get();

	parser.consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
	if (!match(TOKEN_RIGHT_PAREN))
	{
		uint8_t constant = parseVariable("Expect parameter name.");
		defineVariable(constant);
		arity++;
		while (match(TOKEN_COMMA))
		{
			arity++;
			uint8_t constant = parseVariable("Expect parameter name.");
			defineVariable(constant);
		}
		parser.consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
	}

	parser.consume(TOKEN_LEFT_BRACE, "Expect '{' after function declaration");
	createFunction(func_name, arity);
	
	// Parse the function body
	beginScope();
	block();
	endScope();
	
	// Ensure function returns something
	emitByte(OP_RETURN);
	
	// Restore the main compilation context
	this->compiling_chunk = enclosing;
}

void Compiler::createFunction(std::string name, int arity)
{
	FunctionObject function = FunctionObject(name, arity);
	if (functions->count(function.funcName) != 0 || native_functions->count(name) != 0)
	{
		std::cout << "redefinition of function found" << "\n";
		return;
	}

	this->compiling_chunk->function = function;
	this->functions->insert({function.funcName, this->compiling_chunk_shared});
}

void Compiler::call(std::string function_name, int num_arguments)
{
	if (functions->count(function_name) == 0 && native_functions->count(function_name) == 0)
	{
		std::cout << "Definition for " << function_name << " not found" << "\n";
		return;
	}
	if (functions->count(function_name) != 0)
	{
		if (functions->at(function_name)->function.arity != num_arguments)
		{
			std::cout << "not enuf arguments supplied" << "\n";
			return;
		}
	}
	int func_offset = makeConstant(Value(function_name));
	emitBytes(OP_CALL, func_offset);
}

bool Compiler::check_function_call()
{
	std::string func_name = parser.current.getText();
	return (functions->count(func_name) != 0 || native_functions->count(func_name) != 0);
}

void Compiler::printStatement()
{
	expression();
	parser.consume(TOKEN_SEMICOLON, "Expect ';' after value.");
	emitByte(OP_PRINT);
	return;
}

void Compiler::expressionStatement()
{
	expression();
	parser.consume(TOKEN_SEMICOLON, "EXPECT ';' after expression.");
	emitByte(OP_POP);
	return;
}

void Compiler::emitByte(int byte)
{
	compiling_chunk->WriteChunk(byte, parser.previous.line);
}

void Compiler::emitBytes(int byte1, int byte2)
{
	emitByte(byte1);
	emitByte(byte2);
}

void Compiler::expression()
{
	parsePrecedence(PREC_ASSIGNMENT);
}

void Compiler::parsePrecedence(Precedence precedence)
{
	parser.advance();
	std::function<void()> function1 = parser_rules_map[parser.previous.type].prefix;
	if (function1 == NULL)
	{
		parser.error("Expect expression.");
		return;
	}
	function1();

	while (precedence <= parser_rules_map[parser.current.type].precedence)
	{
		parser.advance();
		std::function<void()> infixRule = parser_rules_map[parser.previous.type].infix;
		infixRule();
	}
}

void Compiler::number()
{
	Value value = Value(strtod(parser.previous.getText().c_str(), NULL));
	emitConstant(value);
}

void Compiler::string()
{
	std::string token_text = parser.previous.getText();
	std::string string = token_text.substr(1, token_text.length() - 2);
	Value value = Value(string);
	emitConstant(value);
}

void Compiler::variable()
{

	if (parser.current.type == TOKEN_LEFT_PAREN)
	{
		std::string func_name = parser.previous.getText();
		parser.consume(TOKEN_LEFT_PAREN, "Expect '('");
		int num_arguments = 0;
		if (!match(TOKEN_RIGHT_PAREN))
		{
			expression();
			num_arguments++;
			while (match(TOKEN_COMMA))
			{
				num_arguments++;
				expression();
			}
			parser.consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
		}
		call(func_name, num_arguments);
	}
	else
	{
		namedVariable(parser.previous);
	}
}

void Compiler::namedVariable(Token name)
{
	uint8_t getOp, setOp;
	int arg = resolveLocal(&name);
	if (arg != -1)
	{
		getOp = OP_GET_LOCAL;
		setOp = OP_SET_LOCAL;
	}
	else
	{
		arg = identifierConstant(&name);
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}

	if (match(TOKEN_EQUAL))
	{
		if (check_function_call())
		{
			std::string function_name = parser.current.getText();
			parser.advance();
			parser.consume(TOKEN_LEFT_PAREN, "Expect ( after function call");
			int num_arguments = 0;
			if (!match(TOKEN_RIGHT_PAREN))
			{
				expression();
				num_arguments++;
				while (match(TOKEN_COMMA))
				{
					num_arguments++;
					expression();
				}
				parser.consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
			}
			call(function_name, num_arguments);
		}
		else
		{
			expression();
		}
		emitBytes(setOp, (int)arg);
	}
	else
	{
		emitBytes(getOp, (int)arg);
	}
}

int Compiler::resolveLocal(Token *name)
{
	for (int i = this->compiling_chunk->localCount - 1; i >= 0; i--)
	{
		Local *local = this->compiling_chunk->locals[i].get();
		if (identifiersEqual(name, &local->name))
		{
			if (local->depth == -1)
			{
				continue;
			}
			return i;
		}
	}
	return -1;
}

void Compiler::ifStatement()
{
	parser.consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	expression();
	parser.consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	int thenJump = emitJump(OP_JUMP_IF_FALSE);
	emitByte(OP_POP);
	statement();
	int elseJump = emitJump(OP_JUMP);
	patchJump(thenJump);
	emitByte(OP_POP);
	if (match(TOKEN_ELSE))
		statement();
	patchJump(elseJump);
}

void Compiler::whileStatement()
{
	int loopStart = compiling_chunk->opcodes.size();
	parser.consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
	expression();
	parser.consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
	int exitJump = emitJump(OP_JUMP_IF_FALSE);
	emitByte(OP_POP);
	statement();
	emitLoop(loopStart);
	patchJump(exitJump);
	emitByte(OP_POP);
}

void Compiler::forStatement()
{
	beginScope();
	parser.consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
	if (match(TOKEN_SEMICOLON))
	{
		// No initializer.
	}
	else if (match(TOKEN_VAR))
	{
		varDeclaration();
	}
	else
	{
		expressionStatement();
	}
	int loopStart = compiling_chunk->opcodes.size();
	int exitJump = -1;

	if (!match(TOKEN_SEMICOLON))
	{
		expression();
		parser.consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
		exitJump = emitJump(OP_JUMP_IF_FALSE);
		emitByte(OP_POP);
	}

	if (!match(TOKEN_RIGHT_PAREN))
	{
		int bodyJump = emitJump(OP_JUMP);
		int incrementStart = compiling_chunk->opcodes.size();
		expression();
		emitByte(OP_POP);
		parser.consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

		emitLoop(loopStart);
		loopStart = incrementStart;
		patchJump(bodyJump);
	}

	statement();
	emitLoop(loopStart);
	if (exitJump != -1)
	{
		patchJump(exitJump);
		emitByte(OP_POP); // Condition.
	}
	endScope();
}

void Compiler::emitLoop(int start)
{
	emitByte(OP_LOOP);
	int offset = compiling_chunk->opcodes.size() - start + 2;
	emitByte((offset >> 8) & 0xff);
	emitByte(offset & 0xff);
}

int Compiler::emitJump(uint8_t instruction)
{
	emitByte(instruction);
	emitByte(0xff);
	emitByte(0xff);
	return compiling_chunk->opcodes.size() - 2;
}

void Compiler::patchJump(int offset)
{
	int jump = compiling_chunk->opcodes.size() - offset - 2;

	compiling_chunk->opcodes[offset] = (jump >> 8) & 0xff;
	compiling_chunk->opcodes[offset + 1] = jump & 0xff;
}

void Compiler::and_()
{
	int endJump = emitJump(OP_JUMP_IF_FALSE);

	emitByte(OP_POP);
	parsePrecedence(PREC_AND);

	patchJump(endJump);
}

void Compiler::or_()
{
	int elseJump = emitJump(OP_JUMP_IF_FALSE);
	int endJump = emitJump(OP_JUMP);

	patchJump(elseJump);
	emitByte(OP_POP);

	parsePrecedence(PREC_OR);
	patchJump(endJump);
}

void Compiler::emitConstant(Value value)
{
	emitBytes(OP_CONSTANT, makeConstant(value));
}

int Compiler::makeConstant(Value value)
{
	int constant_offset = this->compiling_chunk->AddConstant(value);
	return constant_offset;
}

void Compiler::grouping()
{
	expression();
	parser.consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

void Compiler::unary()
{
	TokenType operatorType = parser.previous.type;
	parsePrecedence(PREC_UNARY);
	switch (operatorType)
	{
	case TOKEN_BANG:
		emitByte(OP_NOT);
		break;
	case TOKEN_MINUS:
		emitByte(OP_NEGATE);
		break;
	default:
		return;
	}
}

void Compiler::binary()
{
	TokenType operator_type = parser.previous.type;
	ParseRule *rule = &parser_rules_map[operator_type];
	parsePrecedence((Precedence)(rule->precedence + 1));
	switch (operator_type)
	{
	case TOKEN_PLUS:
		emitByte(OP_ADD);
		break;
	case TOKEN_MINUS:
		emitByte(OP_SUB);
		break;
	case TOKEN_STAR:
		emitByte(OP_MUL);
		break;
	case TOKEN_SLASH:
		emitByte(OP_DIV);
		break;
	case TOKEN_BANG_EQUAL:
		emitBytes(OP_EQUAL, OP_NOT);
		break;
	case TOKEN_EQUAL_EQUAL:
		emitByte(OP_EQUAL);
		break;
	case TOKEN_GREATER:
		emitByte(OP_GREATER);
		break;
	case TOKEN_GREATER_EQUAL:
		emitBytes(OP_LESS, OP_NOT);
		break;
	case TOKEN_LESS:
		emitByte(OP_LESS);
		break;
	case TOKEN_LESS_EQUAL:
		emitBytes(OP_GREATER, OP_NOT);
		break;
	default:
		return;
	}
}

void Compiler::literal()
{
	switch (parser.previous.type)
	{
	case TOKEN_FALSE:
		emitByte(OP_FALSE);
		break;
	case TOKEN_NIL:
		emitByte(OP_NIL);
		break;
	case TOKEN_TRUE:
		emitByte(OP_TRUE);
		break;
	default:
		return;
	}
}

void Compiler::block()
{
	while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
	{
		declaration();
	}

	parser.consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

void Compiler::beginScope()
{
	this->compiling_chunk->scopeDepth++;
}

void Compiler::endScope()
{
	this->compiling_chunk->scopeDepth--;
	while (this->compiling_chunk->localCount > 0 && this->compiling_chunk->locals[this->compiling_chunk->localCount - 1]->depth > this->compiling_chunk->scopeDepth)
	{
		this->compiling_chunk->locals.pop_back(); // destroy all variables with same scope
		emitByte(OP_POP);
		this->compiling_chunk->localCount--;
	}
}

bool Compiler::identifiersEqual(Token *a, Token *b)
{
	return a->getText() == b->getText();
}