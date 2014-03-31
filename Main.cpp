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
double doMerge(PCFG *pcfg, Corpus *corpus);
double getTotalCount(int rule, PCFG *pcfg);

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
	Corpus currCorp;
	currCorp.loadCorpusFromFile(filename);
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
		while(continueChunk){
			continueChunk=doChunk(&dammyPCFG, &currCorp);
			
		
			if(continueChunk == true){
				totalGain *= 2;
			}
		}
		totalGain *= doMerge(&dammyPCFG, &currCorp);
	}while(totalGain > 1.0);
	dammyPCFG.printPCFG();
	currCorp.printCorpus(true);
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
		return true;
	}else{
		return false;
	}
}

double doMerge(PCFG *pcfg, Corpus *corpus){
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
			
			//This will be the value for p(G)
			double pG = pcfg->calculateDuplicates(pcfg->allRules[i],pcfg->allRules[j]) * 2;
			//TODO what we do when the rule is the same?
			for(unsigned int iter = 0; iter<pcfg->allRules[i]->numberOfNTProductions(); iter++){
				double up = pcfg->allRules[i]->getNTProductionProbability(iter)/totalSum;
				double down = pcfg->allRules[i]->getNTProductionProbability(iter)/totalOfFirstRule;
				pOgivenG *= pow(up/down, pcfg->allRules[i]->getNTProductionProbability(iter));
			}
			
			for(unsigned int iter = 0; iter<pcfg->allRules[j]->numberOfNTProductions(); iter++){
				double up = pcfg->allRules[j]->getNTProductionProbability(iter)/totalSum;
				double down = pcfg->allRules[j]->getNTProductionProbability(iter)/totalOfSecondRule;
				pOgivenG *= pow(up/down, pcfg->allRules[j]->getNTProductionProbability(iter));
			}
			
			//cout << "i = " << i << " j = " << j << " P = " << pG*pOgivenG << endl;
			//double max2 = totalOfFirstRule>totalOfSecondRule ? totalOfSecondRule : totalOfFirstRule;
			if(pG*pOgivenG > max){
				max = pG*pOgivenG;
				maxI = i;
				maxJ = j;
			}
		}
	}
	if(max > 0.0){
		cout << max << " i = " << maxI << " j = " << maxJ << endl;
		pcfg->mergeTwoNT(pcfg->allRules[maxI]->id,pcfg->allRules[maxJ]->id);
		pcfg->printPCFG();
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
