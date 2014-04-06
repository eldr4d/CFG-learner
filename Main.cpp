#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <math.h>

#include "Corpus/Corpus.hpp"
#include "PCFG/PCFG.hpp"
#include "CYK/CYKparser.hpp"
using namespace std;

bool doChunk(PCFG *pcfg, Corpus *corpus);
double doMerge(PCFG *pcfg, Corpus *corpus, double currGain);
double getTotalCount(int rule, PCFG *pcfg);
void forceChunk(PCFG *pcfg, Corpus *corpus);

int main( int argc, const char* argv[] )
{

	if(argc != 2){
		cout << "Wrong arguments! Aborting!!" << endl;
		return -1;
	}
 	string filename(argv[1]);
 	
 	//Create parser
 	CYKparser cyk;
 	
	//Create corpus
	Corpus currCorp, buCorp;
	currCorp.loadCorpusFromFile(filename);
	buCorp = currCorp;
	//currCorp.printCorpus();
	//currCorp.calculateBiwordsCount();
	
	//Initialize PCFG
	PCFG dammyPCFG;
	dammyPCFG.initFirstNT(currCorp.numberOfSymbolsInCorpus());
	currCorp.initReduceForPCFG(&dammyPCFG);
	
	//currCorp.reduceCorpusForRule(dammyPCFG.allRules[3]);
	//currCorp.reduceCorpusForRule(dammyPCFG.allRules[4]);
	//currCorp.calculateBiwordsCount();
	currCorp.printCorpus(true);
	cout << "First try to reduce" << endl;
	bool continueChunk = true;
	double totalGain = 1.0;
	do{
		continueChunk = true;
		while(continueChunk){
			continueChunk=doChunk(&dammyPCFG, &currCorp);
			if(continueChunk == true){
				totalGain *= 2;
			}
		}
		totalGain = doMerge(&dammyPCFG, &currCorp, totalGain);
	}while(totalGain > 1.0);
	forceChunk(&dammyPCFG, &currCorp);
	Corpus::words allUniq = currCorp.getUniqueWords();
	
	for(Corpus::words::iterator iter = allUniq.begin(); iter!=allUniq.end(); iter++){
		if(iter->size() == 1){
			dammyPCFG.addRuleToStartRules(iter->at(0));
		}else{
			int newId = dammyPCFG.createNewNT(iter->at(0), iter->at(1), 1.0);
			dammyPCFG.addRuleToStartRules(newId);
		}
	}
	
	dammyPCFG.normalizeGrammar();
	dammyPCFG.printPCFG();
	
	currCorp.printCorpus(true);
	
	Corpus::words allInitWords = buCorp.getUniqueWords();
	vector<int> lala;
	lala.push_back(0);
	lala.push_back(0);
	lala.push_back(0);
	lala.push_back(1);
	lala.push_back(2);
	lala.push_back(2);
	allInitWords.push_back(lala);
	for(unsigned int w = 0; w<allInitWords.size(); w++){
		double parseProb = cyk.parseWord(&dammyPCFG, allInitWords[w]);
		cout << " Words: ";
		for(unsigned int i=0; i<allInitWords[w].size(); i++){
			cout << allInitWords[w][i];
		}
		cout << " | prob = " << parseProb << endl;
	}
	//doMerge(&dammyPCFG, &currCorp);
	//doMerge(&dammyPCFG, &currCorp);
	
	//doMerge(&dammyPCFG, &currCorp);
	//doMerge(&dammyPCFG, &currCorp);
	
	
	//double returnV = cyk.parseWord(&dammyPCFG, test);
	return 0;
}

bool doChunk(PCFG *pcfg, Corpus *corpus){
	bestChunk maxBiword = corpus->findMaxChunk();
	if(maxBiword.count > 1){
		int newRule = pcfg->createNewNT(maxBiword.leftSymbol, maxBiword.rightSymbol, maxBiword.prob);
		corpus->reduceCorpusForRule(pcfg->idToRule[newRule]);
		corpus->printCorpus(true);
		return true;
	}else{
		return false;
	}
}

void forceChunk(PCFG *pcfg, Corpus *corpus){
	bestChunk maxBiword;
	maxBiword = corpus->findMaxChunk();
	while(maxBiword.count > 0){
		int newRule = pcfg->createNewNT(maxBiword.leftSymbol, maxBiword.rightSymbol, maxBiword.prob);
		corpus->reduceCorpusForRule(pcfg->idToRule[newRule]);
		maxBiword = corpus->findMaxChunk();
	}
}

double doMerge(PCFG *pcfg, Corpus *corpus, double currGain){
	//return 0.0;
	corpus->calculateBiwordsCount();
	double max = 0.0;
	double maxI = -1;
	double maxJ = -1;

	for(unsigned int i=0; i<pcfg->allRules.size()-1; i++){
		//Calculate the total count for the first rule (sum all probabilities)
		double totalOfFirstRule = getTotalCount(i,pcfg);
		if(pcfg->allRules[i]->numberOfNTProductions() == 0)
			continue;
		for(unsigned int j=i+1; j<pcfg->allRules.size(); j++){
			if(pcfg->allRules[j]->numberOfNTProductions() == 0)
				continue;
			//Calculate the total count for the second rule (sum all probabilities)
			double totalOfSecondRule = getTotalCount(j,pcfg);
			
			float totalSum = totalOfFirstRule + totalOfSecondRule;
			double pOgivenG = 1.0;
			
			corpus->doBackup();
			int initGain = corpus->numberOfUniqueWords();
			corpus->replaceRules(pcfg->allRules[i],pcfg->allRules[j]);
			initGain -= corpus->numberOfUniqueWords();
			corpus->restoreBackup();
			initGain*=4;
			
			//This will be the value for p(G)
			double pG = pcfg->calculateDuplicates(pcfg->allRules[i]->id,pcfg->allRules[j]->id) * 2;
			//TODO what we do when the rule is the same?
			for(unsigned int iter = 0; iter<pcfg->allRules[i]->numberOfNTProductions(); iter++){
				double up = pcfg->allRules[i]->getNTProductionProbability(iter)/totalSum;
				double down = pcfg->allRules[i]->getNTProductionProbability(iter)/totalOfFirstRule;
				//pOgivenG *= pow(up/down, pcfg->allRules[i]->getNTProductionProbability(iter));
				pOgivenG *= up/down;
			}
			
			for(unsigned int iter = 0; iter<pcfg->allRules[j]->numberOfNTProductions(); iter++){
				double up = pcfg->allRules[j]->getNTProductionProbability(iter)/totalSum;
				double down = pcfg->allRules[j]->getNTProductionProbability(iter)/totalOfSecondRule;
				//pOgivenG *= pow(up/down, pcfg->allRules[j]->getNTProductionProbability(iter));
				pOgivenG *= up/down;
			}
			
			//cout << "i = " << i << " j = " << j << " P = " << pG*pOgivenG << " gain = " << pG*initGain << endl;
			//double max2 = totalOfFirstRule>totalOfSecondRule ? totalOfSecondRule : totalOfFirstRule;
			if(pG*initGain*pOgivenG > max){
				max = pG*initGain*pOgivenG;
				maxI = i;
				maxJ = j;
			}
		}
	}
	max *= currGain;
	if(max > 1.0){
		cout << max << " i = " << maxI << " j = " << maxJ << endl;
		corpus->replaceRules(pcfg->allRules[maxI], pcfg->allRules[maxJ]);
		pcfg->mergeTwoNT(pcfg->allRules[maxI]->id,pcfg->allRules[maxJ]->id);
		pcfg->printPCFG();
		corpus->printCorpus(true);
	}
	return max;
}

double getTotalCount(int rule, PCFG *pcfg){
	double result = 0.0;
	for(unsigned int iter = 0; iter<pcfg->allRules[rule]->numberOfNTProductions(); iter++){
		result += pcfg->allRules[rule]->getNTProductionProbability(iter);
	}
	for(unsigned int iter = 0; iter<pcfg->allRules[rule]->numberOfTermProductions(); iter++){
		result += pcfg->allRules[rule]->getTProductionProbability(iter);
	}
	return result;
}
