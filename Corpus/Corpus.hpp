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
	typedef std::vector<singleWord> words;
private:
	int numOfSymbols;
	std::map<char,int> symbols;
	std::map<int,char> intToSymbols;
	words allWords;
	words uniqueWords;
	std::vector<double> wordCount;
	std::vector<char> positive; //Check if the word is actually in the language
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
	std::vector<char> getPositiveStatus(){
		return positive;
	}
	int numberOfSymbolsInCorpus(){
		return numOfSymbols;
	}
	std::map<int,char> symbolsToChars(){
		return intToSymbols;
	}
	unsigned int numberOfUniqueWords(){
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
	void clearForInsideOutside(){
		allWords.clear();
		uniqueWords.clear();
		wordCount.clear();
		positive.clear();
	}
private:
	void reduceToUniqueWords();
	void recalculateUniqueWords();
public:
	void loadCorpusFromFile(std::string filename);
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
