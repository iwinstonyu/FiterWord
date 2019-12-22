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
#include <cassert>
#include "IniFile.h"

using namespace std;

typedef unsigned char uchar;
typedef unsigned int uint;

int Utf8CharLegnth(unsigned char ch)
{
	if ((ch & 0x80) == 0)
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
	SenWord() = default;

	explicit SenWord(string word) : word_(word)
	{
		blocks_.reserve(20);
	}

	struct Block
	{
		Block(string& block) : block_(block), times_(1) 
		{
			head_ = block.substr(0, Utf8CharLegnth(block[0]));
		}

		string block_;
		string head_;
		size_t times_ = 0;
		int senBlockHeadIdx_ = -1;
	};

	void AddBlock(string& block)
	{
		if (!blocks_.empty() && blocks_.back().block_ == block){
			++blocks_.back().times_;
		}
		else{
			blocks_.emplace_back(block);
		}
	}

	int BlockAmount() { return static_cast<int>(blocks_.size()); }

	string word_;
	vector<Block> blocks_;
};

vector<SenWord> gSenWords;
vector<vector<int>> gSenWordBuckets;
vector<string> gSenBlockHeads;

struct TraceWord
{
	TraceWord() = default;

	explicit TraceWord(int senWordIdx) : senWordIdx_(senWordIdx), blockIdx_(0), wordIdx_(0)
	{
		assert(senWordIdx < gSenWords.size());
		auto& senWord = gSenWords[senWordIdx];
		wordIdxRec_.assign(senWord.blocks_.size(), vector<int>());
	}

	int senWordIdx_ = -1;
	int blockIdx_ = -1;
	int wordIdx_ = -1;
	vector<vector<int>> wordIdxRec_;
};

ofstream ofslog;
#define EASYLOG ofslog

int GetSenBlockHeadIdx(string& str)
{
	auto it = find(gSenBlockHeads.begin(), gSenBlockHeads.end(), str);
	if (it != gSenBlockHeads.end())
		return static_cast<int>(distance(gSenBlockHeads.begin(), it));
	else
		return -1;
}

void InitSenWords()
{
	//vector<string> originWords;
	//originWords.push_back("a_b_c");
	//originWords.push_back("a_c_d");
	//originWords.push_back("a_c_e_");

	gSenWords.reserve(10000);
	gSenBlockHeads.reserve(10000);

	ifstream ifs("sensitive.txt");
	if (!ifs.is_open())
		return;

	string word;
	set<string> blockHeads;
	while (getline(ifs, word)) {
		if (word.empty() 
			|| all_of(word.begin(), word.end(), [](uchar c)->bool { return isspace(c); }))
			continue;

		gSenWords.emplace_back(word);
		auto& senWord = gSenWords.back();

		size_t beg = 0;
		for (size_t idx = 0; idx <= word.length(); ++idx) {
			if(idx == word.length() || word[idx] == '_') {
				if(idx > beg){
					string block(word.begin()+beg, word.begin()+idx);
					if (block.empty() 
						|| all_of(block.begin(), block.end(), [](uchar c)->bool { return isspace(c); })) {
						beg = idx + 1;
						continue;
					}

					blockHeads.insert(block.substr(0, Utf8CharLegnth(block[0])));
					senWord.AddBlock(block);
				}
				beg = idx + 1;
			}
		}
	}

	ifs.close();

	copy(blockHeads.begin(), blockHeads.end(), back_inserter(gSenBlockHeads));

	gSenWordBuckets.assign(gSenBlockHeads.size(), vector<int>());
	for (size_t i = 0; i < gSenWords.size(); ++i){
		auto& senWord = gSenWords[i];

		for(int j = 0; j < senWord.BlockAmount(); ++j){
			auto& block = senWord.blocks_[j];

			auto it = find(gSenBlockHeads.begin(), gSenBlockHeads.end(), block.head_);
			if (it != gSenBlockHeads.end()) {
				block.senBlockHeadIdx_ = static_cast<int>(distance(gSenBlockHeads.begin(), it));

				if (j == 0) {
					gSenWordBuckets[block.senBlockHeadIdx_].push_back(static_cast<int>(i));
				}
			}
		}
	}

	EASYLOG << "Load sensitive word amount: " << gSenWords.size() << endl;
	EASYLOG << "Load sensitive block heads amount: " << gSenBlockHeads.size() << endl;
}

void FilterWord(string& src, string& dest)
{
	dest = src;

	vector<TraceWord> traceWords;
	vector<vector<int>> traceWordBuckets;
	vector<vector<int>> traceNextWordBuckets;
	traceWordBuckets.assign(gSenBlockHeads.size(), vector<int>());
	traceNextWordBuckets.assign(gSenBlockHeads.size(), vector<int>());

	for (size_t i = 0; i < dest.length(); i += Utf8CharLegnth(dest[i])) {
		set<size_t> pickIdx;

		string subWord(dest.begin()+i, dest.begin() + i + Utf8CharLegnth(dest[i]));

		int headIdx = GetSenBlockHeadIdx(subWord);
		if(headIdx >= 0){
			auto& traceBucket = traceWordBuckets[headIdx];
			for (auto idx : traceBucket) {
				auto& traceWord = traceWords[idx];
				auto& senWord = gSenWords[traceWord.senWordIdx_];

				if (traceWord.wordIdx_ <= i
					&& dest.length() - i >= senWord.blocks_[traceWord.blockIdx_].block_.length()
					&& dest.substr(i, senWord.blocks_[traceWord.blockIdx_].block_.length()) == senWord.blocks_[traceWord.blockIdx_].block_) {
					traceWord.wordIdx_ = static_cast<int>(i + senWord.blocks_[traceWord.blockIdx_].block_.length());
					traceWord.wordIdxRec_[traceWord.blockIdx_].push_back(static_cast<int>(i));

					if (traceWord.blockIdx_ == 0)
						pickIdx.insert(traceWord.senWordIdx_);

					if (traceWord.blockIdx_ + 1 < senWord.BlockAmount()
						&& traceWord.wordIdxRec_[traceWord.blockIdx_].size() == senWord.blocks_[traceWord.blockIdx_].times_) {
						traceNextWordBuckets[senWord.blocks_[traceWord.blockIdx_+1].senBlockHeadIdx_].push_back(idx);
					}
				}
			}

			auto& traceNextBucket = traceNextWordBuckets[headIdx];
			vector<int> eraseIdx;
			for (auto idx : traceNextBucket) {
				auto& traceWord = traceWords[idx];
				auto& senWord = gSenWords[traceWord.senWordIdx_];

				if (traceWord.wordIdx_ <= i
					&& traceWord.blockIdx_ + 1 < senWord.BlockAmount()
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
					traceWord.wordIdx_ = static_cast<int>(i + senWord.blocks_[traceWord.blockIdx_].block_.length());
					traceWord.wordIdxRec_[traceWord.blockIdx_].push_back(static_cast<int>(i));
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
				auto& senWord = gSenWords[traceWord.senWordIdx_];

				if (traceWord.blockIdx_ + 1 < senWord.BlockAmount()) {
					traceNextWordBuckets[senWord.blocks_[traceWord.blockIdx_ + 1].senBlockHeadIdx_].push_back(idx);
				}
			}

			auto& bucket = gSenWordBuckets[headIdx];
			for (auto idx : bucket) {
				auto& senWord = gSenWords[idx];

				if (pickIdx.count(idx))
					continue;

				if (dest.length() - i >= senWord.blocks_[0].block_.length()
					&& dest.substr(i, senWord.blocks_[0].block_.length()) == senWord.blocks_[0].block_) {
					TraceWord traceWord(idx);
					traceWord.wordIdx_ = static_cast<int>(i + senWord.blocks_[0].block_.length());
					traceWord.wordIdxRec_[traceWord.blockIdx_].push_back(static_cast<int>(i));

					traceWords.push_back(traceWord);

					traceWordBuckets[headIdx].push_back(static_cast<int>(traceWords.size() - 1));

					if (senWord.BlockAmount() > 1
						&& traceWord.wordIdxRec_[traceWord.blockIdx_].size() >= senWord.blocks_[traceWord.blockIdx_].times_){
						traceNextWordBuckets[senWord.blocks_[traceWord.blockIdx_+1].senBlockHeadIdx_].push_back(static_cast<int>(traceWords.size() - 1));
					}
				}
			}
		}
	}

	for (auto& traceWord : traceWords) {
		auto& senWord = gSenWords[traceWord.senWordIdx_];

		if (traceWord.blockIdx_ + 1 >= senWord.BlockAmount()
			&& traceWord.wordIdxRec_[traceWord.blockIdx_].size() >= senWord.blocks_[traceWord.blockIdx_].times_) {
			for (int i = 0; i < senWord.BlockAmount(); ++i) {
				for(auto idx : traceWord.wordIdxRec_[i]){
					dest.replace(dest.begin() + idx,
						dest.begin() + idx + senWord.blocks_[i].block_.length(),
						senWord.blocks_[i].block_.length(), '*');
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