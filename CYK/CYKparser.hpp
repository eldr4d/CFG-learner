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
			std::cout << word[i] << " ";
		}
		std::cout << std::endl;
		//return 0.0;
		
		for(unsigned int i=0; i<word.size(); i++){
			std::map<int, Rule*>::iterator iter = pcfg->rulesForTermSymbol.find(word[i]);
			if(iter == pcfg->rulesForTermSymbol.end()){
				//Term symbol does not belong to the grammar
				return 0.0;
			}
			for(unsigned int j=0; j<iter->second->numberOfTermProductions(); j++){
				if(iter->second->getTerminalProduction(j) == word[i]){
					productionTable[i][0][iter->second->id] = iter->second->getTProductionProbability(j);
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
					for(std::vector<Rule*>::iterator iter = pcfg->allRules.begin(); iter != pcfg->allRules.end(); iter++){
						for(unsigned int iter2 = 0; iter2<(*iter)->numberOfNTProductions(); iter2++){
							std::map<int, double>::iterator leftMatched =  productionTable[i][k].find((*iter)->getLeftNTProduction(iter2)->id);
							std::map<int, double>::iterator rightMatched = productionTable[i+k+1][j-k-1].find((*iter)->getRightNTProduction(iter2)->id);
							if(leftMatched != productionTable[i][k].end() && rightMatched != productionTable[i+k+1][j-k-1].end()){
								std::map<int, double>::iterator max = productionTable[i][j].find((*iter)->id);
								if(max == productionTable[i][j].end()){
									productionTable[i][j][(*iter)->id] = (*iter)->getNTProductionProbability(iter2)*leftMatched->second*rightMatched->second;
								}else{
									double temp = (*iter)->getNTProductionProbability(iter2)*leftMatched->second*rightMatched->second;
									max->second = max->second > temp ? max->second : temp;
								}
							}
						}
					}
				}
			}
		}
		
		double finalValue = 0.0;
		for(std::map<int, double>::iterator iter = productionTable[0][word.size()-1].begin(); iter != productionTable[0][word.size()-1].end(); iter++){
			for(std::vector<Rule*>::iterator found = pcfg->startRules.begin(); found != pcfg->startRules.end(); found++){
				if((*found)->id == iter->first){
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

