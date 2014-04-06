#ifndef RULE_HPP
#define RULE_HPP
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <vector>

class Rule{
public:
typedef struct conections{
	bool terminal;
	int termSymbol;
	int leftID;
	int rightID;
	float probability;
}productions;
public:
	int id;
private:
	std::vector<productions> allProductions;
	int terminalProductions;
	int nonTerminalProductions;
public:
	Rule(int id):id(id),terminalProductions(0),nonTerminalProductions(0){}
	
	/*
	** Add NT production to the rule
	*/
	void addNonTerminalProduction(int leftID, int rightID, float probability){
		productions newProd;
		newProd.terminal = false;
		newProd.leftID = leftID;
		newProd.rightID = rightID;
		newProd.probability = probability;
		allProductions.push_back(newProd);
		nonTerminalProductions++;
	}
	
	/*
	** Update probability of given production
	*/
	void updateProbability(int index, float probability){
		allProductions[index].probability = probability;
	}
	
	/*
	** Add terminal production
	*/
	void addTerminalProduction(int symbol, float probability){
		productions newProd;
		newProd.terminal = true;
		newProd.termSymbol = symbol;
		newProd.probability = probability;
		allProductions.push_back(newProd);
	}
	
	/*
	** Replace a rule of a given NT production
	*/
	void replaceNTInProduction(int index, int newRuleID, bool left){
		if(left){
			allProductions[index].leftID = newRuleID;
		}else{
			allProductions[index].rightID = newRuleID;
		}
	}
	
	/*
	** Get left NT of given production
	*/
	productions getProduction(int index){
		return allProductions[index];
	}
	
	/*
	** Number of NT productions
	*/
	int numberOfNTProductions(){
		return nonTerminalProductions;
	}
	
	/*
	** Number of terminal productions
	*/
	int numberOfTermProductions(){
		return terminalProductions;
	}
	
	/*
	** Number of terminal productions
	*/
	unsigned int totalNumberOfProductions(){
		return allProductions.size();
	}
	
	/*
	** Remove production
	*/
	void removeProduction(int index){
		if(allProductions[index].terminal){
			terminalProductions--;
		}else{
			nonTerminalProductions--;
		}
		allProductions.erase(allProductions.begin()+index);
	}
	
	
};
#endif //RULE_HPP
