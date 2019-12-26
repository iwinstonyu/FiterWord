//
//	<one line to give the program's name and a brief idea of what it does.>
//	Copyright (C) 2018. WenJin Yu. windpenguin@gmail.com.
//
//	Created at 2018/7/4 11:40:47
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

#include "IniFile.h"
#include <fstream>
#include <limits.h>
#include <iterator>

namespace wind {

int Section::ReadInt(string key, int defaultVal)
{
	string strVal = ReadString(key, "");
	if (strVal.empty())
		return defaultVal;
	return atoi(strVal.c_str());
}

void Section::ReadIntVec(string key, vector<int>& vals)
{
	string strVal = ReadString(key, "");
	if (strVal.empty())
		return;

	string::size_type begPos = 0;
	while(begPos < strVal.length()){
		string::size_type endPos = strVal.find(",", begPos);
		if (endPos == string::npos)
			endPos = strVal.length();
		string strPart = strVal.substr(begPos, endPos - begPos);
		vals.push_back(atoi(strPart.c_str()));
		begPos = endPos+1;
	}
}

void Section::ReadIntSet(string key, set<int>& vals)
{
	vector<int> valsVec;
	ReadIntVec(key, valsVec);
	copy(valsVec.begin(), valsVec.end(), inserter(vals, vals.end()));
}

string Section::ReadString(string key, string defaultVal)
{
	auto it = profileIdx_.find(key);
	if (it == profileIdx_.end())
		return defaultVal;
	return profiles_[it->second].val_;
}

float Section::ReadFloat(string key, float defaultVal)
{
	string strVal = ReadString(key, "");
	if (strVal.empty())
		return defaultVal;
	return static_cast<float>(atof(strVal.c_str()));
}

void Section::ReadFloatVec(string key, vector<float>& vals)
{
	string strVal = ReadString(key, "");
	if (strVal.empty())
		return;

	string::size_type begPos = 0;
	while (begPos < strVal.length()) {
		string::size_type endPos = strVal.find(",", begPos);
		if (endPos == string::npos)
			endPos = strVal.length();
		string strPart = strVal.substr(begPos, endPos - begPos);
		vals.push_back(static_cast<float>(atof(strPart.c_str())));
		begPos = endPos + 1;
	}
}

bool Section::ReadBool(string key, bool defaultVal)
{
	string strVal = ReadString(key, "");
	if (strVal.empty())
		return defaultVal;
	return atoi(strVal.c_str()) > 0 ? true : false;
}

void Section::ReadBoolVec(string key, vector<bool>& vals)
{
	string strVal = ReadString(key, "");
	if (strVal.empty())
		return;

	string::size_type begPos = 0;
	while (begPos < strVal.length()) {
		string::size_type endPos = strVal.find(",", begPos);
		if (endPos == string::npos)
			endPos = strVal.length();
		string strPart = strVal.substr(begPos, endPos - begPos);
		vals.push_back(atoi(strPart.c_str()) ? true : false);
		begPos = endPos + 1;
	}
}

IniFile::IniFile(string fileName)
	: fileName_(fileName)
	, isOpen_(false)
	, iterateIdx_(SIZE_MAX)
{
	LoadFile();
}

IniFile::~IniFile()
{

}

void IniFile::StartIterateSection() 
{
	iterateIdx_ = SIZE_MAX;
}

bool IniFile::NextSection() 
{
	if (sections_.empty())
		return false;

	if (iterateIdx_ == SIZE_MAX) {
		iterateIdx_ = 0;
		return true;
	}
	else {
		if (iterateIdx_ >= sections_.size())
			return false;

		return ++iterateIdx_ >= sections_.size() ? false : true;
	}
}

string IniFile::ReadSectionName()
{
	if (iterateIdx_ >= sections_.size())
		return "";

	return sections_[iterateIdx_].name_;
}

int IniFile::ReadInt(string key, int defaultVal)
{
	if (iterateIdx_ >= sections_.size())
		return defaultVal;

	return sections_[iterateIdx_].ReadInt(key, defaultVal);
}

void IniFile::ReadIntVec(string key, vector<int>& vals)
{
	if (iterateIdx_ >= sections_.size())
		return;

	sections_[iterateIdx_].ReadIntVec(key, vals);
}

void IniFile::ReadIntSet(string key, set<int>& vals)
{
	if (iterateIdx_ >= sections_.size())
		return;

	sections_[iterateIdx_].ReadIntSet(key, vals);
}

string IniFile::ReadString(string key, string defaultVal)
{
	if (iterateIdx_ >= sections_.size())
		return defaultVal;

	return sections_[iterateIdx_].ReadString(key, defaultVal);
}

float IniFile::ReadFloat(string key, float defaultVal)
{
	if (iterateIdx_ >= sections_.size())
		return defaultVal;

	return sections_[iterateIdx_].ReadFloat(key, defaultVal);
}

void IniFile::ReadFloatVec(string key, vector<float>& vals)
{
	if (iterateIdx_ >= sections_.size())
		return;

	sections_[iterateIdx_].ReadFloatVec(key, vals);
}

bool IniFile::ReadBool(string key, bool defaultVal)
{
	if (iterateIdx_ >= sections_.size())
		return defaultVal;

	return sections_[iterateIdx_].ReadBool(key, defaultVal);
}

void IniFile::ReadBoolVec(string key, vector<bool>& vals)
{
	if (iterateIdx_ >= sections_.size())
		return;

	sections_[iterateIdx_].ReadBoolVec(key, vals);
}

int IniFile::ReadInt(string section, string key, int defaultVal)
{
	auto pSection = FindSection(section);
	if (!pSection)
		return defaultVal;
	return pSection->ReadInt(key, defaultVal);
}

void IniFile::ReadIntVec(string section, string key, vector<int>& vals)
{
	auto pSection = FindSection(section);
	if (!pSection)
		return;
	return pSection->ReadIntVec(key, vals);
}

string IniFile::ReadString(string section, string key, string defaultVal)
{
	auto pSection = FindSection(section);
	if (!pSection)
		return defaultVal;
	return pSection->ReadString(key, defaultVal);
}

float IniFile::ReadFloat(string section, string key, float defaultVal)
{
	auto pSection = FindSection(section);
	if (!pSection)
		return defaultVal;
	return pSection->ReadFloat(key, defaultVal);
}

void IniFile::ReadFloatVec(string section, string key, vector<float>& vals)
{
	auto pSection = FindSection(section);
	if (!pSection)
		return;
	pSection->ReadFloatVec(key, vals);
}

bool IniFile::ReadBool(string section, string key, bool defaultVal)
{
	auto pSection = FindSection(section);
	if (!pSection)
		return defaultVal;
	return pSection->ReadBool(key, defaultVal);
}

Section* IniFile::FindSection(string name)
{
	auto it = sectionIdx_.find(name);
	if (it == sectionIdx_.end())
		return nullptr;
	return &(sections_[it->second]);
}

void IniFile::LoadFile()
{
	ifstream ifs;
	ifs.open(fileName_);
	if (!ifs.is_open()) {
		isOpen_ = false;
		INI_WRITE_ERROR << "Fail open file : " << fileName_;
		return;
	}

	isOpen_ = true;
	string line;
	Section section;
	int lineno = 0;
	while (getline(ifs, line)) {
		++lineno;
		TrimLeftRight(line);

		if (line.empty())
			continue;

		switch (line[0])
		{
		case '#':
		case ';':
		{
		}
		break;
		case '[':
		{
			string::size_type pos = line.find_last_of("]");
			if (pos == string::npos) {
				INI_WRITE_ERROR << "Find '[' but no ']': " << fileName_ << " " << lineno << " "<< line;
				break;
			}

			if (!section.name_.empty()) {
				AddSection(section);
				section.Reset();
			}

			section.name_ = line.substr(1, pos - 1);
		}
		break;
		default:
		{
			string::size_type pos = line.find_first_of("=");
			if (pos == string::npos) {
				INI_WRITE_ERROR << "What's this line: " << fileName_ << " " << lineno << " " << line;
				break;
			}

			if (section.name_.empty()) {
				INI_WRITE_ERROR << "Find key-val but no section: " << fileName_ << " " << lineno << " " << line;
				break;
			}

			Profile profile;
			profile.key_ = line.substr(0, pos);
			profile.val_ = line.substr(pos + 1);
			TrimLeftRight(profile.key_);
			TrimLeftRight(profile.val_);

			auto it = section.profileIdx_.find(profile.key_);
			if (it == section.profileIdx_.end()) {
				section.profiles_.push_back(profile);
				section.profileIdx_[profile.key_] = static_cast<int>(section.profiles_.size() - 1);
			}
			else {
				INI_WRITE_ERROR << "Duplicated key in section: " << fileName_ << " " << lineno << " "<< section.name_ << " " << profile.key_;
				break;
			}
		}
		break;
		}
	}

	if (!section.name_.empty()) {
		AddSection(section);
		section.Reset();
	}

	ifs.close();
}

void IniFile::TrimLeftRight(string& str)
{
	if (str.empty())
		return;

	str.erase(0, str.find_first_not_of(" "));
	str.erase(str.find_last_not_of(" ")+1);
}

void IniFile::WriteErr(ostringstream& oss)
{
	errs_.push_back(oss.str());
}

void IniFile::AddSection(Section& section)
{
	if (section.name_.empty()) {
		INI_WRITE_ERROR << "Add section but empty name";
		return;
	}

	auto it = sectionIdx_.find(section.name_);
	if (it == sectionIdx_.end()) {
		sections_.push_back(section);
		sectionIdx_[section.name_] = sections_.size() - 1;
	}
	else {
		INI_WRITE_ERROR << "Duplicated section name and ignore: " << fileName_ << " " << section.name_;
	}
}

}