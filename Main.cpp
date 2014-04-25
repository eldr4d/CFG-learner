#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <math.h>
#include <unistd.h>


#include "Corpus/Corpus.hpp"
#include "PCFG/PCFG.hpp"
#include "CYK/CYKparser.hpp"
using namespace std;

typedef struct{
	PCFG pcfg;
	Corpus corpus;
	double currGain;
	bool isChunk;
	bool conflict;
}searchNode;
bool doChunk(PCFG *pcfg, Corpus *corpus);
void doMerge(vector<searchNode> *listOfNodes, searchNode parent);
double getTotalCount(int rule, PCFG *pcfg);
void forceChunk(PCFG *pcfg, Corpus *corpus);
void searchForBestPCFG(Corpus corpus);
vector<searchNode> findAllPossibleMoves(searchNode parent);

int main( int argc, const char* argv[] )
{
	
	/*char cCurrentPath[FILENAME_MAX];
	 if (!getcwd(cCurrentPath, sizeof(cCurrentPath)))
	 {
	 return errno;
	 }
	 cCurrentPath[sizeof(cCurrentPath) - 1] = '\0';
	 cout << cCurrentPath << endl;*/
	
	if(argc != 2){
		cout << "Wrong arguments! Aborting!!" << endl;
		return -1;
	}
	string filename ="../../../../../brackets.txt";
 	//string filename = string("../../../../../") + argv[1];
 	//Create parser
 	CYKparser cyk;
 	
	//Create corpus
	Corpus currCorp;
	currCorp.loadCorpusFromFile(filename);
	currCorp.printCorpus();
	cout << " --------------------Begin---------------------- " << endl;
	Corpus::words allInitWords = currCorp.getUniqueWords();
	searchForBestPCFG(currCorp);
	/*vector<int> lala;
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
	 }*/
	
	//double returnV = cyk.parseWord(&dammyPCFG, test);
	return 0;
}

void searchForBestPCFG(Corpus corpus){
	//Initialize PCFG
	searchNode parent;
	parent.corpus = corpus;
	parent.pcfg.initFirstNT(parent.corpus.numberOfSymbolsInCorpus());
	parent.corpus.initReduceForPCFG(&parent.pcfg);
	
	for(unsigned int doIter = 0; doIter < 1000; doIter++){
		vector<searchNode> possiblePCFGS = findAllPossibleMoves(parent);
		vector<double> bestValues(possiblePCFGS.size());
		for(int i=0; i<possiblePCFGS.size();i++){
			vector<searchNode> possiblePCFGS2 = findAllPossibleMoves(possiblePCFGS[i]);
			vector<double> bestValues2(possiblePCFGS2.size());
			for(int k=0; k<possiblePCFGS2.size();k++){
				vector<searchNode> possiblePCFGS3 = findAllPossibleMoves(possiblePCFGS2[k]);
				double best = -10000;
				for(int j=0; j<possiblePCFGS3.size();j++){
					if(possiblePCFGS3[j].currGain > best && !possiblePCFGS3[j].conflict){
						best = possiblePCFGS3[j].currGain;
					}
				}
				bestValues2[k] = best;
			}
			double best = -10000;
			for(int k=0;k<possiblePCFGS2.size();k++){
				if(bestValues2[k]>best && !possiblePCFGS2[k].conflict){
					best=bestValues2[k];
				}
			}
			bestValues[i]=best;
		}
		//Find the best successor
		searchNode best;
		best.currGain = -1000000.0;
		double currBest = -100000.0;
		for(unsigned int i=0; i < possiblePCFGS.size(); i++){
			/*cout << "Is Chunk = " << possiblePCFGS[i].isChunk << " Gain = " << possiblePCFGS[i].currGain << endl;
			 if((possiblePCFGS[i].currGain > best.currGain) && !(best.isChunk && parent.isChunk)){
			 best = possiblePCFGS[i];
			 }else if((possiblePCFGS[i].currGain > best.currGain) && possiblePCFGS[i].conflict == false){
			 best = possiblePCFGS[i];
			 }else if(best.isChunk == true && best.conflict == true && possiblePCFGS[i].isChunk == true && possiblePCFGS[i].conflict == false){
			 best = possiblePCFGS[i];
			 }*/
			cout << "Is Chunk = " << possiblePCFGS[i].isChunk << " Gain = " << bestValues[i] << " Conflict = " << possiblePCFGS[i].conflict << endl;
			if((bestValues[i] > currBest) && !(best.isChunk && parent.isChunk)){
				best = possiblePCFGS[i];
				currBest = bestValues[i];
			}else if((bestValues[i] > currBest) && possiblePCFGS[i].conflict == false){
				best = possiblePCFGS[i];
				currBest = bestValues[i];
			}else if(best.isChunk == true && best.conflict == true && possiblePCFGS[i].isChunk == true && possiblePCFGS[i].conflict == false){
				best = possiblePCFGS[i];
				currBest = bestValues[i];
			}
			
		}
		parent = best;
		cout << "Best = " << parent.currGain << endl;
		best.pcfg.prettyPrint();
		best.corpus.printCorpus();
	}
	//parent.pcfg.prettyPrint();
	//parent.corpus.printCorpus();
	
	//At the end
	//forceChunk(&dammyPCFG, &currCorp);
	
	Corpus::words allUniq = parent.corpus.getUniqueWords();
	
	for(Corpus::words::iterator iter = allUniq.begin(); iter!=allUniq.end(); iter++){
		if(iter->size() == 1){
			parent.pcfg.addRuleToStartRules(iter->at(0));
		}else{
			int newId = parent.pcfg.createNewNT(iter->at(0), iter->at(1), 1.0);
			parent.pcfg.addRuleToStartRules(newId);
		}
	}
	
	parent.pcfg.normalizeGrammar();
	parent.pcfg.prettyPrint();
}

vector<searchNode> findAllPossibleMoves(searchNode parent){
	vector<searchNode> possiblePCFGS;
	bool isAtLeastOneChunkWithoutConflict = false;
	//Do all Chunks
	Corpus::allChunks chunks= parent.corpus.calculateBiwordsCount();
	for(map<int, int>::iterator iter = chunks.biwordsCount.begin(); iter!=chunks.biwordsCount.end(); iter++){
		searchNode newNode = parent;
		int newRule = newNode.pcfg.createNewNT(iter->first/Corpus::keyAdjust, iter->first%Corpus::keyAdjust, chunks.biwordsProb[iter->first]);
		int timesFound = newNode.corpus.reduceCorpusForRule(newNode.pcfg.allRules[newNode.pcfg.locationOfRule(newRule)]);
		int costToCreateRule = pow(2,3);
		double totalGain = log10(pow(2,2*timesFound)/costToCreateRule);
		if(timesFound <= 1){
			continue;
		}
		newNode.currGain += totalGain;
		newNode.isChunk = true;
		newNode.conflict = chunks.biwordsConflict[iter->first];
		if(!newNode.conflict){
			isAtLeastOneChunkWithoutConflict = true;
		}
		possiblePCFGS.push_back(newNode);
	}
	//Do all merges
	doMerge(&possiblePCFGS,parent);
	return possiblePCFGS;
}

bool doChunk(PCFG *pcfg, Corpus *corpus){
	return false;
}

void forceChunk(PCFG *pcfg, Corpus *corpus){
	/*bestChunk maxBiword;
	 maxBiword = corpus->findMaxChunk();
	 while(maxBiword.count > 0){
	 int newRule = pcfg->createNewNT(maxBiword.leftSymbol, maxBiword.rightSymbol, maxBiword.prob);
	 corpus->reduceCorpusForRule(pcfg->allRules[pcfg->locationOfRule(newRule)]);
	 maxBiword = corpus->findMaxChunk();
	 }*/
}

void doMerge(vector<searchNode> *listOfNodes, searchNode parent){
	
	for(unsigned int i=0; i<parent.pcfg.allRules.size()-1; i++){
		//Calculate the total count for the first rule (sum all probabilities)
		double totalOfFirstRule = getTotalCount(parent.pcfg.allRules[i].id,&(parent.pcfg));
		if(parent.pcfg.allRules[i].numberOfNTProductions() == 0)
			continue;
		for(unsigned int j=i+1; j<parent.pcfg.allRules.size(); j++){
			if(parent.pcfg.allRules[j].numberOfNTProductions() == 0)
				continue;
			
			searchNode newNode = parent;
			newNode.isChunk = false;
			newNode.conflict = false;
			//Calculate the total count for the second rule (sum all probabilities)
			double totalOfSecondRule = getTotalCount(parent.pcfg.allRules[j].id,&parent.pcfg);
			
			//Calculate the change in the likelihood
			float totalSum = totalOfFirstRule + totalOfSecondRule;
			double pOgivenG = 0.0;
			for(unsigned int iter = 0; iter<parent.pcfg.allRules[i].totalNumberOfProductions(); iter++){
				double up = parent.pcfg.allRules[i].getProduction(iter).probability/totalSum;
				double down = parent.pcfg.allRules[i].getProduction(iter).probability/totalOfFirstRule;
				pOgivenG += parent.pcfg.allRules[i].getProduction(iter).probability*log10(up/down);
			}
			
			for(unsigned int iter = 0; iter<parent.pcfg.allRules[j].totalNumberOfProductions(); iter++){
				double up = parent.pcfg.allRules[j].getProduction(iter).probability/totalSum;
				double down = parent.pcfg.allRules[j].getProduction(iter).probability/totalOfSecondRule;
				pOgivenG += parent.pcfg.allRules[j].getProduction(iter).probability*log10(up/down);
			}
			//Gain from mergin same words in corpus
			int initGain = parent.corpus.numberOfUniqueWords();
			newNode.corpus.replaceRules(parent.pcfg.allRules[i].id, parent.pcfg.allRules[j].id);
			initGain -= newNode.corpus.numberOfUniqueWords();
			initGain = initGain == 0 ? 0.0 : initGain*log10(2);
			
			double extraGainFromMerge = newNode.pcfg.mergeTwoNT(parent.pcfg.allRules[i].id,parent.pcfg.allRules[j].id);
			//double gainFromUselessRules = log10(newNode.pcfg.deleteUselessRules());
			
			newNode.currGain += log10(2) + pOgivenG + initGain + log10(extraGainFromMerge);
			//newNode.corpus.reduceCorpusForRule(newNode.pcfg.allRules[newNode.pcfg.locationOfRule(idToMerge)]);
			
			//if(parent.pcfg.allRules.size() == 6 && i == 3 && j == 5){
			//	newNode.pcfg.prettyPrint();
			//}
			//corpus->reduceCorpusForRule(pcfg->allRules[pcfg->locationOfRule(idToMerge)]);
			//pcfg->prettyPrint();
			//corpus->printCorpus();
			listOfNodes->push_back(newNode);
		}
	}
}


double getTotalCount(int ruleID, PCFG *pcfg){
	double result = 0.0;
	int ruleLoc = pcfg->locationOfRule(ruleID);
	for(unsigned int iter = 0; iter<pcfg->allRules[ruleLoc].totalNumberOfProductions(); iter++){
		result += pcfg->allRules[ruleLoc].getProduction(iter).probability;
	}
	return result;
}
