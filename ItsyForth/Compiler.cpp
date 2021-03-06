#include "Compiler.hpp"
#include "DictionaryWord.hpp"
#include "CountedString.hpp"

static const std::string INDENT = "  ";

Compiler::Compiler(Runtime* runtimeIn)
: runtime(runtimeIn)
{
	reset();
}

void Compiler::reset() {
	dictionaryPtrAddr = 0;							// bootstrap dictionary pointer at address 0
	runtime->setCell(dictionaryPtrAddr, 0);			// we know dp will be at 0 since it is the first thing allocated
	dictionaryPtrAddr = allocate(sizeof(Cell));		// allocate the memory for the dp
	
	lastWordPtrAddr = allocate(sizeof(Cell));
	runtime->setCell(lastWordPtrAddr, 0);
}

int Compiler::allocate(int size) {
	int dp = runtime->getCell(dictionaryPtrAddr).asData;
	runtime->setCell(dictionaryPtrAddr, dp + size);
	return dp;
}
	

int Compiler::compileWord(const std::string& name) {
	int result = createWord(name);
	dbg(result, ": " + name);
	return result;
}

int Compiler::compileWord(const std::string& name, const OpCode& opcode, int value) {
	int result = createWord(name, opcode, value);
	dbg(result, ": " + name + " " + Instruction(opcode, value).toString());
	return result;
}

int Compiler::compileWord(const std::string& name, const Instruction& refInstruction) {
	int result = createWord(name, refInstruction);
	dbg(result, ": " + name + " " + refInstruction.toString());
	return result;
}

int Compiler::compileInstruction(const Instruction& ins) {
	int result = allocate(sizeof(Cell));
	(*runtime)[result].asInstruction = ins;
	dbg(result, ins.toString());
	return result;
}

int Compiler::compileLiteral(int value) {
	return compileInstruction(Instruction(OpCode::DoLit, value));
}

int Compiler::compileVariable(const std::string& name, int initialValue) {
	int addr = createWord(name);
	DictionaryWord* word = (DictionaryWord*)runtime->asPtr(addr);
	word->referenceInstruction = Instruction(OpCode::DoVariable, compileData(initialValue));
	dbg(addr, word->referenceInstruction.toString());
	return addr;
}

int Compiler::compileConstant(const std::string& name, int value) {
	int result = createWord(name, OpCode::DoConstant, value);
	dbg(result, Instruction(OpCode::DoConstant, value).toString());
}

int Compiler::compileStartColonWord(const std::string& name) {
	int addr = createWord(name);
	DictionaryWord* word = (DictionaryWord*)runtime->asPtr(addr);
	word->referenceInstruction = Instruction(OpCode::DoColon, addr + sizeof(DictionaryWord));
	dbg(addr, word->referenceInstruction.toString());
	return addr;
}

int Compiler::compileData(int addr) {
	int result = allocate(sizeof(Cell));
	(*runtime)[result].asData = addr;
	dbg(result, std::to_string(addr));
	return result;
}

int Compiler::compileReference(const std::string& name) {
	int result = compileInstruction(Instruction(OpCode::DoColon, findWord(name)));
	dbg(result, Instruction(OpCode::DoColon, findWord(name)).toString() + " " + name);
	return result;
}

int Compiler::compileEndWord() {
	int result = compileInstruction(OpCode::DoSemicolon);
	dbg(result, OpCode::DoSemicolon);
	return result;
}

int Compiler::compileBegin() {
	pushMark(getDictionaryPtr());
	return getDictionaryPtr();
}

int Compiler::compileIf() {
	pushMark(getDictionaryPtr());
	return compileInstruction(OpCode::ZeroBranch);
}

int Compiler::compileElse() {
	int ifMark = popMark();					// if instruction to patch now
	pushMark(getDictionaryPtr());			// else branch to patch later
	int result = compileInstruction(OpCode::Branch);
	(*runtime)[ifMark].asInstruction.data, getDictionaryPtr();
	dbg(ifMark, (*runtime)[ifMark].asInstruction.toString());
	return result;
}

int Compiler::compileEndif() {
	int result = popMark();
	(*runtime)[result].asInstruction.data = getDictionaryPtr();
	dbg(result, (*runtime)[result].asInstruction.toString());
	return result;
}

int Compiler::compileAgain() {
	return compileInstruction(Instruction(OpCode::Branch, popMark()));
}

int Compiler::findWord(const std::string &name) {
	int wordAddr = runtime->getCell(lastWordPtrAddr);
	while (wordAddr) {
		DictionaryWord* lastWord = (DictionaryWord*)runtime->asPtr(wordAddr);
		if (name == CountedString::toCString(lastWord->name)) {
			return wordAddr;
		} else {
			wordAddr = lastWord->previous;
		}
	}
	return -1;
}

int Compiler::createWord(const std::string& name) {
	return createWord(name, Instruction(OpCode::INVALID, 0));
}

int Compiler::createWord(const std::string& name, const OpCode& opcode, int value) {
	return createWord(name, Instruction(opcode, value));
}

int Compiler::createWord(const std::string& name, const Instruction& refInstruction) {
	int result = allocate(sizeof(DictionaryWord));
	DictionaryWord* word = (DictionaryWord*)runtime->asPtr(result);
	word->previous = runtime->getCell(lastWordPtrAddr);
	CountedString::fromCString(name, word->name, sizeof(word->name));
	word->referenceInstruction = refInstruction;
	(*runtime)[lastWordPtrAddr].asData = result;	//add to dictionary
	return result;
}
