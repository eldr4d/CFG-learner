#ifndef PCFG_HPP
#define PCFG_HPP
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <map>

#include "Rule.hpp"

class PCFG{
public:
	//Keep the id of the Start Rules
	std::vector<int> startRules;
	//Keep all the rules
	std::vector<Rule> allRules;
	//Map term symbol to rule for fast access
	std::map<int,int> rulesForTermSymbol;
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
			Rule rule(currFreeId);
			rule.addTerminalProduction(i,1.0);
			allRules.push_back(rule);
			rulesForTermSymbol[i] = rule.id;
			currFreeId++;
		}
	}
	
	/*
	** Merge two different NT given their ID. The first NT will remain in the grammar
	** with the addition of the rules that the second NT has. If there is the same rule
	** in both NT's we will increase its likelihood. The second NT will be deleted!
	** Every occurance of the second NT in the grammar will be replaced with the first one!
	*/
	double mergeTwoNT(int hostID, int targetID){
		int host = locationOfRule(hostID);
		int target = locationOfRule(targetID);
		double totalGain = 1.0;
		//Remove duplicate rules
		totalGain *= mergeSameProductions(hostID,targetID);
		//Merge the target with the host rule
		for(unsigned int iter = 0; iter<allRules[target].totalNumberOfProductions(); iter++){
			Rule::productions toTransfer = allRules[target].getProduction(iter);
			if(toTransfer.terminal){
				allRules[host].addTerminalProduction(toTransfer.termSymbol, toTransfer.probability);
				rulesForTermSymbol[toTransfer.termSymbol] = hostID;
			}else{
				allRules[host].addNonTerminalProduction(toTransfer.leftID, toTransfer.rightID, toTransfer.probability);
			}
		}
		
		//Replace every occurance of the target rule with the host rule
		for(unsigned int iter1 = 0; iter1<allRules.size(); iter1++){
			for(unsigned int iter2 = 0; iter2<allRules[iter1].totalNumberOfProductions(); iter2++){
				Rule::productions currProd = allRules[iter1].getProduction(iter2);
				if(currProd.leftID == allRules[target].id){
					allRules[iter1].replaceNTInProduction(iter2, allRules[host].id, true);
				}
				if(currProd.rightID == allRules[target].id){
					allRules[iter1].replaceNTInProduction(iter2, allRules[host].id, false);
				}
			}
		}
		allRules.erase(allRules.begin()+target);
		for(unsigned int iter=0; iter<allRules.size(); iter++){
			totalGain *= mergeDuplicatesProductionsInSameRule(allRules[iter].id);
		}
		//Delete target

		return totalGain;
	}
	
	
	
	/*
	** Find if two rules that will be merged have the same rule, if true update the host NT.
	*/
	double calculateDuplicates(int hostID, int targetID){
		int host = locationOfRule(hostID);
		int target = locationOfRule(targetID);
		double totalGain = 1.0;
		for(unsigned int iter1 = 0; iter1<allRules[host].totalNumberOfProductions(); iter1++){
			Rule::productions hostProd = allRules[host].getProduction(iter1);
			for(unsigned int iter2 = 0; iter2<allRules[target].totalNumberOfProductions(); iter2++){
				Rule::productions targetProd = allRules[host].getProduction(iter2);
				if(hostProd.leftID == targetProd.leftID && hostProd.rightID == targetProd.rightID){
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
		Rule rule(currFreeId);
		currFreeId++;
		
		rule.addNonTerminalProduction(leftID,rightID,prob);
		allRules.push_back(rule);
		return rule.id;
	}
	
	/*
	** Insert to the start rule set a new one
	*/
	void addRuleToStartRules(int id){
		startRules.push_back(id);
	}
	
	/*
	** Return the index of rule for given ID
	*/
	unsigned int locationOfRule(int ruleID){
		unsigned int found = 0;
		for(unsigned int i = 0; i<allRules.size(); i++){
			if(allRules[i].id == ruleID){
				found = i;
				break;
			}
		}
		return found;
	}
	
	/*
	** Pretty print the PCFG
	*/
	void prettyPrint(){
		std::cout << "Start Rules: " << std::endl;
		for(unsigned int i=0; i<startRules.size(); i++){
			std::cout << "N" << startRules[i] << std::endl;
		}
		std::cout << "All Rules: " << std::endl;
		for(unsigned int i=0; i<allRules.size(); i++){
			std::cout << "\tN" << allRules[i].id;
			for(unsigned int j=0; j<allRules[i].totalNumberOfProductions(); j++){
				Rule::productions tmp = allRules[i].getProduction(j);
				if(tmp.terminal == false){
					std::cout << "\t-> " << "N" << tmp.leftID << " N" << tmp.rightID << " (" << tmp.probability << ")" << std::endl;
				}else{
					std::cout << "\t-> " << tmp.termSymbol << " (" << tmp.probability << ")" << std::endl;
				}
				std::cout << "\t";
			}
			std::cout << std::endl;
		}
	}
	
	/*
	** Normalize the probabiltiies of each production
	*/
	void normalizeGrammar(){
		for(std::vector<Rule>::iterator iter = allRules.begin(); iter != allRules.end(); iter++){
			float sumOfAllProb = 0.0;
			//Find total sum
			for(unsigned int i=0; i<iter->totalNumberOfProductions(); i++){
				sumOfAllProb += iter->getProduction(i).probability;
			}
			//Normalize
			for(unsigned int i=0; i<iter->totalNumberOfProductions(); i++){
				iter->updateProbability(i, iter->getProduction(i).probability/sumOfAllProb);
			}			
		}
	}
	
	//Create a^n c b^n where n > 0
	void createTestPCFG(){
		cleanUp();
		Rule S(0);
		Rule A(1);
		Rule B(2);
		Rule C(3);
		Rule T(4);
		rulesForTermSymbol[0] = A.id;
		rulesForTermSymbol[1] = C.id;
		rulesForTermSymbol[2] = B.id;
		
		S.addNonTerminalProduction(A.id,T.id,1.0);
		T.addNonTerminalProduction(C.id,B.id,0.1);
		T.addNonTerminalProduction(S.id,B.id,0.9);
		A.addTerminalProduction(0,1.0);
		C.addTerminalProduction(1,1.0);
		B.addTerminalProduction(2,1.0);
		allRules.push_back(S);
		startRules.push_back(S.id);
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
		startRules.clear();
		allRules.clear();
		rulesForTermSymbol.clear();
		currFreeId = 0;
	}
	
	/*
	** Find and delete rules that are no longer accesible
	*/
	double deleteUselessRules(){
		std::vector<bool> toDelete(allRules.size());
		int totalDeletions = 0;
		for(unsigned int i = 0; i<allRules.size(); i++){
			int idToCheck = allRules[i].id;
			toDelete[i] = true;
			for(unsigned int j=0; j<allRules.size(); j++){
				if(allRules[j].ruleInAnyProduction(idToCheck)){
					toDelete[i] = false;
					totalDeletions += 2 + allRules[i].numberOfNTProductions()*4 + allRules[i].numberOfTermProductions()*2;
					break;
				}
			}
		}
		for(int i=toDelete.size()-1; i>=0; i--){
			if(toDelete[i]){
				allRules.erase(allRules.begin()+i);
			}
		}
		return totalDeletions;
	}
	
	
private:
	/*
	** Find if the same production exists more than one time in a given Rule
	*/
	double mergeDuplicatesProductionsInSameRule(int hostID){
		int host = locationOfRule(hostID);
		double totalGain = 1.0;
		std::vector<bool> toDelete(allRules[host].totalNumberOfProductions(),false);
		for(unsigned int iter1 = 0; iter1<allRules[host].totalNumberOfProductions(); iter1++){
			if(toDelete[iter1]){
				continue;
			}
			Rule::productions firstProd = allRules[host].getProduction(iter1);
			for(unsigned int iter2 = iter1+1; iter2<allRules[host].totalNumberOfProductions(); iter2++){
				Rule::productions secProd = allRules[host].getProduction(iter2);
				if(!toDelete[iter2] && firstProd.leftID == secProd.leftID && firstProd.rightID == secProd.rightID){
					toDelete[iter2] = true;
					float newProbability = firstProd.probability + secProd.probability;
					allRules[host].updateProbability(iter1, newProbability);
					totalGain *= 4;
					break;
				}
			}
		}
		for(int i=toDelete.size()-1; i>=0; i--){
			if(toDelete[i]){
				allRules[host].removeProduction(i);
			}
		}
		return totalGain;
	}
	
	/*
	** Find if two rules that will be merged have the same production, if true update the host NT.
	*/
	double mergeSameProductions(int hostID, int targetID){
		int host = locationOfRule(hostID);
		int target = locationOfRule(targetID);
		double totalGain = 1.0;
		for(unsigned int iter1 = 0; iter1<allRules[host].totalNumberOfProductions(); iter1++){
			Rule::productions hostProd = allRules[host].getProduction(iter1);
			for(unsigned int iter2 = 0; iter2<allRules[target].totalNumberOfProductions(); iter2++){
				Rule::productions targetProd = allRules[target].getProduction(iter2);
				if(hostProd.leftID == targetProd.leftID && hostProd.rightID == targetProd.rightID){
					float newProbability = hostProd.probability + targetProd.probability;
					allRules[host].updateProbability(iter1, newProbability);
					allRules[target].removeProduction(iter2);
					totalGain *= 4;
					break;
				}
			}
		}
		return totalGain;
	}
	
};
#endif //PCFG_HPP

