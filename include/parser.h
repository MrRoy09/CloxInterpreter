#pragma once
class Scanner {
public:
	const char* start;
	const char* current;
	int line;
	bool isAtEnd() {
		return *current == '\0';
	}

	bool isDigit(char c) {
		if (c >= '0' && c <= '9') {
			return 1;
		}
		return 0;
	}

	bool isAlpha(char c) {
		return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_');
	}

	char advance() {
		current++;
		return current[-1];
	}

	bool match(char expected) {
		if (isAtEnd()) return false;
		if (*current != expected) return false;
		current++;
		return true;
	}

	void consumeWhitespace() {
		if (*current == ' ' || *current == '\r' || *current == '\t') {
			while (*current == ' ' || *current == '\r' || *current == '\t')
				advance();
		}
		start = current;
	}

	void consumeEmptyLine() {
		if (*current =='\n') {
			while (*current == '\n') {
				advance();
				consumeWhitespace();
			}
		}
		start = current;
	}

	Token scanToken() {
		start = current;
		// Handle for whitespaces 
		if (isAtEnd()) return makeToken(TOKEN_EOF);
		consumeWhitespace();
		if (*current == '\n') {
			line++;
			advance();
			consumeWhitespace();
			consumeEmptyLine();
		}

		// Handle for comments
		if (*current == '/' && *current + 1 == '/') {
			while (*current != '\n' && !isAtEnd()) advance();
		}

		char c = advance();
		if (isDigit(c)) return number();
		if (isAlpha(c)) return identifier();
		switch (c) {
		case '(': return makeToken(TOKEN_LEFT_PAREN);
		case ')': return makeToken(TOKEN_RIGHT_PAREN);
		case '{': return makeToken(TOKEN_LEFT_BRACE);
		case '}': return makeToken(TOKEN_RIGHT_BRACE);
		case ';': return makeToken(TOKEN_SEMICOLON);
		case ',': return makeToken(TOKEN_COMMA);
		case '.': return makeToken(TOKEN_DOT);
		case '-': return makeToken(TOKEN_MINUS);
		case '+': return makeToken(TOKEN_PLUS);
		case '/': return makeToken(TOKEN_SLASH);
		case '*': return makeToken(TOKEN_STAR);

		case '!':
			return makeToken(
				match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
		case '=':
			return makeToken(
				match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
		case '<':
			return makeToken(
				match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
		case '>':
			return makeToken(
				match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);

		case '"':
			return String();

		}
		return errorToken("Unexpected character.");
	}

	Token makeToken(TokenType type) {
		Token token = Token(type, start, (int)(current - start), line);
		//std::cout << token.type << "\n";
		return token;
	}

	Token errorToken(const char* message) {
		Token token = Token(TOKEN_ERROR, start, (int)(current - start), line);
		return token;
	}

	Token String() {
		while (*current != '"' && !isAtEnd()) {
			if (*current == '\n') line++;
			advance();
		}
		if (isAtEnd()) {
			return errorToken("Unterminated String");
		}
		advance();
		return makeToken(TOKEN_STRING);
	}

	Token number() {
		while (isDigit(*current) && !isAtEnd()) {
			advance();
		}
		if (*current == '.') {
			advance();
			while (isDigit(*current)) {
				advance();
			}
		}
		return makeToken(TOKEN_NUMBER);
	}

	Token identifier() {
		while ((isAlpha(*current) || isDigit(*current)) && !isAtEnd()) {
			advance();
		}
		return makeToken(TokenTypeIdentifier());
	}

	TokenType TokenTypeIdentifier() {

		switch (*start) {
		case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
		case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
		case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
		case 'f': {
			if (current - start > 1) {
				switch (start[1]) {
				case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
				case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
				case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
				}

			}
			break;
		}
		case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
		case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
		case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
		case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
		case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
		case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
		case 't': {
			if (current - start > 1) {
				switch (start[1]) {
				case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
				case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
				}
			}
			break;
		}
		case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
		case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);

		}
		return TOKEN_IDENTIFIER;
	}

	TokenType checkKeyword(int startC, int length, const char* check_str, TokenType checkToken) {
		if (current - start == startC + length &&
			memcmp(start + startC, check_str, length) == 0) {
			return checkToken;
		}

		return TOKEN_IDENTIFIER;
	}
};

class Parser {
public:
	Token current;
	Token previous;
	Scanner* scanner;
	bool had_error = 0;
	const char* source;

	Parser(const char* src, Scanner* scanner) : source(src), current(TOKEN_NONE, 0, 0, 0), previous(TOKEN_NONE, 0, 0, 0) {
		this->scanner = scanner;
	}

	void advance() {
		previous = current;
		current = scanner->scanToken();
		//std::cout << previous.type <<" " << current.type<<"\n";
	}

	void consume(TokenType type, const char* message) {

		if (current.type == type) {
			advance();
			return;
		}
		else {
			//std::cout << "Current token type is " << current.type << " Expected type is " << type << "\n";
			errorAtCurrent(message);
		}
	}

	void errorAt(Token* token, const char* message) {
		std::cerr << "Error at " << token->line << "\n";
		std::cerr << message << "\n";
		had_error = 1;
	}

	void errorAtCurrent(const char* message) {
		errorAt(&current, message);
	}

	void error(const char* message) {
		errorAt(&previous, message);
	}

};