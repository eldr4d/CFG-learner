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
	int numberOfWords, numberOfUniqueWords;
private:
	int numOfSymbols;
	std::map<char,int> symbols;
	std::map<int,char> invertSymbols;
	std::vector<std::vector<int> > allWords;
	std::vector<std::vector<int> > uniqueWords;
	std::vector<double> wordCount;
	
	std::map<int, int> biwordsCount;
	std::map<int, double> biwordsProb;
	std::map<int, bool> biwordsConflict;
//Getters
public:
	std::vector<std::vector<int> > getAllWords(){
		return allWords;
	}
	std::vector<std::vector<int> > getUniqueWords(){
		return uniqueWords;
	}
	int numberOfSymbolsInCorpus(){
		return numOfSymbols;
	}
	std::vector<int> countOfSymbols(){
		std::vector<int> symCount(numOfSymbols);
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
	void printCorpus(bool unique);
	void initReduceForPCFG(PCFG *pcfg);
	void reduceCorpusForRule(Rule *rule);
	void expandCorpusForRule(Rule *rule);
	bestChunk findMaxChunk();
};


#endif //CORPUS_HPP
