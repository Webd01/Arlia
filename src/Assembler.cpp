#include "Assembler.hpp"
#include "AssemblerMacros.hpp"

bool Assembler::Include::IsUFI(std::string StrToParse) {
	// use [FuncName], from [DllName], in [FileName]
	std::vector<std::string> tokens = System::Vector::MultiSplit(StrToParse, " ,", false);
	for (std::string token : tokens) {
		if
			(!(
				token == "use" ||
				token == "from" ||
				token == "in" ||
				(token[0] == '\'' && token.back() == '\'')
				)) return false;
	}
	return true;
}
void Assembler::Include::AddUFI(std::string ImportName, std::string LibName, std::string IncName) {
	if (!System::Vector::Contains(this->Libraries, LibName)) this->Libraries.push_back(LibName);
	if (!System::Vector::Contains(this->Includes, IncName)) this->Includes.push_back(IncName);
	this->Imports[LibName].push_back(ImportName);
}
std::string Assembler::Include::use(std::string name, std::string from, std::string in) {
	return "use '" + name + "', from '" + from + "', in '" + in + "'" + "\n";
}

std::string Assembler::AsmFinalCode::outset() {
	if (this->IsDLL) {
		if (System::cpp::GetOS() == System::cpp::OS::Windows) return "format PE DLL\n";
		if (System::cpp::GetOS() == System::cpp::OS::Linux) return "format ELF DLL\n";
	}
	else {
		if (System::cpp::GetOS() == System::cpp::OS::Windows) return "format PE\n";
		if (System::cpp::GetOS() == System::cpp::OS::Linux) return "format ELF\n";
	}
}
void Assembler::AsmFinalCode::AppendGlobalVariable(int bytes, std::string value) {
	std::string byte;
BitLoop:
	switch (bytes) {
	case 0: byte = "db"; break;
	case 1: byte = "db"; break;
	case 2: byte = "dw"; break;
	case 4: byte = "dd"; break;
	case 6: byte = "dq"; break;
	case 8: byte = "dq"; break;
	case 10: byte = "dt"; break;
	default: bytes = (bytes * bytes) % 4; goto BitLoop; break;
	}
	if (System::Text::IsString(value)) {
		this->GlobalVariables.push_back({ "GV" + std::to_string(this->GlobalVariableUniqueNameIndex), byte, value + ", 0" });
	}
	else {
		this->GlobalVariables.push_back({ "GV" + std::to_string(this->GlobalVariableUniqueNameIndex), byte, value });
	}
	++this->GlobalVariableUniqueNameIndex;
}
std::string Assembler::AsmFinalCode::OperatorIdentifier(int bytes) {
	switch (bytes * 2) {
	case 2: return "BYTE";
	case 8: return "DWORD";
	case 16: return "WORD";
	case 32: return "DWORD";
	case 48: return "FWORD";
	case 64: return "PWORD";
	case 80: return "QWORD";
	case 128: return "TWORD";
	case 256: return "QQWORD";
	case 512: return "DQQWORD";
	default: return "";
	}
}
void Assembler::AsmFinalCode::append(std::string ToAppend) {
	if (this->IsUFI(ToAppend)) {
		ToAppend = System::Text::replace(ToAppend, " ", "");
		std::vector<std::string> tokens = System::Vector::split(ToAppend, ",");
		std::string FUN = tokens[0].substr(tokens[0].find_first_of('\''), tokens[0].find_last_of('\'') - tokens[0].find('\'')).substr(1);
		std::string DLL = tokens[1].substr(tokens[1].find_first_of('\''), tokens[1].find_last_of('\'') - tokens[1].find('\'')).substr(1);
		std::string LIB = tokens[2].substr(tokens[2].find_first_of('\''), tokens[2].find_last_of('\'') - tokens[2].find('\'')).substr(1);
		//std::cout << FUN << " " << DLL << " " << LIB << " " << std::endl;
		 this->AddUFI(FUN, DLL, LIB);
		return;
	}
	this->RawCode += ToAppend + "\n"; // Append new line
}
void Assembler::AsmFinalCode::PrepareCode() {
	std::string tmp;
	// Firstly, it is necessary to remove duplicates in order to avoid errors:
	for (std::map<std::string, std::vector<std::string>>::iterator it = this->Imports.begin(); it != this->Imports.end(); ++it)
		System::Vector::RemoveDuplicates<std::string>(it->second);
	// Secondly, we import files
	if (!this->Libraries.empty() || !this->Imports.empty()) tmp += this->section_IDATA + "\n";
	for (std::string inc : this->Includes) tmp += "include '" + System::Text::replace(inc, "\n", "") + "'" + "\n";
	if (!this->Libraries.empty()) {
		tmp += "library ";
		tmp += this->Libraries[0] + ", " + "'" + this->Libraries[0] + ".dll" + "'";
		if (this->Libraries.size() > 1) tmp += ", \\";
		for (size_t i = 1; i < this->Libraries.size(); ++i)
			if (i == this->Libraries.size() - 1) tmp += "\n\t\t" + this->Libraries[i] + ", " + "'" + this->Libraries[i] + ".dll" + "'";
			else tmp += "\n\t\t" + this->Libraries[i] + ", " + "'" + this->Libraries[i] + ".dll" + "'" + ", \\";
		tmp += "\n";
	}
	for (std::map<std::string, std::vector<std::string>>::iterator it = this->Imports.begin(); it != this->Imports.end(); ++it) {
		tmp += "import  " + it->first;
		for (std::string name : it->second) tmp += ", " + name + ", " + "'" + name + "'";
		tmp += "\n";
	}
	// Thirdly , we append global variables
	if (this->GlobalVariableUniqueNameIndex > 0) {
		tmp += "\n" + this->section_DATA;
		for (asm_GlobalVariable GV : this->GlobalVariables) tmp += "\t" + GV.UniqueName + " " + GV.Bytes + " " + GV.Value + "\n";
	}
	// Fourthly, we add rest of code
	tmp += "\n" + this->section_CODE + "\n";
	tmp += this->RawCode;
	this->RawCode.clear();
	this->RawCode += "; Code generated by the Arlia compiler (c)\n\n" + outset() + "\n";
	Assembler::Macro::AppendMacros(*this);
	this->RawCode += tmp;
}
std::string Assembler::AsmFinalCode::GetAsm() {
	PrepareCode();
	return this->RawCode;
}

void Assembler::Register::reset(size_t size) {
	if ((size * 8) % 8 == 0) this->counter_8 = 0;
	if ((size * 8) % 16 == 0) this->counter_16 = 0;
	if ((size * 8) % 32 == 0) this->counter_32 = 0;
	if ((size * 8) % 64 == 0) this->counter_64 = 0;
	if ((size * 8) % 80 == 0) this->counter_80 = 0;
	if ((size * 8) % 128 == 0) this->counter_128 = 0;
	if ((size * 8) % 256 == 0) this->counter_256 = 0;
	if ((size * 8) % 512 == 0) this->counter_512 = 0;
}
void Assembler::Register::ReverseOrder(int size) {
	if (size == -1) for (int i = 1; i < 8; ++i) ReverseOrder(i * 8);
	if ((size * 8) % 8 == 0) std::reverse(this->bits_8.begin(), this->bits_8.end());
	if ((size * 8) % 16 == 0) std::reverse(this->bits_16.begin(), this->bits_16.end());
	if ((size * 8) % 32 == 0) std::reverse(this->bits_32.begin(), this->bits_32.end());
	if ((size * 8) % 64 == 0) std::reverse(this->bits_64.begin(), this->bits_64.end());
	if ((size * 8) % 80 == 0) std::reverse(this->bits_80.begin(), this->bits_80.end());
	if ((size * 8) % 128 == 0) std::reverse(this->bits_128.begin(), this->bits_128.end());
	if ((size * 8) % 256 == 0) std::reverse(this->bits_256.begin(), this->bits_256.end());
	if ((size * 8) % 512 == 0) std::reverse(this->bits_512.begin(), this->bits_512.end());
}
void Assembler::Register::ResetStack() {
	this->StackSize = 0;
}
std::string Assembler::Register::next(size_t size, std::vector<std::string> separate) {
	std::string ret;

	if ((size * 8) % 8 == 0) {
		if (counter_8 == max) reset(size);
		ret = bits_8[counter_8];
		++counter_8;
	}
	if ((size * 8) % 16 == 0) {
		if (counter_16 == max) reset(size);
		ret = bits_16[counter_16];
		++counter_16;
	}
	if ((size * 8) % 32 == 0) {
		if (counter_32 == max) reset(size);
		ret = bits_32[counter_32];
		++counter_32;
	}
	if ((size * 8) % 64 == 0) {
		if (counter_64 == max) reset(size);
		ret = bits_64[counter_64];
		++counter_64;
	}
	if ((size * 8) % 80 == 0) {
		if (counter_80 == max) reset(size);
		ret = bits_80[counter_80];
		++counter_80;
	}
	if ((size * 8) % 128 == 0) {
		if (counter_128 == max) reset(size);
		ret = bits_128[counter_128];
		++counter_128;
	}
	if ((size * 8) % 256 == 0) {
		if (counter_256 == max) reset(size);
		ret = bits_256[counter_256];
		++counter_256;
	}
	if ((size * 8) % 512 == 0) {
		if (counter_512 == max) reset(size);
		ret = bits_512[counter_512];
		++counter_512;
	}

	// Prevents the use of stack registers in program manipulation
	if (System::Vector::Contains<std::string>(separate, ret)) return next(size, separate);

	return ret;
}
std::string	Assembler::Register::PushInStack(size_t size, bool subtract) {
	StackSize += (size * 2);
	StackSizes.push(StackSize / 2);
	if (subtract) return "[" + Stack_reg + "-" + std::to_string(StackSizes.top()) + "]";
	return "[" + Stack_reg + "+" + std::to_string(StackSizes.top()) + "]";
}
std::string Assembler::Register::MovInStack(size_t size, std::string value, bool substract) {
	return "\tmov " + AsmFinalCode::OperatorIdentifier(size) + " " + this->PushInStack(size, substract) + ", " + value + "\n";
}
