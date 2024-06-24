#include <iostream>
#include <string>
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"
#include "compiler.h"
#include <fstream>
#include <sstream>

static void repl(VM *vm)
{
    while (1)
    {
        std::string input;
        std::cout << "> ";
        std::getline(std::cin, input);
        if (input == "exit")
        {
            break;
        }
        InterpretResult result = vm->interpret(input);
        if (result != INTERPRET_OK)
        {
            exit(10);
        }
    }
}

int main(int argc, const char *argv[])
{
    VM vm;
    if (argc == 2)
    {
        std::ifstream code(argv[1]);
        std::stringstream buffer;
        buffer << code.rdbuf();
        std::string code_string = buffer.str();
        vm.interpret(code_string);
    }
    else
    {
        std::cout << "Missing argument filepath" << "\n";
    }
    return 0;
}
