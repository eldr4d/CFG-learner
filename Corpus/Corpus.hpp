#ifndef CORPUS_HPP
#define CORPUS_HPP

#include <string>
#include <vector>
#include <map>

#include "../PCFG/PCFG.hpp"

typedef struct{
	int leftSymbol;
	int rightSymbol;
	double prob;
	int count;
	bool conflict;
}bestChunk;

class Corpus {
public:
	typedef std::vector<int> singleWord;
	typedef std::vector<singleWord > words;
private:
	int numOfSymbols;
	std::map<char,int> symbols;
	words allWords;
	words uniqueWords;
	std::vector<double> wordCount;
	
	std::vector<double> wordCountBackup;
	
	std::map<int, int> biwordsCount;
	std::map<int, double> biwordsProb;
	std::map<int, bool> biwordsConflict;
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
public:
	void loadCorpusFromFile(std::string filename);
	void calculateBiwordsCount();
	void printCorpus();
	void initReduceForPCFG(PCFG *pcfg);
	void reduceCorpusForRule(Rule rule);
	void expandCorpusForRule(Rule rule);
	void replaceRules(int newRuleID, int oldRuleID);
	bestChunk findMaxChunk();
};


#endif //CORPUS_HPP
