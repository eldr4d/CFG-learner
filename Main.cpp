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
#include "BeamSearch/BeamSearch.hpp"
using namespace std;


int main( int argc, const char* argv[] )
{
	
	if(argc != 3){
		cout << "Wrong arguments! Aborting!!" << endl;
		return -1;
	}
	
	//For linux
	string filename;
	string filepath = string(argv[1]) + "/";
	filename = string(argv[2]);
	
	//For mac
	//string filepath = string(argv[1]) + "DataSets/";
	//string filename = "plus.txt";
	
 	
 	
	//Create corpus
	Corpus currCorp;
	//string file1 = filepath + filename + ".learn";
	string file1 = filepath + filename;
	currCorp.loadCorpusFromFile(file1);
	currCorp.normalizeCorpus();
	currCorp.printCorpus();
	//return 0;
	cout << "--------------------Begin---------------------- " << endl;
	PCFG learnedPcfg;
	
	//Load
	//string g1 = filepath + filename + "Grammar.unreadable";
	//learnedPcfg.readPCFGfromFile(g1);
	
	
	//Learn
	//learnedPcfg = searchForBestPCFG(currCorp);
	learnedPcfg = searchIncrementallyForBestPCFG(currCorp);
	//string g1 = filepath + filename + "Grammar.readable";
	//string g2 = filepath + filename + "Grammar.unreadable";
	//learnedPcfg.prettyPrint(g1);
	//learnedPcfg.dumpPCFGtoFile(g2);
	
	PCFG forPrintPCFG = learnedPcfg;
	learnedPcfg.toCnf();
	learnedPcfg.prettyPrint();
	
	map<int, char> intToSymbol =  currCorp.symbolsToChars();

 	//Create parser
 	CYKparser cyk;
		
	Corpus::words allInitWords = currCorp.getUniqueWords();
	vector<char> positive = currCorp.getPositiveStatus();
	int oCorrectWordsLearn = 0;
	int oWrongWordsLearn = 0;
	int correntNotDetectedLearn = 0;
	int wrongNotDetectedLearn = 0;
	for(unsigned int w = 0; w<allInitWords.size(); w++){
		double parseProb = cyk.parseWord(&learnedPcfg, allInitWords[w]);
		if((bool)positive[w] == true){
			correntNotDetectedLearn = parseProb > 0 ? correntNotDetectedLearn : correntNotDetectedLearn+1;
			oCorrectWordsLearn++;
		}else{
			wrongNotDetectedLearn = parseProb == 0 ? wrongNotDetectedLearn : wrongNotDetectedLearn+1;
			oWrongWordsLearn++;
		}
		cout << " Words: ";
		for(unsigned int i=0; i<allInitWords[w].size(); i++){
			cout << intToSymbol[allInitWords[w][i]] << " ";
		}
		cout << " | prob = " << parseProb << " p = " << (bool)positive[w] << endl;
		
	}
	
	vector<int> table;
	table.push_back(0);
	table.push_back(0);
	table.push_back(1);
	table.push_back(1);
	table.push_back(1);
	double parseProb = cyk.parseWord(&learnedPcfg, table);
	cout << " Words: ";
	for(unsigned int i=0; i<table.size(); i++){
		cout << intToSymbol[table[i]] << " ";
	}
	cout << " | prob = " << parseProb << endl;

	cout << "Generating!!" << endl;
	currCorp.initReduceForPCFG(&learnedPcfg);
	for(int i=0; i<100; i++){
		learnedPcfg.generateWord(currCorp.numberOfSymbolsInCorpus());
	}
	forPrintPCFG.prettyPrint();
	return 0;
	
	cout << "Testing!!" << endl;
	Corpus currCorpTest;

	string file2 = filepath + filename + ".test";
	currCorpTest.loadCorpusFromFile(file2);
	allInitWords = currCorpTest.getUniqueWords();
	positive = currCorpTest.getPositiveStatus();

	int oCorrectWordsTest = 0;
	int oWrongWordsTest = 0;
	int correntNotDetectedTest = 0;
	int wrongNotDetectedTest = 0;
	for(unsigned int w = 0; w<allInitWords.size(); w++){
		double parseProb = cyk.parseWord(&learnedPcfg, allInitWords[w]);
		if((bool)positive[w] == true){
			correntNotDetectedTest = parseProb > 0 ? correntNotDetectedTest : correntNotDetectedTest+1;
			oCorrectWordsTest++;
		}else{
			wrongNotDetectedTest = parseProb == 0 ? wrongNotDetectedTest : wrongNotDetectedTest+1;
			oWrongWordsTest++;
		}
		cout << " Words: ";
		for(unsigned int i=0; i<allInitWords[w].size(); i++){
			cout << intToSymbol[allInitWords[w][i]] << " ";
		}
		cout << " | prob = " << parseProb << " p = " << (bool)positive[w] << endl;
	}
	
	cout << "Evaluating!!" << endl;
	Corpus currCorpEval;
	string file3 = filepath + filename + ".eval";
	currCorpEval.loadCorpusFromFile(file3);
	allInitWords = currCorpEval.getUniqueWords();
	positive = currCorpEval.getPositiveStatus();
	
	int oCorrectWordsEval = 0;
	int oWrongWordsEval = 0;
	int correntNotDetectedEval = 0;
	int wrongNotDetectedEval = 0;
	for(unsigned int w = 0; w<allInitWords.size(); w++){
		double parseProb = cyk.parseWord(&learnedPcfg, allInitWords[w]);
		if((bool)positive[w] == true){
			correntNotDetectedEval = parseProb > 0 ? correntNotDetectedEval : correntNotDetectedEval+1;
			oCorrectWordsEval++;
		}else{
			wrongNotDetectedEval = parseProb == 0 ? wrongNotDetectedEval : wrongNotDetectedEval+1;
			oWrongWordsEval++;
		}
		cout << " Words: ";
		for(unsigned int i=0; i<allInitWords[w].size(); i++){
			cout << intToSymbol[allInitWords[w][i]] << " ";
		}
		cout << " | prob = " << parseProb << " p = " << (bool)positive[w] << endl;
	}
	string resultsFiles = filepath + filename + ".res";
	ofstream myfile(resultsFiles.c_str());
	myfile << "-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-" << endl;
	myfile << "Final Results: " << endl;
	myfile <<	"Correct learn not detected: " << correntNotDetectedLearn << " out of: " << oCorrectWordsLearn << endl;
	myfile <<	"Wrong learn not detected: " << wrongNotDetectedLearn << " out of: " << oWrongWordsLearn << endl;
	myfile <<	"Correct test not detected: " << correntNotDetectedTest << " out of: " << oCorrectWordsTest << endl;
	myfile <<	"Wrong test not detected: " << wrongNotDetectedTest << " out of: " << oWrongWordsTest << endl;
	myfile <<	"Correct eval not detected: " << correntNotDetectedEval << " out of: " << oCorrectWordsEval << endl;
	myfile <<	"Wrong eval not detected: " << wrongNotDetectedEval << " out of: " << oWrongWordsEval << endl;
	myfile.close();
	
	/*cout << "-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-" << endl;
	cout << "Final Results: " << endl;
	cout <<	"Correct learn not detected: " << correntNotDetectedLearn << " out of: " << oCorrectWordsLearn << endl;
	cout <<	"Wrong learn not detected: " << wrongNotDetectedLearn << " out of: " << oWrongWordsLearn << endl;
	cout <<	"Correct test not detected: " << correntNotDetectedTest << " out of: " << oCorrectWordsTest << endl;
	cout <<	"Wrong test not detected: " << wrongNotDetectedTest << " out of: " << oWrongWordsTest << endl;
	cout <<	"Correct eval not detected: " << correntNotDetectedEval << " out of: " << oCorrectWordsEval << endl;
	cout <<	"Wrong eval not detected: " << wrongNotDetectedEval << " out of: " << oWrongWordsEval << endl;*/
	return 0;
}



