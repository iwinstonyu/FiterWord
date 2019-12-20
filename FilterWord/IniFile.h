//
//	<one line to give the program's name and a brief idea of what it does.>
//	Copyright (C) 2018. WenJin Yu. windpenguin@gmail.com.
//
//	Created at 2018/7/4 11:40:04
//	Version 1.0
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <string>
#include <map>
#include <vector>
#include <set>
#include <sstream>
#include <functional>
using namespace std;

namespace wind {

struct ErrorHelper
{
	typedef function<void(ostringstream& oss)> FuncWriteErr;

	ErrorHelper(FuncWriteErr func) : func_(func) {}
	~ErrorHelper() {
		func_(oss_);
	}

	ostringstream& Stream() { return oss_; }

	ostringstream oss_;
	FuncWriteErr func_;
};
#define INI_WRITE_ERROR  ErrorHelper(bind(&IniFile::WriteErr, this, std::placeholders::_1)).Stream()

struct Profile
{
	Profile() {}

	string key_;
	string val_;
};

struct Section
{
	Section() {}
	~Section() {}
	void Reset() {
		name_ = "";
		profiles_.clear();
		profileIdx_.clear();
	}

	int ReadInt(string key, int defaultVal);
	void ReadIntVec(string key, vector<int>& vals);
	void ReadIntSet(string key, set<int>& vals);
	string ReadString(string key, string defaultVal);
	float ReadFloat(string key, float defaultVal);
	void ReadFloatVec(string key, vector<float>& vals);
	bool ReadBool(string key, bool defaultVal);
	void ReadBoolVec(string key, vector<bool>& vals);

	string name_;
	vector<Profile> profiles_;
	map<string, int> profileIdx_;
};

class IniFile
{
public:
	explicit IniFile(string fileName);
	~IniFile();
	
	bool IsOpen() { return isOpen_; }
	vector<string>& Errs() { return errs_; }

	/// @brief 遍历所有的section
	void StartIterateSection();
	bool NextSection();
	string ReadSectionName();
	int ReadInt(string key, int defaultVal);
	void ReadIntVec(string key, vector<int>& vals);
	void ReadIntSet(string key, set<int>& vals);
	string ReadString(string key, string defaultVal);
	float ReadFloat(string key, float defaultVal);
	void ReadFloatVec(string key, vector<float>& vals);
	bool ReadBool(string key, bool defaultVal);
	void ReadBoolVec(string key, vector<bool>& vals);

	/// @brief 通用查询接口
	int ReadInt(string section, string key, int defaultVal);
	void ReadIntVec(string section, string key, vector<int>& vals);
	string ReadString(string section, string key, string defaultVal);
	float ReadFloat(string section, string key, float defaultVal);
	void ReadFloatVec(string section, string key, vector<float>& vals);
	bool ReadBool(string section, string key, bool defaultVal);

private:
	Section* FindSection(string name);

	void LoadFile();
	void TrimLeftRight(string& str);
	void WriteErr(ostringstream& oss);
	void AddSection(Section& section);

private:
	vector<Section> sections_;
	map<string, size_t> sectionIdx_;
	size_t iterateIdx_;

	string fileName_;
	bool isOpen_;
	vector<string> errs_;
};

}

