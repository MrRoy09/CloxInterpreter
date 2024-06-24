#include "debug.h"
#include "chunk.h"
#include <iostream>

static int disassembleInstruction(Chunk* chunk, int opcode, int offset) {
	if (offset == 0) {
		std::cout << "Disassembling Chunk id " << chunk->id<<"\n";
	}
	switch (opcode)
	{
	case OP_RETURN:
		std::cout <<" At line = " << chunk->lines[offset]<<" At Offset " << offset << " Instruction " << "RETURN" << "\n";
		return offset + 1;
		break;

	case OP_RETURN_VALUE:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "RETURN VALUE" << "\n";
		return offset + 1;
		break;

	case OP_CONSTANT:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "LOAD_CONSTANT" << " = "; 
		chunk->constants[chunk->opcodes[offset + 1]].printValue();
		return offset + 2;
		break;
	case OP_NEGATE:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_NEGATE" <<"\n";
		return offset + 1;
		break;
	case OP_ADD:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_ADD" << "\n";
		return offset + 1;
		break;
	case OP_SUB:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_SUB" << "\n";
		return offset + 1;
		break;
	case OP_MUL:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_MUL" << "\n";
		return offset + 1;
		break;
	case OP_DIV:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_DIV" << "\n";
		return offset + 1;
		break;
	case OP_NIL:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_NIL" << "\n";
		return offset + 1;
		break;
	case OP_FALSE:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_FALSE" << "\n";
		return offset + 1;
		break;
	case OP_TRUE:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_TRUE" << "\n";
		return offset + 1;
		break;
	case OP_NOT:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_NOT" << "\n";
		return offset + 1;
		break;
	case OP_EQUAL:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_EQUAL" << "\n";
		return offset + 1;
		break;
	case OP_GREATER:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_GREATER" << "\n";
		return offset + 1;
		break;
	case OP_LESS:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_LESS" << "\n";
		return offset + 1;
		break;
	case OP_PRINT:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_PRINT" << "\n";
		return offset + 1;
		break;
	case OP_POP:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_POP" << "\n";
		return offset + 1;
		break;
	
	case OP_DEFINE_GLOBAL:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_DEFINE_GLOBAL" << "\n";
		return offset + 2;
		break;
	case OP_GET_GLOBAL:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_GET_GLOBAL" << "\n";
		return offset + 2;
		break;
	case OP_SET_GLOBAL:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_SET_GLOBAL" << "\n";
		return offset + 2;
		break;
	case OP_SET_LOCAL:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_SET_LOCAL" << "\n";
		return offset + 2;
		break;
	case OP_GET_LOCAL:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_GET_LOCAL" << "\n";
		return offset + 2;
		break;

	case OP_JUMP:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_JUMP " << "TO OFFSET "<<(uint16_t)((chunk->opcodes[offset +1] << 8) | chunk->opcodes[offset + 2])+offset+3<<"\n";
		return offset + 3;
		break;
	case OP_JUMP_IF_FALSE:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_JUMP_IF_FALSE " << "TO OFFSET "<< (uint16_t)((chunk->opcodes[offset +1] << 8) | chunk->opcodes[offset + 2]) +offset+3<< "\n";
		return offset + 3;
		break;
	case OP_LOOP:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_LOOP " << "TO OFFSET " << -(uint16_t)((chunk->opcodes[offset + 1] << 8) | chunk->opcodes[offset + 2]) + offset + 3 << "\n";
		return offset + 3;
		break;
	case OP_CALL:
		std::cout << " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "OP_CALL " << "\n";
		return offset + 2;
		break;

	default:
		std::cout<< " At line = " << chunk->lines[offset] << " At Offset " << offset << " Instruction " << "UNKNOWN"<< "\n";
		return offset + 1;
		break;
	}
}


void disassembleChunk(Chunk* chunk)
{
	int offset = 0;
	while(offset<chunk->opcodes.size()){
		offset= disassembleInstruction(chunk, chunk->opcodes[offset], offset);
	}

}




