#include "BeamSearch.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits>

double L;
using namespace std;

PCFG searchIncrementallyForBestPCFG(Corpus corpus, double lValue, string fileIO){
	//Words for parser
	Corpus::words allInitWords = corpus.getUniqueWords();

	PCFG pcfgForCYK;

	//Initialize PCFG
	searchNode parent;
 
 	L = lValue;//0.001;
	parent.pcfg.intToCharTerminalValue = corpus.symbolsToChars();
	parent.pcfg.initFirstNT(corpus.numberOfSymbolsInCorpus());
	parent.currGain = 0.0;
	parent.currPriori = 0.0;
	parent.currLikelihood = 0.0;
	parent.expanded = false;
	corpus.initReduceForPCFG(&parent.pcfg);
	Corpus::words allWords = corpus.getUniqueWords();

	int startId = parent.pcfg.createStartRule(vector<vector<int> >(), vector<double>());
	int startRuleLoc = parent.pcfg.locationOfRule(startId);
	
	Corpus onlyParsedCorpus = corpus;
	onlyParsedCorpus.clearForInsideOutside();
	unsigned int i; 
	do{
		for(i=0; (i < _batch_ && i<allWords.size()); i++){
				parent.pcfg.allRules[startRuleLoc].addProduction(allWords[i].word, allWords[i].count, false);
		}
		parent.expanded = false;
		parent = doBeamSearch(parent,_beamWidth_,_beamDepth_);

		recalculateProbabilities(&onlyParsedCorpus, &allInitWords, &allWords, &parent.pcfg, fileIO);
		parent.pcfg.pruneProductions(0.0001);
	}while(i == _batch_ && allWords.size() > 0);
	
	parent.pcfg.normalizeGrammar();
	return parent.pcfg;	
}


searchNode doBeamSearch(searchNode root, int width, int depth){
	
	int globalIter = 0;
	int cPq = 0;
	bool somethingChanged = true;

	priority_queue<searchNode, vector<searchNode>, mycomparison> *pqForBeam = new priority_queue<searchNode, vector<searchNode>, mycomparison>[2];

	searchNode gBest;
	gBest = root;

	pqForBeam[cPq].push(root);

	while(pqForBeam[cPq].top().currGain >= 0.0 && somethingChanged == true && globalIter < 50000){
		cout << "---------- " << pqForBeam[cPq].size() << " " <<  pqForBeam[cPq].top().currGain << " " <<  pqForBeam[cPq].top().currLikelihood << " -----------------" << endl;
		int invcPq = cPq == 0 ? 1 : 0;
		somethingChanged = false;
		
		gBest = pqForBeam[cPq].top();
		gBest.pcfg.prettyPrint();

		//empty the q
		while(pqForBeam[invcPq].size() > 0){
			pqForBeam[invcPq].pop();
		}
		//Check the top n nodes
		int totalNodesInTheBeam = 0;
		int totalUnexpanded = 0;
		double prevGain = -1.0;
		cout << pqForBeam[invcPq].size() << endl;
		while(pqForBeam[cPq].size() != 0){
	
			searchNode node = pqForBeam[cPq].top();
			pqForBeam[cPq].pop();
			if(totalNodesInTheBeam == depth){
				continue;
			}
			//FIX THAT TODO
			if(prevGain == node.currGain){
				continue;
			}
			prevGain = node.currGain;				
		
			totalNodesInTheBeam++;
		
			//Expand node
			if(node.expanded == false && totalUnexpanded < width){
				totalUnexpanded++;
				node.expanded = true;
				vector<searchNode> possiblePCFGS = findAllPossibleMoves(node);
				for(unsigned int i=0; i<possiblePCFGS.size(); i++){
					pqForBeam[invcPq].push(possiblePCFGS[i]);
					somethingChanged = true;
				}
				pqForBeam[invcPq].push(node);
			}else{
				pqForBeam[invcPq].push(node);	
			}
		}
		
		cPq = invcPq;
		globalIter++;
		
		cout << pqForBeam[cPq].size() << " " << totalNodesInTheBeam << " " << totalUnexpanded << endl;
		cout << "---------------------------------------" << endl;
	}
	cout << "Best gain = " << gBest.currGain << endl;
	gBest.pcfg.prettyPrint();
	return gBest;
}

vector<searchNode> findAllPossibleMoves(searchNode parent){
	vector<searchNode> possiblePCFGS;
	//Do all Chunks
	doChunk(&possiblePCFGS,parent);
	//Do all merges
	doMerge(&possiblePCFGS,parent);
	return possiblePCFGS;
}

void doChunk(vector<searchNode> *listOfNodes, searchNode parent){
	PCFG::allChunks chunks = parent.pcfg.calculateAllChunks(_chunkLength_);
	
	for(map<std::vector<int>, int>::iterator iter = chunks.timesFound.begin(); iter!=chunks.timesFound.end(); iter++){
		
		searchNode newNode = parent;
		newNode.expanded = false;
		int ruleID = newNode.pcfg.createNewNT(iter->first, chunks.totalProb[iter->first]);
		int ruleLoc = newNode.pcfg.locationOfRule(ruleID);
		pair<int,double> pairFoundProb = newNode.pcfg.replaceSubsequence(ruleID);
		
		if(pairFoundProb.first <= 1){// || newNode.conflict){
			continue;
		}
		
		newNode.pcfg.allRules[ruleLoc].updateProbability(0, pairFoundProb.second);
		
		//Extra cost to create if there is a conflict
		int extraCost = (chunks.conflict[iter->first] == true && chunks.atLeastOneRuleWithoutConflict == true) ? 1 : 1;
		
		int costToCreateRule = extraCost*(1+1+iter->first.size());
		
		//symbols removed - symbol added - new rule
		double pG = L*((iter->first.size()*pairFoundProb.first)*log10(2) - pairFoundProb.first*log10(2) - costToCreateRule*log10(2) - log10(extraCost));
		//cout << pG << endl;
		newNode.conflict = chunks.conflict[iter->first];
		newNode.currGain += pG;
		newNode.currPriori += pG;
		newNode.isChunk = true;
		listOfNodes->push_back(newNode);
	}
}

void doMerge(vector<searchNode> *listOfNodes, searchNode parent){
	
	for(int i=0; i<(int)parent.pcfg.allRules.size()-1; i++){
		//Calculate the total count for the first rule (sum all probabilities)
		if(parent.pcfg.allRules[i].hasNTproduction == false){
			continue;
		}
		double totalOfFirstRule = parent.pcfg.getTotalCount(parent.pcfg.allRules[i].id);
		for(unsigned int j=i+1; j<parent.pcfg.allRules.size(); j++){
			if(parent.pcfg.allRules[j].hasNTproduction == false){
				continue;
			}
			//cout << "------------------------------------------" << endl;
			searchNode newNode = parent;
			newNode.expanded = false;
			newNode.isChunk = false;
			newNode.conflict = false;
			//Calculate the total count for the second rule (sum all probabilities)
			double totalOfSecondRule = parent.pcfg.getTotalCount(parent.pcfg.allRules[j].id);
			
			//Calculate the change in the likelihood
			float totalSum = totalOfFirstRule + totalOfSecondRule;
			double likelihood = 0.0;
			for(unsigned int iter = 0; iter<parent.pcfg.allRules[i].totalNumberOfProductions(); iter++){
				double up = parent.pcfg.allRules[i].getProduction(iter).probability/totalSum;
				double down = parent.pcfg.allRules[i].getProduction(iter).probability/totalOfFirstRule;
				likelihood += parent.pcfg.allRules[i].getProduction(iter).probability*log10(up/down);
			}
			
			for(unsigned int iter = 0; iter<parent.pcfg.allRules[j].totalNumberOfProductions(); iter++){
				double up = parent.pcfg.allRules[j].getProduction(iter).probability/totalSum;
				double down = parent.pcfg.allRules[j].getProduction(iter).probability/totalOfSecondRule;
				likelihood += parent.pcfg.allRules[j].getProduction(iter).probability*log10(up/down);
			}
					
			double extraGainFromMerge = newNode.pcfg.mergeTwoNT(parent.pcfg.allRules[i].id,parent.pcfg.allRules[j].id, &likelihood);

			double pG = L*(log10(2) + extraGainFromMerge);
			newNode.currGain += pG + (1.0/L2)*likelihood;
			newNode.currLikelihood += (1.0/L2)*likelihood;
			newNode.currPriori += pG;
			
			listOfNodes->push_back(newNode);
		}
	}
}


void recalculateProbabilities(Corpus *corpus, Corpus::words *allInitWords, Corpus::words *allWords, PCFG *pcfg, string fileIO){
	CYKparser parser;
	PCFG cnfPCFG = *pcfg;
	cnfPCFG.normalizeGrammar();
	cnfPCFG.toCnf();

	vector<char> toDelete(allWords->size(), 0);
	for(unsigned int i=0; i<allWords->size(); i++){
		double parseProb = parser.parseWord(&cnfPCFG, allInitWords->at(i).word);
		if(parseProb > 0){
			corpus->addUniqueWord(allInitWords->at(i));
			toDelete[i] = true;
		}
	}
	for(int i=allWords->size()-1; i>=0; i--){
		if(toDelete[i] == true){
			allWords->erase(allWords->begin()+i);
			allInitWords->erase(allInitWords->begin()+i);
		}
	}
	string corpusFile = string("../") + fileIO + ".txt";
	corpus->dumbCorpusToFile(fileIO + ".txt",true);
	//corpus->printCorpus();
	string pcfgFile = string("../") + fileIO + ".grammar";
	pcfg->writeForInsideOutside(fileIO + ".grammar");

	int pid = fork();
	if(pid == 0){
		int r = chdir("Inside-Outside");
    	execl("./io", "./io", "-d", "1000", "-s", "0.0001", "-g", pcfgFile.c_str(), corpusFile.c_str(), (char *) 0);
    	std::cerr << "Apetixa" << std::endl;
    	exit(-5);
	}else{ // This is the parent process
    	int status;
        waitpid(pid, &status, 0); // Wait for the child process to return.
        std::cout << "Process returned " << status << ".\n";
    }

    ifstream newGrammarFile(string(fileIO + ".grammarNew").c_str());
    //Eat the start rule;
	newGrammarFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    for( unsigned int i=0; i<pcfg->allRules.size(); i++){
    	for(int j=0; j<pcfg->allRules[i].totalNumberOfProductions(); j++){
       		double prob;
    		newGrammarFile >> prob;
    		if(pcfg->allRules[i].getProduction(j).terminal){
				pcfg->allRules[i].updateProbability(j,1.0);
    		}else{
				pcfg->allRules[i].updateProbability(j,prob);
			}
			newGrammarFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    	}
    }
    pcfg->prettyPrint();
    newGrammarFile.close();

    remove(string(fileIO + ".grammar").c_str());
    remove(string(fileIO + ".txt").c_str());
    remove(string(fileIO + ".grammarNew").c_str());
}