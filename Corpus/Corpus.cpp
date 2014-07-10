#include "Corpus.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>



using namespace std;
const bool compareSingleWords(const Corpus::singleWord &i, const Corpus::singleWord &j){
	return i.count < j.count;
}

const bool compareIntMap(const std::map<int, int>::value_type &i1, const std::map<int, int>::value_type &i2){
	return i1.second<i2.second;
}

void Corpus::loadCorpusFromFile(string filename, std::map<char,int> inSymbols, bool omphalos){
	numOfSymbols =0;
	symbols = inSymbols;
	intToSymbols.clear();
	allWords.clear();

	//Read File line by line
	ifstream symbolFile;
	symbolFile.open(filename.c_str());
	string line;
	Corpus::singleWord currentWord;
	currentWord.count = 1;
	map<char,int>::iterator itSymbols;
	cout << "Loading Corpus from file: " << filename << endl;
	bool first = true;
	while(getline(symbolFile, line)){
		if(omphalos && first){
			first = false;
			continue;
		}
		currentWord.word.clear();
		string::iterator it = line.begin();
		if(*it == '1'){
			currentWord.positive = true;
		}else{
			currentWord.positive = false;
		}

		if(omphalos){
			it+=2;
			while(*it != ' '){
				it++;
			}
		}
		it++;
		for(; it != line.end(); it++){
			if(*it == ' '){
				continue;
			}

			itSymbols = symbols.find(*it);
			if(itSymbols == symbols.end()){
				symbols[*it] = numOfSymbols;
				intToSymbols[numOfSymbols] = *it;
				
				numOfSymbols++;

				itSymbols = symbols.find(*it);
			}
			currentWord.word.push_back(itSymbols->second);
		}
		allWords.push_back(currentWord);
	}
	symbolFile.close();
	reduceToUniqueWords();
	totalStartWords = allWords.size();
}

void Corpus::reduceToUniqueWords(){
	uniqueWords.clear();
	
	for(unsigned int iter = 0; iter < allWords.size(); iter++){
		bool found = false;
		for(unsigned int iter2 = 0; iter2 < uniqueWords.size(); iter2++){
			if(uniqueWords[iter2].word.size() != allWords[iter].word.size()){
				continue;
			}
			found = true;
			for(unsigned int i=0; i<allWords[iter].word.size(); i++){
				if(allWords[iter].word[i] != uniqueWords[iter2].word[i]){
					found = false;
					break;
				}
			}	
			if(found==true){
				uniqueWords[iter2].count++;				
				break;
			}
		}
		if(found == false){
			uniqueWords.push_back(allWords[iter]);
		}
	}
	std::sort(uniqueWords.begin(), uniqueWords.end(), compareSingleWords);
}

void Corpus::normalizeCorpus(){
	for(unsigned int iter = 0; iter < uniqueWords.size(); iter++){
		uniqueWords[iter].count = uniqueWords[iter].count/ (double)totalStartWords;
	}

}
void Corpus::unnormalizeCorpus(){
	for(unsigned int iter = 0; iter < uniqueWords.size(); iter++){
		uniqueWords[iter].count = uniqueWords[iter].count*(double)totalStartWords;
	}
}

void Corpus::initReduceForPCFG(PCFG *pcfg){
	for(unsigned int i=0; i<uniqueWords.size(); i++){
		for(unsigned int j=0; j<uniqueWords[i].word.size(); j++){
			uniqueWords[i].word[j] = pcfg->rulesForTermSymbol[uniqueWords[i].word[j]];
		}
	}
}

void Corpus::printCorpus(){
	int count = 1;
	cout << endl;
	cout << "-----Symbols and the number associated to them-----" << endl;
	for(map<char,int>::iterator it = symbols.begin(); it != symbols.end(); it++){
		cout << count << ") " << it->first << " " << it->second << endl;
		count++;
	}
	cout << endl;
	
	count = 1;
	cout << endl;
	cout << "-----Unique Words in Corpus-----" << endl;
	for(vector<singleWord>::iterator it1 = uniqueWords.begin(); it1 != uniqueWords.end(); it1++){
		cout << count << ") ";
		for(vector<int>::iterator it2 = it1->word.begin(); it2 != it1->word.end(); it2++){
			cout << intToSymbols[*it2] << " ";
		}
		cout << " count = " << it1->count;
		cout << " positive = " << it1->positive;
		count++;
	}
	cout << endl;
}

/*
**	Dumb corpus tou a file. If withCharSymbols is true it will dumb the char representations of the corpus.
**  if false it will dumb the integer representation
*/
void Corpus::dumbCorpusToFile(std::string filename, bool withCharSymbols){
	ofstream symbolFile;
	symbolFile.open(filename.c_str());
	for(unsigned int i=0; i<uniqueWords.size(); i++){
		double multiplicator = 1;
		if(uniqueWords[i].count < 0.9999){
			multiplicator = totalStartWords;
		}
		for(unsigned int j=0; j<uniqueWords[i].count*multiplicator; j++){
			for(unsigned int k=0; k<uniqueWords[i].word.size(); k++){
				if(withCharSymbols){
					symbolFile << intToSymbols[uniqueWords[i].word[k]] << " ";
				}else{
					symbolFile << uniqueWords[i].word[k] << " ";
				}
			}
			symbolFile << endl;
		}
	}
	symbolFile.close();
}