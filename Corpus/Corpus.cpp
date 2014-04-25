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
		for(string::iterator it = line.begin(); it != line.end(); it++){
			if(*it == ' '){
				continue;
			}

			itSymbols = symbols.find(*it);
			if(itSymbols == symbols.end()){
				symbols[*it] = numOfSymbols;
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
	allWords.clear();
}

void Corpus::reduceToUniqueWords(){
	uniqueWords.clear();
	wordCount.clear();
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
				break;
			}
		}
		if(found == false){
			wordCount.push_back(1);
			uniqueWords.push_back(allWords[iter]);
		}
	}
}

Corpus::allChunks Corpus::calculateBiwordsCount(){
	allChunks findAllChunks;
	int currSymbol;
	int prevSymbol;
	int previusKey;
	for(unsigned int i=0; i<uniqueWords.size(); i++){
		previusKey = -1;
		//Ignore all words of size two or less
		if(uniqueWords[i].size() <2){
			continue;
		}
		for(unsigned int j=0; j<uniqueWords[i].size(); j++){
			prevSymbol = currSymbol;
			currSymbol = uniqueWords[i][j];
			if(j==0){
				continue;
			}
			int key = prevSymbol*keyAdjust + currSymbol;
			map<int,int>::iterator iter = findAllChunks.biwordsCount.find(key);
			if(iter == findAllChunks.biwordsCount.end()){
				findAllChunks.biwordsCount[key] = 1;//*wordCount[i];
				findAllChunks.biwordsProb[key] = wordCount[i];
				findAllChunks.biwordsConflict[key] = false;
			}else{
				if(findAllChunks.biwordsConflict[key] == true || key == previusKey){
					findAllChunks.biwordsConflict[key] = true;
				}
				findAllChunks.biwordsCount[key]++; //+= wordCount[i];
				findAllChunks.biwordsProb[key]+= wordCount[i];
			}
			previusKey = key;
		}
	}
	return findAllChunks;
	//for(map<int,double>::iterator iter = biwordsCount.begin(); iter != biwordsCount.end(); iter++){
	//	cout << iter->first << " " << iter->second << " " << biwordsConflict[iter->first] << endl;
	//}
}

void Corpus::initReduceForPCFG(PCFG *pcfg){
	for(unsigned int i=0; i<uniqueWords.size(); i++){
		for(unsigned int j=0; j<uniqueWords[i].size(); j++){
			uniqueWords[i][j] = pcfg->rulesForTermSymbol[uniqueWords[i][j]];
		}
	}
}

int Corpus::reduceCorpusForRule(Rule rule){
	int timesFound = 0;
	for(unsigned int i=0; i<uniqueWords.size(); i++){
		singleWord newWord;
		bool reduce = true;
		int prevSymbol;
		int currSymbol = -1;
		for(unsigned int j=0; j<uniqueWords[i].size(); j++){
			prevSymbol = currSymbol;
			currSymbol = uniqueWords[i][j];
			if(reduce){
				reduce = false;
				continue;
			}
			for(unsigned int i=0; i<rule.totalNumberOfProductions(); i++){
				Rule::productions prod = rule.getProduction(i);
				if(!prod.terminal && prod.leftID == prevSymbol && prod.rightID == currSymbol){
					reduce = true;
					timesFound++;
					break;
				}
			}

			if(reduce){
				newWord.push_back(rule.id);
				prevSymbol = -1;
				currSymbol = -1;
			}else{
				newWord.push_back(prevSymbol);
			}
		}
		if(currSymbol != -1){
			newWord.push_back(currSymbol);
		}
		uniqueWords[i] = newWord;
	}
	recalculateUniqueWords();
	return timesFound;
}

int Corpus::recursivelyReduce(PCFG *pcfg){
	int timesFound = 0;
	for(int i=0; i<pcfg->allRules.size(); i++){
		int found = reduceCorpusForRule(pcfg->allRules[i]);
		//restart the loop!!!!
		timesFound += found;
		if(found > 0){
			i=0;
		}
	}
	recalculateUniqueWords();
	return timesFound;
}

void Corpus::recalculateUniqueWords(){
	for(unsigned int i=0; i<uniqueWords.size()-1; i++){
		for(unsigned int j=i+1; j<uniqueWords.size(); j++){
			if(uniqueWords[i].size() == uniqueWords[j].size()){
				bool found = true;
				for(unsigned int k=0; k<uniqueWords[i].size(); k++){
					if(uniqueWords[i][k] != uniqueWords[j][k]){
						found = false;
						break;
					}
				}
				if(found){
					wordCount[i]+=wordCount[j];
					uniqueWords.erase(uniqueWords.begin()+j);
					wordCount.erase(wordCount.begin()+j);
					break;
				}
			}
		}
	}
}

void Corpus::replaceRules(int newRuleID, int oldRuleID){
	for(words::iterator outIter = uniqueWords.begin(); outIter != uniqueWords.end(); outIter++){
		for(singleWord::iterator inerIter = outIter->begin(); inerIter != outIter->end(); inerIter++){
			if(*inerIter == oldRuleID){
				*inerIter = newRuleID;
			}
		}
	}
	recalculateUniqueWords();
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
		cout << endl;
		count++;
	}
	cout << endl;
}

