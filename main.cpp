#include "Windows.h"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>

void SetCurrDir(std::string path = "")
{
	if (path != "") { SetCurrentDirectory(path.c_str()); return; }
	char currentDir[MAX_PATH]; // dynamic set curr dir to exe
	GetModuleFileName(NULL, currentDir, MAX_PATH);
	std::string::size_type pos = std::string(currentDir).find_last_of("\\/");
	SetCurrentDirectory(std::string(currentDir).substr(0, pos).c_str());
}

std::string ToHexString(uint32_t value, bool zx = true)
{
	std::stringstream ss;
	ss << (zx ? "0x" : "") << std::hex << std::uppercase << value;
	return ss.str();
}


void FileClearMake(std::string filePath) // or make empty file
{
	//if (is_valid_utf8(filePath)) { filePath = utf8_to_cp1251(filePath); }
	std::ofstream file(filePath, std::ios::out | std::ios::trunc);
	if (file.is_open()) { file.close(); }
}

bool FileExists(std::string filePath)
{
	//if (is_valid_utf8(filePath)) { filePath = utf8_to_cp1251(filePath); }
	std::ifstream file(filePath);
	return file.good();
}

void FileWriteAllLines(std::string filePath, std::vector<std::string> lines)
{
	//if (is_valid_utf8(filePath)) { filePath = utf8_to_cp1251(filePath); }
	if (!FileExists(filePath)) { FileClearMake(filePath); }
	std::ofstream file(filePath);
	if (!file.is_open()) { return; }
	for (const std::string& line : lines) { file << line << '\n'; }

	file.close();
}



//------------------------------------gta
// 002D25F0 [83] player
// 00391150 [951]
#define OFFSET (0x4BEAF0 - 0x3BFAF0) // CE!
uint32_t RelativeIda(uint32_t p, bool s = true) { return s ? p - OFFSET : p + OFFSET; }

std::string MkIdaFuncDefScript(uint32_t p, std::string name)
{
	std::string sphex = ToHexString(p);
	std::string str = "";
	//str += "del_items(" + sphex + ", DELIT_SIMPLE, 0x466DE0TODOENDPTR - " + sphex + ");"; // не обязательно
	str += "add_func(" + sphex + ", BADADDR);";
	str += "set_name(" + sphex +", \"" + name + "\", SN_AUTO);";
	return str;
}

struct MangMGR
{
	uint32_t pointer = 0x0;
	//std::vector<std::string> sufxindi;
	std::string buf = "";
	std::string symbol = ""; // out
};
void Do()
{
	std::string path = "SLUS_215.90";
	uint32_t ida_arr_ptr = 0x4BEAF0; // not file pos!
	size_t sz = 1381;
	uint32_t fpos = RelativeIda(ida_arr_ptr);
	std::cout << "0x" << (void*)fpos << "\n";

	//dbg prt
	if (ida_arr_ptr + (sz * 8) == RelativeIda(0x4C1618)) { std::cout << "!IF DBG PRT!!!!!!!!!" << "\n"; return; } // assert
	else {std::cout << "assert passed!" << "\n";}

	//std::fstream file(path, std::ios::in | std::ios::binary);
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) { std::cout << "Не удалось открыть файл!" << "\n"; return; }

	// go to arr
	//file.seekg(5, std::ios::beg);
	file.seekg(fpos, std::ios::cur);

	// read pointers
	std::vector<MangMGR> vals;
	for (size_t i = 0; i < sz; i++)
	{
		uint32_t p = 0x0;
		// skip hi prt :/
		file.seekg(4, std::ios::cur); // 0x0000FFFF
		file.read((char*)&p, 4);
		std::cout << "i: " << i << " " << ToHexString(p) << "\n";

		// try found ptr in table
		int index = -1;
		for (size_t j = 0; j < vals.size(); j++) { if (vals[j].pointer == p) { index = j; break; } }

		//_ZN14CRunningScript18DoDeatharrestCheckEv
		if (index >= 0) // found
		{
			MangMGR& mgr = vals[index];
			mgr.buf += std::to_string(i) + "_";
		}
		else // new
		{
			MangMGR mgr = { 0 };
			mgr.pointer = p;
			mgr.buf = "ProcessCommand_" + std::to_string(i) + "_";
			vals.push_back(mgr);
		}
	}


	for (size_t i = 0; i < vals.size(); i++) // concat final symbol
	{
		vals[i].symbol = "_ZN14CRunningScript" + std::to_string(vals[i].buf.size()) + vals[i].buf + "Ev";
	}

	// dump
	std::vector<std::string> scrvec;
	for (size_t i = 0; i < vals.size(); i++)
	{
		scrvec.push_back(MkIdaFuncDefScript(vals[i].pointer, vals[i].symbol)); // define function at pointer
	}
	FileWriteAllLines("IDAFIXSCRIPT.TXT", scrvec);

	//if (!file) { std::cerr << "Ошибка чтения данных!" << "\n"; return 1; }
	file.close();  // Закрываем файл
}



//-----entry
int main()
{
	SetCurrDir();
	Do();
	return 0;
}