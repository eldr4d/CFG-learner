#ifndef CYKparser_HPP
#define CYKparser_HPP
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <map>

#include "../PCFG/PCFG.hpp"

class CYKparser{
public:
	double parseWord(PCFG *pcfg, std::vector<int> word){
		std::map<int, double> **productionTable;
		productionTable = new std::map<int,double>*[word.size()];
		for(unsigned int i=0; i<word.size(); i++){
			productionTable[i] = new std::map<int,double>[word.size()];
		}
		//return 0.0;
		
		for(unsigned int i=0; i<word.size(); i++){
			std::map<int, int>::iterator iter = pcfg->rulesForTermSymbol.find(word[i]);
			if(iter == pcfg->rulesForTermSymbol.end()){
				//Term symbol does not belong to the grammar
				return -1.0;
			}
			int rule = pcfg->locationOfRule(iter->second);
			for(unsigned int j=0; j<pcfg->allRules[rule].totalNumberOfProductions(); j++){
				Rule::productions prod = pcfg->allRules[rule].getProduction(j);
				if(prod.terminal == true && prod.rightRules[0] == word[i]){
					productionTable[i][0][iter->second] = prod.probability;
					break;
				}
			}
		}
		
		//Wikipedia algorithm
		//http://en.wikipedia.org/wiki/CYK_algorithm
		for(unsigned int j=1; j<word.size(); j++){
			for(unsigned int i=0; i<word.size()-j; i++){
				for(unsigned int k=0; k<j; k++){
					if(productionTable[i][k].size()==0 || productionTable[i+k+1][j-k-1].size()==0){
						continue;
					}
					for(unsigned iter = 0; iter < pcfg->allRules.size(); iter++){
						for(unsigned int iter2 = 0; iter2<pcfg->allRules[iter].totalNumberOfProductions(); iter2++){
							Rule::productions prod = pcfg->allRules[iter].getProduction(iter2);
							if(prod.terminal == true || prod.probability == 0.0f){
								continue;
							}else if(prod.rightRules.size() != 2){
								//Not in Chomsky Normal Form
								return -2;
							}
							std::map<int, double>::iterator leftMatched =  productionTable[i][k].find(prod.rightRules[0]);
							std::map<int, double>::iterator rightMatched = productionTable[i+k+1][j-k-1].find(prod.rightRules[1]);

							if(leftMatched != productionTable[i][k].end() && rightMatched != productionTable[i+k+1][j-k-1].end()){
								std::map<int, double>::iterator max = productionTable[i][j].find(pcfg->allRules[iter].id);
								if(max == productionTable[i][j].end()){
									productionTable[i][j][pcfg->allRules[iter].id] = prod.probability*leftMatched->second*rightMatched->second;
								}else{
									double temp = prod.probability*leftMatched->second*rightMatched->second;
									max->second = max->second > temp ? max->second : temp;
								}
							}
						}
					}
				}
			}
		}
		
		double finalValue = -4;
		for(std::map<int, double>::iterator iter = productionTable[0][word.size()-1].begin(); iter != productionTable[0][word.size()-1].end(); iter++){
			for(std::vector<int>::iterator found = pcfg->startRules.begin(); found != pcfg->startRules.end(); found++){
				if(*found == iter->first){
					finalValue = iter->second;
				}
			}
		}		
		
		for(unsigned int i=0; i<word.size(); i++){
			delete [] productionTable[i];
		}
		delete [] productionTable;
		
		return finalValue;
	}
};
#endif //CYKparser.hpp

