#ifndef RULE_HPP
#define RULE_HPP
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <vector>

class Rule{
public:
	int id;
private:
	std::vector<Rule *> nonTerminalRulesLeft;
	std::vector<Rule *> nonTerminalRulesRight;
	std::vector<int> terminalRules;
	std::vector<float> termProbabilities;
	std::vector<float> nontermProbabilities;
public:
	Rule(int id):id(id){}
	
	/*
	** Add NT production to the rule
	*/
	void addNonTerminalProduction(Rule *first,Rule *second, float propability){
		nonTerminalRulesLeft.push_back(first);
		nonTerminalRulesRight.push_back(second);
		nontermProbabilities.push_back(propability);
	}
	
	/*
	** Update probability of given production
	*/
	void updateProbability(int index, float probability, bool terminal){
		if(terminal){
			termProbabilities[index] = probability;
		}else{
			nontermProbabilities[index]=probability;
		}
	}
	
	/*
	** Add terminal production
	*/
	void addTerminalProduction(int symbol, float propability){
		terminalRules.push_back(symbol);
		termProbabilities.push_back(propability);
	}
	
	/*
	** Replace a rule of a given NT production
	*/
	void replaceNTInProduction(int index, Rule *rule, bool left){
		if(left){
			nonTerminalRulesLeft[index]=rule;
		}else{
			nonTerminalRulesRight[index]=rule;
		}
	}
	
	/*
	** Get left NT of given production
	*/
	Rule *getLeftNTProduction(int index){
		return nonTerminalRulesLeft[index];
	}
	
	/*
	** Get right NT of given production
	*/
	Rule *getRightNTProduction(int index){
		return nonTerminalRulesRight[index];
	}
	
	/*
	** Get terminal symbol of given production
	*/
	int getTerminalProduction(int index){
		return terminalRules[index];
	}
	
	/*
	** Get a probability for a given NT production
	*/
	float getNTProductionProbability(int index){
		return nontermProbabilities[index];
	}
	
	/*
	** Get a probability for a given terminal production
	*/
	float getTProductionProbability(int index){
		return termProbabilities[index];
	}
	
	/*
	** Number of NT productions
	*/
	unsigned int numberOfNTProductions(){
		return nonTerminalRulesLeft.size();
	}
	
	/*
	** Number of terminal productions
	*/
	unsigned int numberOfTermProductions(){
		return terminalRules.size();
	}
	
	/*
	** Remove production
	*/
	void removeProduction(int index, bool terminal){
		if(terminal){
			terminalRules.erase(terminalRules.begin()+index);
			termProbabilities.erase(termProbabilities.begin()+index);
		}else{
			nonTerminalRulesLeft.erase(nonTerminalRulesLeft.begin()+index);
			nonTerminalRulesRight.erase(nonTerminalRulesRight.begin()+index);
			nontermProbabilities.erase(nontermProbabilities.begin()+index);
		}
	}
	
	
};
#endif //RULE_HPP
