#ifndef PCFG_HPP
#define PCFG_HPP
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <map>
#include <math.h>
#include <time.h>
#include <algorithm>

#include "Rule.hpp"

class PCFG{
public:
	typedef struct{
		std::map<std::vector<int>, int> timesFound;
		std::map<std::vector<int>, double> totalProb;
		std::map<std::vector<int>, bool> conflict;
		bool atLeastOneRuleWithoutConflict;
	}allChunks;
	//Keep the id of the Start Rules
	std::vector<int> startRules;
	//Keep all the rules
	std::vector<Rule> allRules;
	//Map term symbol to rule for fast access
	std::map<int,int> rulesForTermSymbol;
	std::map<int,char> intToCharTerminalValue;
private:
	int currFreeId;
public:
	PCFG(){
		srand (time(NULL));
	};
	
	/*
	** Create NT symbols that point to the terminal symbols
	*/
	void initFirstNT(int symbols){
		cleanUp();
		currFreeId = symbols;
		for(int i=0; i<symbols; i++){
			Rule rule(currFreeId);
			std::vector<int> v;
			v.push_back(i);
			rule.addProduction(v,1.0,true);
			allRules.push_back(rule);
			rulesForTermSymbol[i] = rule.id;
			currFreeId++;
		}
	}
	
	int createStartRule(std::vector<std::vector<int> > allUniqueWords, std::vector<double> wordCount){
		Rule rule(currFreeId);
		currFreeId++;
		for(unsigned int i=0; i<allUniqueWords.size(); i++){
			rule.addProduction(allUniqueWords[i], wordCount[i], false);
		}
		allRules.push_back(rule);
		startRules.push_back(rule.id);
		return rule.id;
	}
	
	/*
	** Create new NT
	*/
	int createNewNT(std::vector<int> rightPart, double prob){
		Rule rule(currFreeId);
		currFreeId++;
		rule.addProduction(rightPart, prob, false);
		allRules.push_back(rule);
		return rule.id;
	}
	
	int const totalSizeOfGrammar(){
		int totalLength = 0;
		for(unsigned int iter = 0; iter<allRules.size(); iter++){
			if(allRules[iter].hasNTproduction == false)
				continue;
			totalLength += allRules[iter].totalLength;
		}
		return totalLength;
	}

	/*
	** Merge two different NT given their ID. The first NT will remain in the grammar
	** with the addition of the rules that the second NT has. If there is the same rule
	** in both NT's we will increase its likelihood. The second NT will be deleted!
	** Every occurance of the second NT in the grammar will be replaced with the first one!
	*/
	double mergeTwoNT(int hostID, int targetID, double *pOgivenG){
		int host = locationOfRule(hostID);
		int target = locationOfRule(targetID);
		double totalGain = 0.0;
		//Remove duplicate rules
		totalGain += mergeSameProductions(hostID,targetID);
		//Merge the target with the host rule
		for(unsigned int iter = 0; iter<allRules[target].totalNumberOfProductions(); iter++){
			Rule::productions toTransfer = allRules[target].getProduction(iter);
			if(toTransfer.terminal){
				allRules[host].addProduction(toTransfer.rightRules, toTransfer.probability, true);
				rulesForTermSymbol[toTransfer.rightRules[0]] = hostID;
			}else{
				allRules[host].addProduction(toTransfer.rightRules, toTransfer.probability, false);
			}
		}
		
		//Replace every occurance of the target rule with the host rule
		for(unsigned int iter1 = 0; iter1<allRules.size(); iter1++){
			for(unsigned int iter2 = 0; iter2<allRules[iter1].totalNumberOfProductions(); iter2++){
				Rule::productions currProd = allRules[iter1].getProduction(iter2);
				bool changed = false;
				for(unsigned int i = 0; i<currProd.rightRules.size(); i++){
					if(currProd.rightRules[i] == targetID){
						currProd.rightRules[i] = hostID;
						changed = true;
					}
				}
				if(changed){
					allRules[iter1].updateProductions(currProd, iter2);
				}
			}
		}
		
		//Delete target
		allRules.erase(allRules.begin()+target);
		
		
		
		
		for(unsigned int iter=0; iter<allRules.size(); iter++){
			double temp = mergeDuplicatesProductionsInSameRule(allRules[iter].id,pOgivenG);
			totalGain += temp;			
		}
		
		totalGain += deleteUselessProductions(hostID, pOgivenG);
		
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
				if(hostProd.rightRules.size() == targetProd.rightRules.size()){
					bool same = true;
					for(unsigned int i = 0; i<hostProd.rightRules.size(); i++){
						if(hostProd.rightRules[i] != targetProd.rightRules[i]){
							same = false;
							break;
						}
					}
					if(same){
						totalGain *= pow(2,hostProd.rightRules.size());
						break;
					}
				}
			}
		}
		return totalGain;
	}

	/*
	** Calculate all possible Chunks
	*/
	allChunks calculateAllChunks(int maxChunkSize){
		std::vector<int> key;
		allChunks chunks;
		for(unsigned int iterRules=0; iterRules<allRules.size(); iterRules++){
			for(unsigned int iterProd=0; iterProd<allRules[iterRules].totalNumberOfProductions(); iterProd++){
				Rule::productions prod = allRules[iterRules].getProduction(iterProd);
				for(unsigned int iterProdRule=0; iterProdRule<prod.rightRules.size(); iterProdRule++){
					key.clear();
					for(int iter=0; iter<maxChunkSize; iter++){
						if(iter+iterProdRule >= prod.rightRules.size()){
							break ;
						}
						key.push_back(prod.rightRules[iterProdRule+iter]);
						//Chunks bigger than 1 in size
						if(iter >= 1){
							chunks.timesFound[key]++;
							chunks.totalProb[key] += prod.probability;
							if(chunks.conflict[key] == false){
								if((int)prod.rightRules.size() - (int)iterProdRule-1 > iter){
									int end = iterProdRule+iter+iter > prod.rightRules.size()-1 ? prod.rightRules.size() : iterProdRule+iter+iter+1;
									std::vector<int>::iterator found;
									found = std::search(prod.rightRules.begin() + iterProdRule + 1, prod.rightRules.begin()+ end, key.begin(), key.end());
									if(found != prod.rightRules.begin() + end){
										chunks.conflict[key] = true;
									}	
								}
							}
						}
					}
				}
			}
		}
		chunks.atLeastOneRuleWithoutConflict = false;
		for(std::map<std::vector<int>,int>::iterator iter = chunks.timesFound.begin(); iter != chunks.timesFound.end(); iter++){
			if(iter->second > 1 && chunks.conflict[iter->first] == false){
				chunks.atLeastOneRuleWithoutConflict = true;
				break;
			}
		}
		return chunks;
	}
	
	/*
	** Replace all occurences of a sequence with a single rule
	*/
	std::pair<int, double> replaceSubsequence(int ruleID){
		unsigned int ruleLoc = locationOfRule(ruleID);
		std::pair<int,double> rt;
		rt.first = 0;
		rt.second = 0.0;
		for(unsigned int i=0; i<allRules[ruleLoc].totalNumberOfProductions(); i++){
			Rule::productions prod = allRules[ruleLoc].getProduction(i);
			for(unsigned int j=0; j<allRules.size(); j++){
				if(j==ruleLoc){
					continue;
				}
				for(unsigned int k=0; k<allRules[j].totalNumberOfProductions(); k++){
					Rule::productions tmp = allRules[j].getProduction(k);
					std::vector<int>::iterator found = std::search(tmp.rightRules.begin(),tmp.rightRules.end(), prod.rightRules.begin(), prod.rightRules.end());
					while(found != tmp.rightRules.end()){
						tmp.rightRules.erase(found, found+prod.rightRules.size());
						tmp.rightRules.insert(found, ruleID);
						allRules[j].updateProductions(tmp, k);
						rt.first++;
						rt.second += tmp.probability;
						found = std::search(tmp.rightRules.begin(),tmp.rightRules.end(), prod.rightRules.begin(), prod.rightRules.end());
					}
				}
			}
		}
		return rt;		
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
	** Normalize the probabiltiies of each production
	*/
	void normalizeGrammar(){
		for(std::vector<Rule>::iterator iter = allRules.begin(); iter != allRules.end(); iter++){
			double sumOfAllProb = 0.0;
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
	
	/*
	** Transform PCFG to chomsky normal form
	*/
	void toCnf(){
		double trash;
		for(unsigned int i=0; i<allRules.size(); i++){
			deleteUselessProductions(allRules[i].id,&trash);
		}
		breakLargeRules();
		eliminateSingletons();
	}
	
	/*
	**	Pretty print to cout
	*/
	void prettyPrint(){
		prettyPrint(std::cout.rdbuf());
	}
	
	/*
	**	Pretty print to file
	*/
	void prettyPrint(std::string filename){
		std::ofstream myfile(filename.c_str());
		prettyPrint(myfile.rdbuf());
		myfile.close();
	}

	/*
	** Write the grammar in a format to be read from the inside outside implementation
	*/
	void writeForInsideOutside(std::string filename){

		std::ofstream myfile(filename.c_str());
		std::ostream out(myfile.rdbuf());
		out << "1 " << "S --> " << "N" << startRules[0] << std::endl;
		for(unsigned int i=0; i<allRules.size(); i++){
			for(unsigned int j=0; j<allRules[i].totalNumberOfProductions(); j++){
				Rule::productions tmp = allRules[i].getProduction(j);
				out << tmp.probability << " ";
				out << "N" << allRules[i].id;
				out << " --> ";
				if(tmp.terminal == false){
					for(unsigned int k = 0; k<tmp.rightRules.size();k++){
						out << "N" << tmp.rightRules[k] << " ";
					}
				}else{
					out << intToCharTerminalValue[tmp.rightRules[0]] << " ";
				}
				out << std::endl;
			}
		}
		myfile.close();
	}
	
	//Create a^n c b^n where n > 0
	void createTestPCFG(){
		cleanUp();
		Rule A(3);
		Rule B(4);
		Rule C(5);
		Rule T(6);
		Rule S(7);
		rulesForTermSymbol[0] = A.id;
		rulesForTermSymbol[1] = C.id;
		rulesForTermSymbol[2] = B.id;
		intToCharTerminalValue[0] = 'a';
		intToCharTerminalValue[1] = 'c';
		intToCharTerminalValue[2] = 'b';
		
		std::vector<int> tmp;
		tmp.push_back(T.id);
		tmp.push_back(B.id);
		S.addProduction(tmp,1.0,false);
		tmp.clear();
		tmp.push_back(A.id);
		tmp.push_back(C.id);
		T.addProduction(tmp,0.1,false);
		tmp.clear();
		tmp.push_back(A.id);
		tmp.push_back(S.id);
		T.addProduction(tmp,0.9,false);
		tmp.clear();
		tmp.push_back(0);
		A.addProduction(tmp,1.0,true);
		tmp.clear();
		tmp.push_back(2);
		B.addProduction(tmp,1.0,true);
		tmp.clear();
		tmp.push_back(1);
		C.addProduction(tmp,1.0,true);
		allRules.push_back(S);
		startRules.push_back(S.id);
		allRules.push_back(A);
		allRules.push_back(B);
		allRules.push_back(C);
		allRules.push_back(T);
		currFreeId = 8;
	}
	
	void createTestPCFG2(){
		cleanUp();
		Rule A(3);
		Rule B(4);
		Rule C(5);
		Rule T1(6);
		Rule T2(7);
		Rule T3(8);
		Rule S(9);
		rulesForTermSymbol[0] = A.id;
		rulesForTermSymbol[1] = B.id;
		rulesForTermSymbol[2] = C.id;
		intToCharTerminalValue[0] = 'a';
		intToCharTerminalValue[1] = 'b';
		intToCharTerminalValue[2] = 'c';
		
		std::vector<int> tmp;
		tmp.push_back(A.id);
		tmp.push_back(T1.id);
		S.addProduction(tmp,1.0,false);
		//T1
		tmp.clear();
		tmp.push_back(A.id);
		tmp.push_back(T1.id);
		T1.addProduction(tmp,0.5,false);
		tmp.clear();
		tmp.push_back(B.id);
		tmp.push_back(T2.id);
		T1.addProduction(tmp,0.3,false);
		tmp.clear();
		tmp.push_back(B.id);
		tmp.push_back(C.id);
		T1.addProduction(tmp,0.2,false);

		//T2
		tmp.clear();
		tmp.push_back(B.id);
		tmp.push_back(T2.id);
		T2.addProduction(tmp,0.5,false);
		tmp.clear();
		tmp.push_back(C.id);
		tmp.push_back(T3.id);
		T2.addProduction(tmp,0.3,false);
		tmp.clear();
		tmp.push_back(B.id);
		tmp.push_back(C.id);
		T2.addProduction(tmp,0.2,false);

		//T3
		tmp.clear();
		tmp.push_back(C.id);
		tmp.push_back(T3.id);
		T3.addProduction(tmp,0.3,false);
		tmp.clear();
		tmp.push_back(C.id);
		tmp.push_back(C.id);
		T3.addProduction(tmp,0.7,false);

		tmp.clear();
		tmp.push_back(0);
		A.addProduction(tmp,1.0,true);
		tmp.clear();
		tmp.push_back(1);
		B.addProduction(tmp,1.0,true);
		tmp.clear();
		tmp.push_back(2);
		C.addProduction(tmp,1.0,true);

		allRules.push_back(S);
		startRules.push_back(S.id);
		allRules.push_back(A);
		allRules.push_back(B);
		allRules.push_back(C);
		allRules.push_back(T1);
		allRules.push_back(T2);
		allRules.push_back(T3);
		currFreeId = 10;
	}
	void createTestPCFG3(){
		cleanUp();
		Rule A(7);
		Rule B(4);
		Rule C(5);
		Rule D(6);
		Rule T1(8);
		Rule T2(9);
		Rule T3(10);
		Rule T4(11);
		Rule S(12);
		rulesForTermSymbol[3] = A.id;
		rulesForTermSymbol[0] = B.id;
		rulesForTermSymbol[1] = C.id;
		rulesForTermSymbol[2] = D.id;
		intToCharTerminalValue[3] = 'a';
		intToCharTerminalValue[0] = 'b';
		intToCharTerminalValue[1] = 'c';
		intToCharTerminalValue[2] = 'd';
		
		std::vector<int> tmp;
		tmp.push_back(A.id);
		tmp.push_back(T1.id);
		S.addProduction(tmp,0.4,false);
		tmp.clear();
		tmp.push_back(B.id);
		tmp.push_back(T2.id);
		S.addProduction(tmp,0.3,false);
		tmp.clear();
		tmp.push_back(C.id);
		tmp.push_back(T3.id);
		S.addProduction(tmp,0.2,false);
		tmp.clear();
		tmp.push_back(D.id);
		tmp.push_back(T4.id);
		S.addProduction(tmp,0.1,false);
		//T1
		tmp.clear();
		tmp.push_back(A.id);
		tmp.push_back(T1.id);
		T1.addProduction(tmp,0.1,false);
		tmp.clear();
		tmp.push_back(A.id);
		tmp.push_back(T2.id);
		T1.addProduction(tmp,0.9,false);

		//T2
		tmp.clear();
		tmp.push_back(B.id);
		tmp.push_back(T2.id);
		T2.addProduction(tmp,0.1,false);
		tmp.clear();
		tmp.push_back(B.id);
		tmp.push_back(T3.id);
		T2.addProduction(tmp,0.9,false);

		//T3
		tmp.clear();
		tmp.push_back(C.id);
		tmp.push_back(T3.id);
		T3.addProduction(tmp,0.1,false);
		tmp.clear();
		tmp.push_back(C.id);
		tmp.push_back(T4.id);
		T3.addProduction(tmp,0.2,false);
		tmp.clear();
		tmp.push_back(C.id);
		tmp.push_back(D.id);
		T3.addProduction(tmp,0.7,false);

		//T4
		tmp.clear();
		tmp.push_back(D.id);
		tmp.push_back(T4.id);
		T4.addProduction(tmp,0.1,false);
		tmp.clear();
		tmp.push_back(D.id);
		tmp.push_back(D.id);
		T4.addProduction(tmp,0.9,false);

		tmp.clear();
		tmp.push_back(3);
		A.addProduction(tmp,1.0,true);
		tmp.clear();
		tmp.push_back(0);
		B.addProduction(tmp,1.0,true);
		tmp.clear();
		tmp.push_back(1);
		C.addProduction(tmp,1.0,true);
		tmp.clear();
		tmp.push_back(2);
		D.addProduction(tmp,1.0,true);

		allRules.push_back(S);
		startRules.push_back(S.id);
		allRules.push_back(A);
		allRules.push_back(B);
		allRules.push_back(C);
		allRules.push_back(D);
		allRules.push_back(T1);
		allRules.push_back(T2);
		allRules.push_back(T3);
		allRules.push_back(T4);
		currFreeId = 13;
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
	** Prun all the productions which probability is below the threshold
	*/
	void pruneProductions(float threshold){
		std::vector<bool> toDeleteRules(allRules.size(),false);
		for(unsigned int i=0; i<allRules.size(); i++){
			std::vector<bool> toDelete(allRules[i].totalNumberOfProductions(),false);
			for(unsigned int j=0; j<allRules[i].totalNumberOfProductions(); j++){
				if(allRules[i].getProbability(j) <= threshold){
					toDelete[j] = true;
				}
			}
			for(int k=toDelete.size()-1; k>=0; k--){
				if(toDelete[k]){
					allRules[i].removeProduction(k);
				}
			}
			if(allRules[i].hasNTproduction && allRules[i].totalLength == 1){
				toDeleteRules[i] = true;
			}
		}
		for(int k=toDeleteRules.size()-1; k>=0; k--){
			if(toDeleteRules[k]){
				allRules.erase(allRules.begin()+k);
			}
		}
		normalizeGrammar();
	}

	/*
	**	Return the total count of one rule
	*/
	double getTotalCount(int ruleID){
		double result = 0.0;
		int ruleLoc = locationOfRule(ruleID);
		for(unsigned int iter = 0; iter<allRules[ruleLoc].totalNumberOfProductions(); iter++){
			result += allRules[ruleLoc].getProduction(iter).probability;
		}
		return result;
	}
	
	/*
	** Generate a random word
	*/
	std::vector<int> generateWord(){
		std::vector<int> randomWord;
		randomWord.push_back(startRules[0]);
		do{
			unsigned int i=0;
			while(i<randomWord.size() && randomWord[i] < intToCharTerminalValue.size()){
				i++;
			}
			if(i == randomWord.size()){
				break;
			}
			int ruleLoc = locationOfRule(randomWord[i]);
			float randomNumber = ((float) rand() / (RAND_MAX));
			std::vector<int> newV = allRules[ruleLoc].randomProduction(randomNumber);
			randomWord.erase(randomWord.begin() + i);
			randomWord.insert(randomWord.begin() + i, newV.begin(), newV.end());
		}while(true);
		for(unsigned int i=0; i<randomWord.size(); i++){
			std::cout << intToCharTerminalValue[randomWord[i]] << " ";
			//std::cout << randomWord[i] << " ";
		}
		std::cout << std::endl;
		return randomWord;
	}
	
	void dumpPCFGtoFile(std::string filename){
		std::ofstream myfile;
		myfile.open (filename.c_str());
		myfile << startRules[0] << std::endl;
		myfile << rulesForTermSymbol.size() << std::endl;
		for(std::map<int, int>::iterator iter=rulesForTermSymbol.begin(); iter!=rulesForTermSymbol.end(); iter++){
			myfile << iter->first << " " << iter->second << std::endl;
		}
		for(unsigned int i=0; i<allRules.size(); i++){
			myfile << allRules[i].id << " " << allRules[i].totalNumberOfProductions() << std::endl;
			for(unsigned int j=0; j<allRules[i].totalNumberOfProductions(); j++){
				Rule::productions prod = allRules[i].getProduction(j);
				myfile << prod.rightRules.size() << " ";
				for(unsigned int k=0; k<prod.rightRules.size(); k++){
					myfile << prod.rightRules[k] << " ";
				}
				myfile << prod.probability << " ";
				myfile << prod.terminal << std::endl;
			}
		}
		myfile.close();
	
	}
	
	void readPCFGfromFile(std::string filename){
		cleanUp();
		std::ifstream myfile(filename.c_str());
		int startRule;
		myfile >> startRule;
		startRules.push_back(startRule);
		int num;
		myfile >> num;
		for(int i=0; i<num; i++){
			int r,l;
			myfile >> r;
			myfile >> l;
			rulesForTermSymbol[r] = l;
		}
		while(true){
			int ruleID;
			myfile >> ruleID;
			if(myfile.eof()){
				break;
			}
			myfile >> num;
			int ruleLoc = createNewEmptyNTwithID(ruleID);
			for(int i=0; i<num; i++){
				int num2;
				myfile >> num2;
				Rule::productions prod;
				for(int j=0; j<num2; j++){
					int symbol;
					myfile >> symbol;
					prod.rightRules.push_back(symbol);
				}
				float prob;
				myfile >> prob;
				prod.probability = prob;
				bool term;
				myfile >> term;
				prod.terminal=term;
				allRules[ruleLoc].appendProduction(prod);
			}
		}
		myfile.close();
		
	}
	
private:
	/*
	** Create empty NT with forcedID
	*/
	int createNewEmptyNTwithID(int ruleID){
		if(ruleID > currFreeId){
			currFreeId = ruleID + 1;
		}
		Rule rule(ruleID);
		allRules.push_back(rule);
		//location of rule;
		return allRules.size()-1;
	}
	/*
	** Find and delete useless productions a merged node
	*/
	double deleteUselessProductions(int ruleID, double *pOgivenG){
		unsigned int ruleLoc = locationOfRule(ruleID);
		std::vector<bool> toDelete(allRules[ruleLoc].totalNumberOfProductions());
		double totalGain = 0.0;
		bool somethingToDelete = false;
		double inCaseOfDelete = 0.0;
		double totalCount = getTotalCount(ruleID);
		for(unsigned int i = 0; i<allRules[ruleLoc].totalNumberOfProductions(); i++){
			Rule::productions prod = allRules[ruleLoc].getProduction(i);
			inCaseOfDelete += prod.probability*log10(prod.probability/totalCount);
			if(prod.rightRules.size()==1 && prod.rightRules[0] == ruleID){
				toDelete[i] = true;
				totalGain += (1+1)*log10(2);
				somethingToDelete = true;
			}
		}
		if(somethingToDelete){
			
			for(int i=toDelete.size()-1; i>=0; i--){
				if(toDelete[i]){
					allRules[ruleLoc].removeProduction(i);
				}
			}


			*pOgivenG -= inCaseOfDelete;
			totalCount = getTotalCount(ruleID);
			double temp = 0.0;
			for(unsigned int i = 0; i<allRules[ruleLoc].totalNumberOfProductions(); i++){
				temp += allRules[ruleLoc].getProduction(i).probability*log10(allRules[ruleLoc].getProduction(i).probability/totalCount);
			}
			*pOgivenG += temp;

		}
		
		return totalGain;
	}
	
	/*
	** Find if the same production exists more than one time in a given Rule
	*/
	double mergeDuplicatesProductionsInSameRule(int hostID,double *pOgivenG){
		int host = locationOfRule(hostID);
		double totalCount = -1.0;
		double totalGain = 0.0;
		std::vector<bool> toDelete(allRules[host].totalNumberOfProductions(),false);
		for(unsigned int iter1 = 0; iter1<allRules[host].totalNumberOfProductions(); iter1++){
			if(toDelete[iter1]){
				continue;
			}
			Rule::productions firstProd = allRules[host].getProduction(iter1);
			for(unsigned int iter2 = iter1+1; iter2<allRules[host].totalNumberOfProductions(); iter2++){
				Rule::productions secProd = allRules[host].getProduction(iter2);
				if(!toDelete[iter2] && firstProd.rightRules.size() == secProd.rightRules.size()){
					bool same = true;
					for(unsigned int i = 0; i< firstProd.rightRules.size(); i++){
						if(firstProd.rightRules[i] != secProd.rightRules[i]){
							same = false;
							break;
						}
					}
					if(same == true){
						if(totalCount < 0){
							totalCount = getTotalCount(hostID);
						}
						toDelete[iter2] = true;
						double newProbability = firstProd.probability + secProd.probability;
						//gain from replacement of rule
						totalGain += (firstProd.rightRules.size()+1)*log10(2);
						//recalculate priori
						*pOgivenG += newProbability*log10(newProbability/totalCount) -firstProd.probability*log10(firstProd.probability/totalCount) - secProd.probability*log10(secProd.probability/totalCount);
						allRules[host].updateProbability(iter1, newProbability);
						firstProd.probability = newProbability;
						//break;
					}
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
		double totalGain = 0.0;
		for(unsigned int iter1 = 0; iter1<allRules[host].totalNumberOfProductions(); iter1++){
			Rule::productions hostProd = allRules[host].getProduction(iter1);
			for(unsigned int iter2 = 0; iter2<allRules[target].totalNumberOfProductions(); iter2++){
				Rule::productions targetProd = allRules[target].getProduction(iter2);
				if(hostProd.rightRules.size() == targetProd.rightRules.size()){
					
					bool same = true;
					for(unsigned int i = 0; i< hostProd.rightRules.size(); i++){
						if(hostProd.rightRules[i] != targetProd.rightRules[i]){
							same = false;
							break;
						}
					}
					if(same){
						double newProbability = hostProd.probability + targetProd.probability;
						allRules[host].updateProbability(iter1, newProbability);
						allRules[target].removeProduction(iter2);
						totalGain += (hostProd.rightRules.size()+1)*log10(2);
						break;
					}
				}
			}
		}
		return totalGain;
	}
	
	/*
	**	Break big rules to create rules of size two
	*/
	void breakLargeRules(){
		//This will change thrue the loop
		int currAllRulesSize = allRules.size();

		for(int i=0; i<currAllRulesSize; i++){
			for(unsigned int j=0; j<allRules[i].totalNumberOfProductions(); j++){
				Rule::productions prod = allRules[i].getProduction(j);
				if(prod.rightRules.size() > 2){
					int k = prod.rightRules.size()-1;
					std::vector<int> newRight;
					newRight.insert(newRight.begin(),prod.rightRules[k]);
					k--;
					newRight.insert(newRight.begin(),prod.rightRules[k]);
					k--;
					int newRule = createNewNT(newRight, 1.0);
					while(k>=1){
						newRight.clear();
						newRight.push_back(prod.rightRules[k]);
						newRight.push_back(newRule);
						k--;
						newRule = createNewNT(newRight, 1.0);
					}
					newRight.clear();
					newRight.push_back(prod.rightRules[0]);
					newRight.push_back(newRule);
					prod.rightRules = newRight;
					allRules[i].updateProductions(prod,j);
				}
			}
		}
	}
	
	/*
	**	Eliminate Singleton productions
	*/
	void eliminateSingletons(){
		//This will change thrue the loop
		for(unsigned int i=0; i<allRules.size(); i++){
			for(unsigned int k=0; k<allRules.size(); k++){
				if(i==k){
					continue;
				}
				for(int j=0; j<(int)allRules[k].totalNumberOfProductions(); j++){
					Rule::productions prod = allRules[k].getProduction(j);
				
					if(prod.rightRules.size() == 1 && prod.rightRules[0] == allRules[i].id){
				
						std::cout << "Merging = " << allRules[k].id << " " << prod.rightRules[0] << std::endl;
					
						bool toDelete = true;
						for(unsigned int l=0; l<allRules[i].totalNumberOfProductions(); l++){
							Rule::productions newProd = allRules[i].getProduction(l);
							if(newProd.terminal == false){
								newProd.probability *= prod.probability;
								allRules[k].appendProduction(newProd);
							}else{
								toDelete = false;
								break;
							}
						}
						if(toDelete){
							allRules[k].removeProduction(j);
							double temp;
							deleteUselessProductions(allRules[k].id, &temp);
							mergeDuplicatesProductionsInSameRule(allRules[k].id, &temp);
							//prettyPrint();
							//char ch;
							//std::cin >> ch;
						}
					}
				}
			}	
		}
	}
	
	/*
	** Pretty print the PCFG
	*/
	void prettyPrint(std::streambuf * buf){
		std::ostream out(buf);
		out << "Start Rules: " << std::endl;
		for(unsigned int i=0; i<startRules.size(); i++){
			out << "N" << startRules[i] << std::endl;
		}
		out << "All Rules: " << std::endl;
		for(unsigned int i=0; i<allRules.size(); i++){
			out << "\tN" << allRules[i].id;
			for(unsigned int j=0; j<allRules[i].totalNumberOfProductions(); j++){
				Rule::productions tmp = allRules[i].getProduction(j);
				out << "\t-> ";
				if(tmp.terminal == false){
					for(unsigned int k = 0; k<tmp.rightRules.size();k++){
						out << "N" << tmp.rightRules[k] << " ";
					}
				}else{
					out << intToCharTerminalValue[tmp.rightRules[0]] << " ";
				}
				out << "(" << tmp.probability << ")" << std::endl << "\t";
			}
			out << std::endl;
		}
	}
	
};
#endif //PCFG_HPP

