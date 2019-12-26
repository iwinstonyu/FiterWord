// FilterWord.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

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
			tail_ = block.substr(Utf8CharLegnth(block[0]), block.length() - Utf8CharLegnth(block[0]));
		}

		string block_;
		string head_;
		string tail_;
		size_t times_ = 0;
		int senBlockHeadIdx_ = -1;
		int senBlockIdx_ = -1;
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

struct BucketNode
{
	BucketNode() = default;
	BucketNode(int idx, int headIdx) : idx_(idx), headIdx_(headIdx) {}
	void Reset() {
		preNode_ = nullptr; nextNode_ = nullptr; idx_ = -1; headIdx_ = -1;
	}

	BucketNode* preNode_ = nullptr;
	BucketNode* nextNode_ = nullptr;
	int idx_ = -1;
	int headIdx_ = -1;
};

struct RecycleNodes
{
	static const int RESERVE_NODES_SIZE = 1000;

	RecycleNodes()
	{
		reserveNodesPtr_ = new BucketNode[RESERVE_NODES_SIZE];

		for (size_t i = 0; i < RESERVE_NODES_SIZE; ++i) {
			if (i > 0) {
				reserveNodesPtr_[i].preNode_ = &(reserveNodesPtr_[i - 1]);
			}
			if (i + 1 < RESERVE_NODES_SIZE) {
				reserveNodesPtr_[i].nextNode_ = &(reserveNodesPtr_[i + 1]);
			}
		}
		recyclePtr_ = &(reserveNodesPtr_[RESERVE_NODES_SIZE - 1]);
	}

	~RecycleNodes() 
	{
		auto nodePtr = recyclePtr_;
		BucketNode* preNodePtr = nullptr;
		while (nodePtr) {
			preNodePtr = nodePtr->preNode_;
			if(nodePtr < &(reserveNodesPtr_[0]) || nodePtr > &(reserveNodesPtr_[RESERVE_NODES_SIZE-1]))
				delete nodePtr;
			nodePtr = preNodePtr;
		}
		recyclePtr_ = nullptr;
	}

	BucketNode* NewNode(int idx, int headIdx)
	{
		BucketNode* nodePtr = nullptr;
		if (recyclePtr_) {
			nodePtr = recyclePtr_;
			recyclePtr_ = recyclePtr_->preNode_;
			if(recyclePtr_)
				recyclePtr_->nextNode_ = nullptr;

			nodePtr->Reset();
			nodePtr->idx_ = idx;
			nodePtr->headIdx_ = headIdx;
		}
		else {
			nodePtr = new BucketNode(idx, headIdx);
		}

		return nodePtr;
	}

	void DelNode(BucketNode* nodePtr)
	{
		nodePtr->Reset();
		if (recyclePtr_) {
			recyclePtr_->nextNode_ = nodePtr;
			nodePtr->preNode_ = recyclePtr_;
			recyclePtr_ = nodePtr;
		}
		else {
			recyclePtr_ = nodePtr;
		}
	}

	BucketNode* recyclePtr_ = nullptr;
	BucketNode* reserveNodesPtr_ = nullptr;
};

struct BucketNodeList
{
	BucketNodeList(RecycleNodes& recycleNodes) :recycleNodesPtr_(&recycleNodes) {}
	
	~BucketNodeList()
	{
		auto nodePtr = tailPtr_;
		BucketNode* preNodePtr = nullptr;
		while (nodePtr) {
			preNodePtr = nodePtr->preNode_;
			recycleNodesPtr_->DelNode(nodePtr);
			nodePtr = preNodePtr;
		}
		headPtr_ = nullptr;
		tailPtr_ = nullptr;
	}

	BucketNode* AddNode(int idx, int headIdx)
	{
		++amount_;

		BucketNode* nodePtr = recycleNodesPtr_->NewNode(idx, headIdx);

		if (headPtr_ == nullptr)
			headPtr_ = nodePtr;

		if (tailPtr_ == nullptr) {
			tailPtr_ = nodePtr;
		}
		else {
			tailPtr_->nextNode_ = nodePtr;
			nodePtr->preNode_ = tailPtr_;
			tailPtr_ = nodePtr;
		}

		return nodePtr;
	}

	void DelNode(BucketNode* nodePtr)
	{
		--amount_;

		assert(nodePtr);
		assert(nodePtr->idx_ >= 0);

// 		auto it = headPtr_;
// 		while (it && it != nodePtr) it = it->nextNode_;
// 
// 		if (it != nodePtr)
// 			return;

		if(nodePtr == headPtr_)
			headPtr_ = nodePtr->nextNode_;
		if (nodePtr == tailPtr_)
			tailPtr_ = nodePtr->preNode_;
		if (nodePtr->preNode_)
			nodePtr->preNode_->nextNode_ = nodePtr->nextNode_;
		if (nodePtr->nextNode_)
			nodePtr->nextNode_->preNode_ = nodePtr->preNode_;

		recycleNodesPtr_->DelNode(nodePtr);
	}

	BucketNode* headPtr_ = nullptr;
	BucketNode* tailPtr_ = nullptr;
	RecycleNodes* recycleNodesPtr_;
	int amount_ = 0;
};

vector<SenWord> gSenWords;
vector<vector<int>> gSenWordBuckets;
vector<string> gSenBlockHeads;
vector<string> gSenBlocks;

struct TraceWord
{
	TraceWord() = default;

	explicit TraceWord(int senWordIdx) : senWordIdx_(senWordIdx), blockIdx_(0), wordIdx_(0)
	{
		assert(senWordIdx < static_cast<int>(gSenWords.size()));
		auto& senWord = gSenWords[senWordIdx];
		wordIdxRec_.assign(senWord.blocks_.size(), vector<int>());
	}

	int senWordIdx_ = -1;
	int blockIdx_ = -1;
	int wordIdx_ = -1;
	vector<vector<int>> wordIdxRec_;
	BucketNode* traceNodePtr_ = nullptr;
};

ofstream ofslog;
#define EASYLOG ofslog

int GetSenBlockHeadIdx(string& str)
{
	auto it = lower_bound(gSenBlockHeads.begin(), gSenBlockHeads.end(), str);
	if (it != gSenBlockHeads.end() && *it == str)
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
	set<string> blocks;
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
					blocks.insert(block);
					senWord.AddBlock(block);
				}
				beg = idx + 1;
			}
		}
	}

	ifs.close();

	copy(blockHeads.begin(), blockHeads.end(), back_inserter(gSenBlockHeads));
	copy(blocks.begin(), blocks.end(), back_inserter(gSenBlocks));

	gSenWordBuckets.assign(gSenBlockHeads.size(), vector<int>());
	for (size_t i = 0; i < gSenWords.size(); ++i){
		auto& senWord = gSenWords[i];

		for(int j = 0; j < senWord.BlockAmount(); ++j){
			auto& block = senWord.blocks_[j];

			{
				auto it = lower_bound(gSenBlockHeads.begin(), gSenBlockHeads.end(), block.head_);
				if (it != gSenBlockHeads.end() && *it == block.head_) {
					block.senBlockHeadIdx_ = static_cast<int>(distance(gSenBlockHeads.begin(), it));

					if (j == 0) {
						gSenWordBuckets[block.senBlockHeadIdx_].push_back(static_cast<int>(i));
					}
				}
			}
			{
				auto it = lower_bound(gSenBlocks.begin(), gSenBlocks.end(), block.block_);
				if (it != gSenBlocks.end() && *it == block.block_) {
					block.senBlockIdx_ = static_cast<int>(distance(gSenBlocks.begin(), it));
				}
			}
		}
	}

	EASYLOG << "Load sensitive word amount: " << gSenWords.size() << endl;
	EASYLOG << "Load sensitive block heads amount: " << gSenBlockHeads.size() << endl;
}

void FilterWord(string& src, string& dest, RecycleNodes& recycleNodes)
{

	dest = src;

	vector<TraceWord> traceWords;
	traceWords.reserve(1000);
	vector<BucketNodeList> traceWordBuckets;
	traceWordBuckets.assign(gSenBlockHeads.size(), BucketNodeList(recycleNodes));
	vector<BucketNodeList> traceNextWordBuckets;
	traceNextWordBuckets.assign(gSenBlockHeads.size(), BucketNodeList(recycleNodes));

	for (size_t i = 0; i < dest.length(); i += Utf8CharLegnth(dest[i])) {
		assert(i <= 84065);
		set<size_t> pickIdx;

		size_t nextCharIdx = i + Utf8CharLegnth(dest[i]);
		string subWord(dest.begin()+i, dest.begin() + nextCharIdx);

		int headIdx = GetSenBlockHeadIdx(subWord);
		if (headIdx >= 0) {
			{
				auto nodePtr = traceWordBuckets[headIdx].headPtr_;
				while (nodePtr) {
					auto& traceWord = traceWords[nodePtr->idx_];
					auto& senWord = gSenWords[traceWord.senWordIdx_];

					if (traceWord.wordIdx_ <= static_cast<int>(i)
						&& (senWord.blocks_[traceWord.blockIdx_].tail_.empty()
							|| (dest.length() - nextCharIdx >= senWord.blocks_[traceWord.blockIdx_].tail_.length()
								&& !strncmp(dest.c_str() + nextCharIdx, senWord.blocks_[traceWord.blockIdx_].tail_.c_str(), senWord.blocks_[traceWord.blockIdx_].tail_.length())))) {
						//&& dest.substr(nextCharIdx, senWord.blocks_[traceWord.blockIdx_].tail_.length()) == senWord.blocks_[traceWord.blockIdx_].tail_))) {
						traceWord.wordIdx_ = static_cast<int>(i + senWord.blocks_[traceWord.blockIdx_].block_.length());
						traceWord.wordIdxRec_[traceWord.blockIdx_].push_back(static_cast<int>(i));
						assert(static_cast<int>(i) <= 84065);

						if (traceWord.blockIdx_ == 0)
							pickIdx.insert(traceWord.senWordIdx_);

						if (traceWord.blockIdx_ + 1 < senWord.BlockAmount()
							&& traceWord.wordIdxRec_[traceWord.blockIdx_].size() == senWord.blocks_[traceWord.blockIdx_].times_) {
							traceNextWordBuckets[senWord.blocks_[traceWord.blockIdx_ + 1].senBlockHeadIdx_].AddNode(nodePtr->idx_, senWord.blocks_[traceWord.blockIdx_ + 1].senBlockHeadIdx_);
						}
					}

					nodePtr = nodePtr->nextNode_;
				}
			}

			vector<int> eraseIdx;
			vector<int> reserveIdx;
			auto nextNodePtr = traceNextWordBuckets[headIdx].headPtr_;
			while (nextNodePtr) {
				auto& traceWord = traceWords[nextNodePtr->idx_];
				auto& senWord = gSenWords[traceWord.senWordIdx_];

				if (traceWord.wordIdx_ <= static_cast<int>(i)
					&& traceWord.blockIdx_ + 1 < senWord.BlockAmount()
					&& dest.length() - nextCharIdx >= senWord.blocks_[traceWord.blockIdx_ + 1].tail_.length()
					&& !strncmp(dest.c_str() + nextCharIdx, senWord.blocks_[traceWord.blockIdx_ + 1].tail_.c_str(), senWord.blocks_[traceWord.blockIdx_ + 1].tail_.length())) {
					//&& dest.substr(i, senWord.blocks_[traceWord.blockIdx_ + 1].block_.length()) == senWord.blocks_[traceWord.blockIdx_ + 1].block_) {
					if (traceWord.traceNodePtr_) {
						traceWordBuckets[senWord.blocks_[traceWord.blockIdx_].senBlockHeadIdx_].DelNode(traceWord.traceNodePtr_);
						traceWord.traceNodePtr_ = nullptr;
					}

					{
						BucketNode* tempNodePtr = traceWordBuckets[headIdx].AddNode(nextNodePtr->idx_, headIdx);
						traceWord.traceNodePtr_ = tempNodePtr;
						if (headIdx != senWord.blocks_[traceWord.blockIdx_ + 1].senBlockHeadIdx_)
							return;
					}
	
					eraseIdx.push_back(nextNodePtr->idx_);

					auto tempNodePtr = nextNodePtr->nextNode_;
					traceNextWordBuckets[headIdx].DelNode(nextNodePtr);
					nextNodePtr = tempNodePtr;

					traceWord.blockIdx_ += 1;
					traceWord.wordIdx_ = static_cast<int>(i + senWord.blocks_[traceWord.blockIdx_].block_.length());
					traceWord.wordIdxRec_[traceWord.blockIdx_].push_back(static_cast<int>(i));
					assert(static_cast<int>(i) <= 84065);
				}
				else{
					nextNodePtr = nextNodePtr->nextNode_;
				}
			}

			for (auto idx : eraseIdx) {
				auto& traceWord = traceWords[idx];
				auto& senWord = gSenWords[traceWord.senWordIdx_];

				if (traceWord.blockIdx_ + 1 < senWord.BlockAmount()
					&& traceWord.wordIdxRec_[traceWord.blockIdx_].size() == senWord.blocks_[traceWord.blockIdx_].times_) {
					traceNextWordBuckets[senWord.blocks_[traceWord.blockIdx_ + 1].senBlockHeadIdx_].AddNode(idx, senWord.blocks_[traceWord.blockIdx_ + 1].senBlockHeadIdx_);
				}
			}

			auto& bucket = gSenWordBuckets[headIdx];
			for (auto idx : bucket) {
				auto& senWord = gSenWords[idx];

				if (pickIdx.count(idx))
					continue;

				if (dest.length() - i >= senWord.blocks_[0].block_.length()
					&& dest.substr(i, senWord.blocks_[0].block_.length()) == senWord.blocks_[0].block_) {
					traceWords.emplace_back(idx);

					auto& traceWord = traceWords.back();
					traceWord.wordIdx_ = static_cast<int>(i + senWord.blocks_[0].block_.length());
					traceWord.wordIdxRec_[traceWord.blockIdx_].push_back(static_cast<int>(i));
					assert(static_cast<int>(i) <= 84065);

					{
						BucketNode* tempNodePtr = traceWordBuckets[headIdx].AddNode(static_cast<int>(traceWords.size() - 1), headIdx);
						traceWord.traceNodePtr_ = tempNodePtr;
						if (senWord.blocks_[traceWord.blockIdx_].senBlockHeadIdx_ != headIdx)
							return;
					}

					if (senWord.BlockAmount() > 1
						&& traceWord.wordIdxRec_[traceWord.blockIdx_].size() >= senWord.blocks_[traceWord.blockIdx_].times_){
						traceNextWordBuckets[senWord.blocks_[traceWord.blockIdx_ + 1].senBlockHeadIdx_].AddNode(static_cast<int>(traceWords.size() - 1), senWord.blocks_[traceWord.blockIdx_ + 1].senBlockHeadIdx_);
					}
				}
			}
		}
	}

// 	vector<size_t> marks;
// 	marks.assign(dest.size(), 0);
// 	for (auto& traceWord : traceWords) {
// 		auto& senWord = gSenWords[traceWord.senWordIdx_];
// 
// 		if (traceWord.blockIdx_ + 1 >= senWord.BlockAmount()
// 			&& traceWord.wordIdxRec_[traceWord.blockIdx_].size() >= senWord.blocks_[traceWord.blockIdx_].times_) {
// 			for (int i = 0; i < senWord.BlockAmount(); ++i) {
// 				for(auto idx : traceWord.wordIdxRec_[i]) {
// 					if (marks[idx] >= senWord.blocks_[i].block_.length())
// 						continue;
// 					else
// 						marks[idx] = senWord.blocks_[i].block_.length();
// 				
// 					dest.replace(dest.begin() + idx,
// 						dest.begin() + idx + senWord.blocks_[i].block_.length(),
// 						senWord.blocks_[i].block_.length(), '*');
// 				}
// 			}
// 		}
// 	}
	for (auto& traceWord : traceWords) {
		auto& senWord = gSenWords[traceWord.senWordIdx_];

		if (traceWord.blockIdx_ + 1 >= senWord.BlockAmount()
			&& traceWord.wordIdxRec_[traceWord.blockIdx_].size() >= senWord.blocks_[traceWord.blockIdx_].times_) {
			for (int i = 0; i < senWord.BlockAmount(); ++i) {
				for (auto idx : traceWord.wordIdxRec_[i]) {
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
				RecycleNodes recycleNodes;
				string dest;
				for (size_t j = i * filterAmount; j < (i + 1)*filterAmount && j < words.size(); ++j) {
					FilterWord(words[j], dests[j], recycleNodes);
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