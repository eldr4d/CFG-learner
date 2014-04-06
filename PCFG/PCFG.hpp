#ifndef PCFG_HPP
#define PCFG_HPP
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <map>

#include "Rule.hpp"

class PCFG{
public:
	std::vector<Rule *> startRules;
	std::vector<Rule *> allRules;
	std::map<int,Rule *> rulesForTermSymbol;
	std::map<int,Rule *> idToRule;
private:
	int currFreeId;
public:
	/*
	** Create NT symbols that point to the terminal symbols
	*/
	void initFirstNT(int symbols){
		cleanUp();
		currFreeId = symbols;
		for(int i=0; i<symbols; i++){
			Rule * rule = new Rule(currFreeId);
			rule->addTerminalProduction(i,1.0);
			allRules.push_back(rule);
			rulesForTermSymbol[i] = rule;
			idToRule[currFreeId] = rule;
			currFreeId++;
		}
	}
	
	/*
	** Merge two different NT given their ID. The first NT will remain in the grammar
	** with the addition of the rules that the second NT has. If there is the same rule
	** in both NT's we will increase its likelihood. The second NT will be deleted!
	** Every occurance of the second NT in the grammar will be replaced with the first one!
	*/
	double mergeTwoNT(int NT1id, int NT2id){
		//printPCFG();
		double totalGain = 1.0;
		Rule* host = idToRule[NT1id];
		Rule* target = idToRule[NT2id];
		//Remove duplicate rules
		totalGain *= mergeSameProductions(host,target);
		//Merge the target with the host rule
		for(unsigned int iter = 0; iter<target->numberOfNTProductions(); iter++){
			host->addNonTerminalProduction(target->getLeftNTProduction(iter), target->getRightNTProduction(iter), target->getNTProductionProbability(iter));
		}
		for(unsigned int iter = 0; iter<target->numberOfTermProductions(); iter++){
			host->addTerminalProduction(target->getTerminalProduction(iter), target->getTProductionProbability(iter));
			rulesForTermSymbol[target->getTerminalProduction(iter)] = host;
		}

		//Replace every occurance of the target rule with the host rule
		for(unsigned int iter1 = 0; iter1<allRules.size(); iter1++){
			for(unsigned int iter2 = 0; iter2<allRules[iter1]->numberOfNTProductions(); iter2++){
				if(allRules[iter1]->getLeftNTProduction(iter2)->id == target->id){
					allRules[iter1]->replaceNTInProduction(iter2, host, true);
				}
				if(allRules[iter1]->getRightNTProduction(iter2)->id == target->id){
					allRules[iter1]->replaceNTInProduction(iter2, host, false);
				}
			}
		}
		totalGain *= mergeDuplicates(host);
		//Delete target
		std::map<int,Rule *>::iterator it = idToRule.find(target->id);
		idToRule.erase(it);
		for(unsigned int iter1 = 0; iter1<allRules.size(); iter1++){
			if(allRules[iter1]->id == target->id){
				allRules.erase(allRules.begin()+iter1);
				break;
			}
		}
		delete target;
		return totalGain;
	}
	
	
	
	/*
	** Find if two rules that will be merged have the same rule, if true update the host NT.
	*/
	double calculateDuplicates(int NT1id, int NT2id){
		Rule* host = idToRule[NT1id];
		Rule* target = idToRule[NT2id];
		double totalGain = 1.0;
		for(unsigned int iter1 = 0; iter1<host->numberOfNTProductions(); iter1++){
			for(unsigned int iter2 = 0; iter2<target->numberOfNTProductions(); iter2++){
				if(host->getLeftNTProduction(iter1)->id == target->getLeftNTProduction(iter2)->id && host->getRightNTProduction(iter1)->id == target->getRightNTProduction(iter2)->id){
					totalGain *= 4;
					break;
				}
			}
		}
		return totalGain;
	}

	/*
	** Create new Chomsky NT
	*/
	int createNewNT(int leftID, int rightID, float prob){
		Rule * rule = new Rule(currFreeId);
		idToRule[currFreeId] = rule;
		currFreeId++;
		rule->addNonTerminalProduction(idToRule[leftID],idToRule[rightID],prob);
		allRules.push_back(rule);
		return rule->id;
	}
	
	void addRuleToStartRules(int id){
		Rule* host = idToRule[id];
		for(std::vector<Rule *>::iterator iter = startRules.begin(); iter != startRules.end(); iter++){
			if((*iter)->id == id){
				return;
			}
		}
		startRules.push_back(host);
	}
	
	/*
	** Pretty print the PCFG
	*/
	void printPCFG(){
		std::cout << "Start Rules: " << std::endl;
		for(unsigned int i=0; i<startRules.size(); i++){
			std::cout << "\tS" << i << " = " << "N" << startRules[i]->id << std::endl;
		}
		std::cout << "All Rules: " << std::endl;
		for(unsigned int i=0; i<allRules.size(); i++){
			std::cout << "\tN" << allRules[i]->id;
			for(unsigned int j=0; j<allRules[i]->numberOfNTProductions(); j++){
				std::cout << "\t-> " << "N" << allRules[i]->getLeftNTProduction(j)->id << " N" << allRules[i]->getRightNTProduction(j)->id << " (" << allRules[i]->getNTProductionProbability(j) << ")" << std::endl;
				std::cout << "\t";
			}
			for(unsigned int j=0; j<allRules[i]->numberOfTermProductions(); j++){
				std::cout << "\t-> " << allRules[i]->getTerminalProduction(j) << " (" << allRules[i]->getTProductionProbability(j) << ")" << std::endl;
				std::cout << "\t";
			}
			std::cout << std::endl;
		}
	}
	
	/*
	** Normalize the probabiltiies of each production
	*/
	void normalizeGrammar(){
		for(std::vector<Rule *>::iterator iter = allRules.begin(); iter != allRules.end(); iter++){
			float sumOfAllProb = 0.0;
			//Find total sum
			for(unsigned int i=0; i<(*iter)->numberOfNTProductions(); i++){
				sumOfAllProb += (*iter)->getNTProductionProbability(i);
			}
			for(unsigned int i=0; i<(*iter)->numberOfTermProductions(); i++){
				sumOfAllProb += (*iter)->getTProductionProbability(i);
			}
			//Normalize
			for(unsigned int i=0; i<(*iter)->numberOfNTProductions(); i++){
				(*iter)->updateProbability(i, (*iter)->getNTProductionProbability(i)/sumOfAllProb, false);
			}
			for(unsigned int i=0; i<(*iter)->numberOfTermProductions(); i++){
				(*iter)->updateProbability(i, (*iter)->getTProductionProbability(i)/sumOfAllProb, true);
			}
			
		}
	}
	
	//Create a^n c b^n where n > 0
	void createTestPCFG(){
		cleanUp();
		Rule * S = new Rule(0);
		Rule * A = new Rule(1);
		Rule * B = new Rule(2);
		Rule * C = new Rule(3);
		Rule * T = new Rule(4);
		rulesForTermSymbol[0] = A;
		rulesForTermSymbol[1] = C;
		rulesForTermSymbol[2] = B;
		idToRule[0] = S;
		idToRule[1] = A;
		idToRule[2] = B;
		idToRule[3] = C;
		idToRule[4] = T;
		S->addNonTerminalProduction(A,T,1.0);
		T->addNonTerminalProduction(C,B,0.1);
		T->addNonTerminalProduction(S,B,0.9);
		A->addTerminalProduction(0,1.0);
		C->addTerminalProduction(1,1.0);
		B->addTerminalProduction(2,1.0);
		allRules.push_back(S);
		startRules.push_back(S);
		allRules.push_back(A);
		allRules.push_back(B);
		allRules.push_back(C);
		allRules.push_back(T);
		currFreeId = 5;
	}
	
	/*
	** Reinitialzie the PCFG
	*/
	void cleanUp(){
		for(std::vector<Rule *>::iterator iter = allRules.begin(); iter!=allRules.end(); iter++){
			delete *iter;
		}
		startRules.clear();
		allRules.clear();
		rulesForTermSymbol.clear();
		idToRule.clear();
		currFreeId = 0;
	}
	
private:
	/*
	** Find if the same production exists more than one time in a given Rule
	*/
	double mergeDuplicates(Rule* host){
		double totalGain = 1.0;
		std::vector<bool> toDelete(host->numberOfNTProductions(),false);
		for(unsigned int iter1 = 0; iter1<host->numberOfNTProductions(); iter1++){
			if(toDelete[iter1]){
				continue;
			}
			for(unsigned int iter2 = iter1+1; iter2<host->numberOfNTProductions(); iter2++){
				if(!toDelete[iter2] && host->getLeftNTProduction(iter1)->id == host->getLeftNTProduction(iter2)->id && host->getRightNTProduction(iter1)->id == host->getRightNTProduction(iter2)->id){
					toDelete[iter2] = true;
					float newProbability = host->getNTProductionProbability(iter1) + host->getNTProductionProbability(iter2);
					host->updateProbability(iter1, newProbability, false);
					totalGain *= 4;
					break;
				}
			}
		}
		for(int i=toDelete.size()-1; i>=0; i--){
			if(toDelete[i]){
				host->removeProduction(i, false);
			}
		}
		return totalGain;
	}
	
	/*
	** Find if two rules that will be merged have the same production, if true update the host NT.
	*/
	double mergeSameProductions(Rule* host, Rule* target){
		double totalGain = 1.0;
		for(unsigned int iter1 = 0; iter1<host->numberOfNTProductions(); iter1++){
			for(unsigned int iter2 = 0; iter2<target->numberOfNTProductions(); iter2++){
				if(host->getLeftNTProduction(iter1)->id == target->getLeftNTProduction(iter2)->id && host->getRightNTProduction(iter1)->id == target->getRightNTProduction(iter2)->id){
					float newProbability = host->getNTProductionProbability(iter1) + target->getNTProductionProbability(iter2);
					host->updateProbability(iter1, newProbability, false);
					target->removeProduction(iter2,false);
					totalGain *= 4;
					break;
				}
			}
		}
		return totalGain;
	}
};
#endif //PCFG_HPP

