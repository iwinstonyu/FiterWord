// FilterWord.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <set>
#include <ctime>
#include <thread>
#include <sstream>
#include "IniFile.h"

using namespace std;

typedef unsigned char uchar;
typedef unsigned int uint;

const char* MASK = "*************************************************";
const int MAX_WORD_BLOCK = 20;

int Utf8CharLegnth(unsigned char ch)
{
	if (ch >= 0 && ch <= 127)
		return 1;
	if ((ch & 0xE0) == 0xC0)
		return 2;
	if ((ch & 0xF0) == 0xE0)
		return 3;
	if ((ch & 0xF8) == 0xF0)
		return 4;
	if ((ch & 0xFC) == 0xF8)
		return 5;
	if ((ch & 0xFE) == 0xFC)
		return 6;
	if ((ch & 0xFF) == 0xFE)
		return 7;

	return 8;
}

struct SenWord
{
	struct Block
	{
		string block_;
		int times_ = 0;
		int senBlockHeadIdx_ = -1;
	};

	void AddBlock(string& block)
	{
		if (blockAmount_ && blocks_[blockAmount_- 1].block_ == block){
			blocks_[blockAmount_ - 1].times_++;
		}
		else{
			blocks_[blockAmount_].block_ = block;
			blocks_[blockAmount_].times_ = 1;
			++blockAmount_;
		}
	}

	int blockAmount_ = 0;
	Block blocks_[MAX_WORD_BLOCK];
};

struct TraceWord
{
	int senWordIdx_ = -1;
	int blockIdx_ = -1;
	size_t wordIdx_ = -1;
	vector<int> wordIdxRec_[MAX_WORD_BLOCK];
};

const int SEN_WORDS_AMOUNT = 10000;
SenWord senWords[SEN_WORDS_AMOUNT];
vector<vector<int>> senWordBuckets;
vector<string> senBlockHeads;

ofstream ofslog;

#define EASYLOG ofslog

int GetSenBlockIdx(string& str)
{
	auto it = find(senBlockHeads.begin(), senBlockHeads.end(), str);
	if (it != senBlockHeads.end())
		return distance(senBlockHeads.begin(), it);
	else
		return -1;
}

void InitSenWords()
{
	//vector<string> originWords;
	//originWords.push_back("a_b_c");
	//originWords.push_back("a_c_d");
	//originWords.push_back("a_c_e_");

	ifstream ifs("sensitive.txt");
	if (!ifs.is_open())
		return;

	size_t senWordIdx = 0;
	string word;
	set<string> blockHeads;
	while (getline(ifs, word)) {
		size_t beg = 0;
		for (size_t idx = 0; idx < word.length(); ++idx) {
			if(word[idx] == '_'){
				if(idx > beg){
					string block = word.substr(beg, idx - beg);
					blockHeads.insert(block.substr(0, Utf8CharLegnth(block[0])));
					senWords[senWordIdx].AddBlock(block);
				}
				beg = idx + 1;
			}
		}
		if(beg < word.length()){
			string block = word.substr(beg, word.length() - beg);
			blockHeads.insert(block.substr(0, Utf8CharLegnth(block[0])));
			senWords[senWordIdx].AddBlock(block);
		}

		++senWordIdx;
	}

	ifs.close();

	copy(blockHeads.begin(), blockHeads.end(), back_inserter(senBlockHeads));

	senWordBuckets.assign(senBlockHeads.size(), vector<int>());
	for (size_t i = 0; i < senWordIdx; ++i) {
		auto& senWord = senWords[i];

		for(int j = 0; j < senWord.blockAmount_; ++j){
			auto& block = senWord.blocks_[j];

			auto it = find(senBlockHeads.begin(), senBlockHeads.end(), block.block_.substr(0, Utf8CharLegnth(block.block_[0])));
			if (it != senBlockHeads.end()) {
				block.senBlockHeadIdx_ = distance(senBlockHeads.begin(), it);

				if (j == 0) {
					senWordBuckets[block.senBlockHeadIdx_].push_back(i);
				}
			}
		}
	}

	EASYLOG << "Load sensitive word amount: " << senWordIdx << endl;
	EASYLOG << "Load sensitive block amount: " << senBlockHeads.size() << endl;
}

void FilterWord(string& src, string& dest)
{
	dest = src;

	vector<TraceWord> traceWords;
	vector<vector<int>> traceWordBuckets;
	vector<vector<int>> traceNextWordBuckets;
	traceWordBuckets.assign(senBlockHeads.size(), vector<int>());
	traceNextWordBuckets.assign(senBlockHeads.size(), vector<int>());

	for (size_t i = 0; i < dest.length(); i += Utf8CharLegnth(dest[i])) {
		set<int> pickIdx;

		string subWord = string(dest.begin()+i, dest.begin() + i + Utf8CharLegnth(dest[i]));

		int senBlockIdx = GetSenBlockIdx(subWord);
		if(senBlockIdx >= 0){
			auto& traceBucket = traceWordBuckets[senBlockIdx];
			for (auto idx : traceBucket) {
				auto& traceWord = traceWords[idx];
				auto& senWord = senWords[traceWord.senWordIdx_];

				if (traceWord.wordIdx_ <= i
					&& dest.length() - i >= senWord.blocks_[traceWord.blockIdx_].block_.length()
					&& dest.substr(i, senWord.blocks_[traceWord.blockIdx_].block_.length()) == senWord.blocks_[traceWord.blockIdx_].block_) {
					traceWord.wordIdx_ = i + senWord.blocks_[traceWord.blockIdx_].block_.length();
					traceWord.wordIdxRec_[traceWord.blockIdx_].push_back(i);

					if (traceWord.blockIdx_ == 0)
						pickIdx.insert(traceWord.senWordIdx_);

					if (traceWord.blockIdx_ + 1 < senWord.blockAmount_
						&& traceWord.wordIdxRec_[traceWord.blockIdx_].size() == senWord.blocks_[traceWord.blockIdx_].times_){
						traceNextWordBuckets[senWord.blocks_[traceWord.blockIdx_+1].senBlockHeadIdx_].push_back(idx);
					}
				}
			}

			auto& traceNextBucket = traceNextWordBuckets[senBlockIdx];
			vector<int> eraseIdx;
			for (auto idx : traceNextBucket) {
				auto& traceWord = traceWords[idx];
				auto& senWord = senWords[traceWord.senWordIdx_];

				if (traceWord.wordIdx_ <= i
					&& traceWord.blockIdx_ + 1 < senWord.blockAmount_
					&& dest.length() - i >= senWord.blocks_[traceWord.blockIdx_ + 1].block_.length()
					&& dest.substr(i, senWord.blocks_[traceWord.blockIdx_ + 1].block_.length()) == senWord.blocks_[traceWord.blockIdx_ + 1].block_) {
					auto it = find(traceWordBuckets[senWord.blocks_[traceWord.blockIdx_].senBlockHeadIdx_].begin(),
						traceWordBuckets[senWord.blocks_[traceWord.blockIdx_].senBlockHeadIdx_].end(),
						idx);
					if (it != traceWordBuckets[senWord.blocks_[traceWord.blockIdx_].senBlockHeadIdx_].end())
						traceWordBuckets[senWord.blocks_[traceWord.blockIdx_].senBlockHeadIdx_].erase(it);

					traceBucket.push_back(idx);

					eraseIdx.push_back(idx);

					traceWord.blockIdx_ += 1;
					traceWord.wordIdx_ = i + senWord.blocks_[traceWord.blockIdx_].block_.length();
					traceWord.wordIdxRec_[traceWord.blockIdx_].push_back(i);
				}
			}

			for (auto idx : eraseIdx) {
				auto it = find(traceNextBucket.begin(),
					traceNextBucket.end(), idx);
				if (it != traceNextBucket.end())
					traceNextBucket.erase(it);
			}

			for (auto idx : eraseIdx) {
				auto& traceWord = traceWords[idx];
				auto& senWord = senWords[traceWord.senWordIdx_];

				if (traceWord.blockIdx_ + 1 < senWord.blockAmount_) {
					traceNextWordBuckets[senWord.blocks_[traceWord.blockIdx_ + 1].senBlockHeadIdx_].push_back(idx);
				}
			}

			auto& bucket = senWordBuckets[senBlockIdx];
			for (auto idx : bucket) {
				auto& senWord = senWords[idx];

				if (pickIdx.count(idx))
					continue;

				if (dest.length() - i >= senWord.blocks_[0].block_.length()
					&& dest.substr(i, senWord.blocks_[0].block_.length()) == senWord.blocks_[0].block_) {
					TraceWord traceWord;
					traceWord.senWordIdx_ = idx;
					traceWord.blockIdx_ = 0;
					traceWord.wordIdx_ = i + senWord.blocks_[0].block_.length();
					traceWord.wordIdxRec_[traceWord.blockIdx_].push_back(i);

					traceWords.push_back(traceWord);

					traceWordBuckets[senBlockIdx].push_back(traceWords.size() - 1);

					if (senWord.blockAmount_ > 1
						&& static_cast<int>(traceWord.wordIdxRec_[traceWord.blockIdx_].size()) >= senWord.blocks_[traceWord.blockIdx_].times_){
						traceNextWordBuckets[senWord.blocks_[traceWord.blockIdx_+1].senBlockHeadIdx_].push_back(traceWords.size() - 1);
					}
				}
			}
		}
	}

	for (auto& traceWord : traceWords) {
		auto& senWord = senWords[traceWord.senWordIdx_];

		if (traceWord.blockIdx_ + 1 >= senWord.blockAmount_) {
			for (int i = 0; i < senWord.blockAmount_; ++i) {
				for(auto idx : traceWord.wordIdxRec_[i]){
					dest.replace(dest.begin() + idx,
						dest.begin() + idx + senWord.blocks_[i].block_.length(),
						MASK, senWord.blocks_[i].block_.length());
				}
			}
		}
	}
}

void TestFilterWord()
{
	ifstream ifs("words.txt");
	if (!ifs.is_open())
		return;

	vector<string> words;
	string word;
	while(getline(ifs, word)){
		if (word[0] == ';')
			continue;

		words.push_back(word);
	}

	ifs.close();

	wind::IniFile confFile("conf.ini");
	int thrAmount = confFile.ReadInt("system", "thread", 1);
	int loopCount = confFile.ReadInt("system", "loop", 1);
	bool showFilterWord = confFile.ReadBool("system", "showFilterWord", false);

	EASYLOG << "Thread amount: " << thrAmount << endl;
	EASYLOG << "Loop count: " << loopCount << endl;
	EASYLOG << "Filter words amount: " << words.size() << endl;

	clock_t totalT = clock();
	int loop = 0;
	while(loop++ < loopCount){
		clock_t t = clock();
		vector<string> dests;
		dests.assign(words.size(), "");

		vector<thread> thrs;
		size_t filterAmount = words.size() / thrAmount + 1;
		for (int i = 0; i < thrAmount; ++i) {
			thrs.emplace_back([i, filterAmount, &words, &dests]()->void {
				string dest;
				for (size_t j = i * filterAmount; j < (i + 1)*filterAmount && j < words.size(); ++j) {
					FilterWord(words[j], dests[j]);
				}
			});
		}

		for (auto& thr : thrs) {
			thr.join();
		}

		EASYLOG << "Filter time: " << clock() - t << endl;
		if(showFilterWord){
			for (size_t i = 0; i < words.size(); ++i) {
				EASYLOG << words[i] << endl;
				EASYLOG << dests[i] << endl << endl;
			}
		}
	}

	clock_t totalCost = clock() - totalT;
	EASYLOG << "Total filter time: " << totalCost << endl;
	EASYLOG << "Filter per second: " << 1000*loopCount*words.size()/totalCost << endl;
}

int main()
{
	ostringstream osslog;
	osslog << "log_" << time(nullptr) << ".txt";
	ofslog.open(osslog.str());
	if (!ofslog.is_open())
		return 1;

	InitSenWords();

	TestFilterWord();

	ofslog.close();

	//system("pause");
	return 0;
}