#ifndef CORPUS_HPP
#define CORPUS_HPP

#include <string>
#include <vector>
#include <map>

#include "../PCFG/PCFG.hpp"


/* Each word has the following format: positive s1 s2 s3 s4 */
class Corpus {
public:
	typedef struct{
		std::vector<int> word;
		double count;
		bool positive;
	}singleWord;

	typedef std::vector<singleWord> words;
private:
	int numOfSymbols;
	std::map<char,int> symbols;
	std::map<int,char> intToSymbols;
	words allWords;
	words uniqueWords;
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

	words getInitUnsortedWords(){
		return allWords;
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
			for(unsigned int j=0; j<uniqueWords[i].word.size(); j++){
				symCount[uniqueWords[i].word[j]] += uniqueWords[i].count;
			}
		}
		return symCount;
	}
	void clearForInsideOutside(){
		allWords.clear();
		uniqueWords.clear();
	}
private:
	void reduceToUniqueWords();
public:
	void loadCorpusFromFile(std::string filename, bool omphalos){
		loadCorpusFromFile(filename, std::map<char,int>(), omphalos);
	}

	void loadCorpusFromFile(std::string filename, std::map<char,int> inSymbols, bool omphalos);
	void dumbCorpusToFile(std::string filename, bool withCharSymbols);
	void printCorpus();
	void initReduceForPCFG(PCFG *pcfg);
	void normalizeCorpus();
	void unnormalizeCorpus();

public:
	void addUniqueWord(singleWord wordToAdd){
		uniqueWords.push_back(wordToAdd);
	};
};


#endif //CORPUS_HPP
