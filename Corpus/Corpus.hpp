#ifndef CORPUS_HPP
#define CORPUS_HPP

#include <string>
#include <vector>
#include <map>

#include "../PCFG/PCFG.hpp"


/* Each word has the following format: positive s1 s2 s3 s4 */
class Corpus {
public:
	static const int keyAdjust = 10000;
	typedef std::vector<int> singleWord;
	typedef std::vector<singleWord> words;
private:
	int numOfSymbols;
	std::map<char,int> symbols;
	std::map<int,char> intToSymbols;
	words allWords;
	words uniqueWords;
	std::vector<double> wordCount;
	bool positiveDropped;
public:
	int totalStartWords;	
//Getters
public:
	int getTotalStartWords(){
		return totalStartWords;
	}
	words getUniqueWords(){
		return uniqueWords;
	}
	std::vector<double> getWordCount(){
		return wordCount;
	}
	int numberOfSymbolsInCorpus(){
		return numOfSymbols;
	}
	std::map<int,char> symbolsToChars(){
		return intToSymbols;
	}
	std::map<char,int> charsToSymbols(){
		return symbols;
	}
	unsigned int numberOfUniqueWords(){
		return uniqueWords.size();
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
	void clearForInsideOutside(){
		allWords.clear();
		uniqueWords.clear();
		wordCount.clear();
		positiveDropped = true;
	}
private:
	void reduceToUniqueWords();
	void recalculateUniqueWords();
	void sortUniqueWords();
public:
	void loadCorpusFromFile(std::string filename, bool omphalos){
		loadCorpusFromFile(filename, std::map<char,int>(), omphalos);
	}
	void dropPositive(){
		if(positiveDropped)
			return;
		for(unsigned int i=0; i<uniqueWords.size();i++){
			uniqueWords[i].erase(uniqueWords[i].begin());
		}
		positiveDropped = true;
	}
	void loadCorpusFromFile(std::string filename, std::map<char,int> inSymbols, bool omphalos);
	void dumbCorpusToFile(std::string filename, bool withCharSymbols);
	void printCorpus();
	void initReduceForPCFG(PCFG *pcfg);
	void resample(int howManySamples);
	void normalizeCorpus();
	void unnormalizeCorpus();

public:
	void addUniqueWord(singleWord wordToAdd, double count){
		uniqueWords.push_back(wordToAdd);
		wordCount.push_back(count);
	};
};


#endif //CORPUS_HPP
