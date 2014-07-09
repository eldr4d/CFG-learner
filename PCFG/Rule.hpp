#ifndef RULE_HPP
#define RULE_HPP
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <vector>

class Rule{
public:
typedef struct conections{
	std::vector<int> rightRules;
	float probability;
	bool terminal;
}productions;
public:
	int id;
	bool hasNTproduction;
private:
	std::vector<productions> allProductions;
public:
	Rule(int id):id(id),hasNTproduction(false){}
	
	/*
	** Add NT production to the rule
	*/
	void addProduction(std::vector<int> rightPart, float probability, bool terminal){
		productions newProd;
		newProd.rightRules = rightPart;
		newProd.probability = probability;
		newProd.terminal = terminal;
		allProductions.push_back(newProd);
		if(!terminal){
			hasNTproduction = true;
		}
	}
	
	/*
	** Append production to the rule
	*/
	void appendProduction(productions prod){
		allProductions.push_back(prod);
	}
	
	/*
	** Update probability of given production
	*/
	void updateProbability(int index, float probability){
		allProductions[index].probability = probability;
	}

	/*
	** Get probability of a rule
	*/
	float getProbability(int index){
		return allProductions[index].probability;
	}
	
	/*
	 ** Get left NT of given production
	 */
	productions getProduction(int index){
		return allProductions[index];
	}
	
	/*
	 ** Get left NT of given production
	 */
	void updateProductions(productions prod,int index){
		allProductions[index] = prod;
	}
	
	/*
	** Number of terminal productions
	*/
	unsigned int totalNumberOfProductions(){
		return allProductions.size();
	}
	
	/*
	**
	*/
	std::vector<int> randomProduction(float randomNumber){
		float cumSum = 0;
		unsigned int i;
		for(i=0; i<allProductions.size(); i++){
			cumSum += allProductions[i].probability;
			if(cumSum >= randomNumber){
				break;
			}
		}
		return allProductions[i].rightRules;
	}
	
	/*
	** Remove production
	*/
	void removeProduction(int index){
		allProductions.erase(allProductions.begin()+index);
	}
	
	/*
	** Find if a given rule is in my productions
	*/
	bool ruleInAnyProduction(int ruleID){
		bool found = false;
		for(unsigned int i=0; i<allProductions.size(); i++){
			for(unsigned int j; j<allProductions[i].rightRules.size(); j++){
				if(allProductions[i].rightRules[j] == ruleID){
					found = true;
					break;
				}
			}
			if(found){
				break;
			}
		}
		return found;
	}
	
};
#endif //RULE_HPP
