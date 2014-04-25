#ifndef CORPUS_HPP
#define CORPUS_HPP

#include <string>
#include <vector>
#include <map>

#include "../PCFG/PCFG.hpp"



class Corpus {
public:
	static const int keyAdjust = 10000;
	typedef std::vector<int> singleWord;
	typedef std::vector<singleWord > words;
	typedef struct{
		std::map<int, int> biwordsCount;
		std::map<int, double> biwordsProb;
		std::map<int, bool> biwordsConflict;
	}allChunks;
private:
	int numOfSymbols;
	std::map<char,int> symbols;
	words allWords;
	words uniqueWords;
	std::vector<double> wordCount;
	
	std::vector<double> wordCountBackup;
	
//Getters
public:
	words getUniqueWords(){
		return uniqueWords;
	}
	int numberOfSymbolsInCorpus(){
		return numOfSymbols;
	}
	int numberOfUniqueWords(){
		return uniqueWords.size();
	}
	singleWord countOfSymbols(){
		singleWord symCount(numOfSymbols);
		for(unsigned int i=0; i<uniqueWords.size(); i++){
			for(unsigned int j=0; j<uniqueWords[i].size(); j++){
				symCount[uniqueWords[i][j]] += wordCount[i];
			}
		}
		return symCount;
	}
private:
	void reduceToUniqueWords();
	void recalculateUniqueWords();
public:
	void loadCorpusFromFile(std::string filename);
	allChunks calculateBiwordsCount();
	void printCorpus();
	void initReduceForPCFG(PCFG *pcfg);
	int reduceCorpusForRule(Rule rule);
	int recursivelyReduce(PCFG *pcfg);
	void expandCorpusForRule(Rule rule);
	void replaceRules(int newRuleID, int oldRuleID);
};


#endif //CORPUS_HPP
