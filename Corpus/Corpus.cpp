#include "Corpus.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>



using namespace std;
const bool compareVectorSize(const Corpus::singleWord &i, const Corpus::singleWord &j){
	return i.size() < j.size();
}

const bool compareIntMap(const std::map<int, int>::value_type &i1, const std::map<int, int>::value_type &i2){
	return i1.second<i2.second;
}

void Corpus::loadCorpusFromFile(string filename){
	numOfSymbols =0;

	symbols.clear();
	intToSymbols.clear();
	allWords.clear();

	//Read File line by line
	ifstream symbolFile;
	symbolFile.open(filename.c_str());
	string line;
	vector<int> currentWord;
	map<char,int>::iterator itSymbols;
	cout << "Loading Corpus from file: " << filename << endl;
	while(getline(symbolFile, line)){

		currentWord.clear();
		string::iterator it = line.begin();
		if(*it == '1'){
			positive.push_back(1);
		}else{
			positive.push_back(0);
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
			currentWord.push_back(itSymbols->second);
		}
		allWords.push_back(currentWord);
	}
	symbolFile.close();
	
	std::sort(allWords.begin(), allWords.end(), compareVectorSize);
	reduceToUniqueWords();
	totalStartWords = allWords.size();
	allWords.clear();
}

void Corpus::reduceToUniqueWords(){
	uniqueWords.clear();
	wordCount.clear();
	std::vector<char> newPositive; 
	
	for(unsigned int iter = 0; iter < allWords.size(); iter++){
		bool found = false;
		for(unsigned int iter2 = 0; iter2 < uniqueWords.size(); iter2++){
			if(uniqueWords[iter2].size() != allWords[iter].size()){
				continue;
			}
			found = true;
			for(unsigned int i=0; i<allWords[iter].size(); i++){
				if(allWords[iter][i] != uniqueWords[iter2][i]){
					found = false;
					break;
				}
			}	
			if(found==true){
				wordCount[iter2]++;
				if((bool)newPositive[iter2] == false && (bool)positive[iter] == true){
					newPositive[iter2] = (char)false;
				}
				break;
			}
		}
		if(found == false){
			wordCount.push_back(1);
			newPositive.push_back(positive[iter]);
			uniqueWords.push_back(allWords[iter]);
		}
	}
	positive = newPositive;
}

void Corpus::normalizeCorpus(){
	for(unsigned int iter = 0; iter < wordCount.size(); iter++){
		wordCount[iter] = wordCount[iter] / (double)totalStartWords;
	}

}
void Corpus::unnormalizeCorpus(){
	for(unsigned int iter = 0; iter < wordCount.size(); iter++){
		wordCount[iter] = wordCount[iter]*(double)totalStartWords;
	}
}

void Corpus::resample(int howManySamples){	
	words newUniqueWords;
	std::vector<double> newWordCount; 
	std::vector<char> newPositive; 
	vector<bool> toDelete(uniqueWords.size(),false);
	
	double step = (double)totalStartWords/(double)howManySamples;
	int j=-1;
	double cumSum = 0.0;
	for(double i=0.0; i<(double)totalStartWords; i+=step){
		if(cumSum <= i){
			while(cumSum <= i && j < (int)wordCount.size()){
				j++;
				cumSum+=wordCount[j];
			}
			if(j == wordCount.size()){
				break;
			}

			newUniqueWords.push_back(uniqueWords[j]);
			newWordCount.push_back(1);
			newPositive.push_back(positive[j]);
			
		}else{
			newWordCount.back()++;
		}
	}
	
	uniqueWords = newUniqueWords;
	wordCount = newWordCount;
}

void Corpus::initReduceForPCFG(PCFG *pcfg){
	for(unsigned int i=0; i<uniqueWords.size(); i++){
		for(unsigned int j=0; j<uniqueWords[i].size(); j++){
			uniqueWords[i][j] = pcfg->rulesForTermSymbol[uniqueWords[i][j]];
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
	for(vector<vector<int> >::iterator it1 = uniqueWords.begin(); it1 != uniqueWords.end(); it1++){
		cout << count << ") ";
		for(vector<int>::iterator it2 = it1->begin(); it2 != it1->end(); it2++){
			cout << *it2 << " ";
		}
		cout << " count = " << wordCount[count-1];
		cout << " positive = " << (bool)positive[count-1];
		cout << endl;
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
		if(wordCount[i] < 1){
			std::cerr << "Corpus is at normalized form and cannot be dumbed" << std::endl;
			symbolFile.close();
			return;
		}
		for(unsigned int j=0; j<wordCount[i]; j++){
			for(unsigned int k=0; k<uniqueWords[i].size(); k++){
				if(withCharSymbols){
					symbolFile << intToSymbols[uniqueWords[i][k]] << " ";
				}else{
					symbolFile << uniqueWords[i][k] << " ";
				}
			}
			symbolFile << endl;
		}
	}
	symbolFile.close();
}