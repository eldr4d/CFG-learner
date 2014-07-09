#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <utility>
#include <math.h>
#include <unistd.h>

#include "Corpus/Corpus.hpp"
#include "PCFG/PCFG.hpp"
#include "CYK/CYKparser.hpp"
#include "BeamSearch/BeamSearch.hpp"
using namespace std;

#define _omphalos_ false
#define _runForLogs_ true
#define _loadGrammar_ false
#define _saveGrammar_ true
#define _groundTruth_ false
#define _generate_ false
void checkGroundTruth(PCFG learned, Corpus corp, string dirpath, string filename, double L);
void omphalosParse(PCFG omphPcfg, Corpus currCorp, string filename);

int main( int argc, const char* argv[] )
{
	
	if(argc < 3){
		cout << "Wrong arguments! Aborting!!" << endl;
		return -1;
	}
	
	//For linux
	string filename;
	string dirpath = string(argv[1]) + "/";
	filename = string(argv[2]);


	string grammarLoad = dirpath + filename + "Grammar.unreadable";
	
	//For mac
	//string dirpath = string(argv[1]) + "DataSets/";
	//string filename = "plus.txt";
 	
	string file1;
	if(_runForLogs_){
		file1 = dirpath + filename + ".learn";
	}else{
		file1 = dirpath + filename;
	}

	Corpus currCorp;
	currCorp.loadCorpusFromFile(file1, _omphalos_);
	currCorp.dropPositive();
	currCorp.normalizeCorpus();
	currCorp.printCorpus();

	cout << "--------------------Begin---------------------- " << endl;
	PCFG learnedPcfg;
	
	//Load
	if(_loadGrammar_){
		learnedPcfg.readPCFGfromFile(grammarLoad);
	}else{
		string filenameForIO = dirpath + filename; 
		learnedPcfg = searchIncrementallyForBestPCFG(currCorp, atof(argv[3]), filenameForIO);
		string g1 = dirpath + filename + "Grammar.readable";
		string g2 = dirpath + filename + "Grammar.unreadable";
		if(_saveGrammar_){
			learnedPcfg.prettyPrint(g1);
			learnedPcfg.dumpPCFGtoFile(g2);
		}
	}
	
	if(_groundTruth_){
		checkGroundTruth(learnedPcfg, currCorp, dirpath, filename, atof(argv[3]));
	}

	PCFG forPrintPCFG = learnedPcfg;
	learnedPcfg.toCnf();
	learnedPcfg.prettyPrint();
	
	map<int, char> intToSymbol =  currCorp.symbolsToChars();

 	//Create parser
 	CYKparser cyk;
		
	Corpus::words allInitWords = currCorp.getUniqueWords();
	int oCorrectWordsLearn = 0;
	int oWrongWordsLearn = 0;
	int correntNotDetectedLearn = 0;
	int wrongNotDetectedLearn = 0;
	for(unsigned int w = 0; w<allInitWords.size(); w++){
		double parseProb = cyk.parseWord(&learnedPcfg, allInitWords[w]);
		correntNotDetectedLearn = parseProb > 0 ? correntNotDetectedLearn : correntNotDetectedLearn+1;
		oCorrectWordsLearn++;
	
		cout << " Words: ";
		for(unsigned int i=0; i<allInitWords[w].size(); i++){
			cout << intToSymbol[allInitWords[w][i]] << " ";
		}
		cout << " | prob = " << parseProb << endl;
		
	}

	if(_omphalos_){
		omphalosParse(learnedPcfg, currCorp, file1);
		return 0;
	}

	if(_generate_){
		cout << "Generating!!" << endl;
		currCorp.initReduceForPCFG(&learnedPcfg);
		for(int i=0; i<100; i++){
			learnedPcfg.generateWord();
		}
		forPrintPCFG.prettyPrint();
	}

	if(_runForLogs_ == false){
		return 0;
	}

	cout << "Testing!!" << endl;
	Corpus currCorpTest;

	string file2 = dirpath + filename + ".test";
	currCorpTest.loadCorpusFromFile(file2,currCorp.charsToSymbols(), _omphalos_);
	currCorpTest.dropPositive();
	allInitWords = currCorpTest.getUniqueWords();

	int oCorrectWordsTest = 0;
	int oWrongWordsTest = 0;
	int correntNotDetectedTest = 0;
	int wrongNotDetectedTest = 0;
	for(unsigned int w = 0; w<allInitWords.size(); w++){
		double parseProb = cyk.parseWord(&learnedPcfg, allInitWords[w]);
		correntNotDetectedTest = parseProb > 0 ? correntNotDetectedTest : correntNotDetectedTest+1;
		oCorrectWordsTest++;
		
		cout << " Words: ";
		for(unsigned int i=0; i<allInitWords[w].size(); i++){
			cout << intToSymbol[allInitWords[w][i]] << " ";
		}
		cout << " | prob = " << parseProb << endl;
	}
	
	cout << "Evaluating!!" << endl;
	Corpus currCorpEval;
	string file3 = dirpath + filename + ".eval";
	currCorpEval.loadCorpusFromFile(file3,currCorp.charsToSymbols(), _omphalos_);
	allInitWords = currCorpEval.getUniqueWords();
	
	int oCorrectWordsEval = 0;
	int oWrongWordsEval = 0;
	int correntNotDetectedEval = 0;
	int wrongNotDetectedEval = 0;
	for(unsigned int w = 0; w<allInitWords.size(); w++){
		vector<int> wordTOCYK = allInitWords[w];
		wordTOCYK.erase(wordTOCYK.begin());
		double parseProb = cyk.parseWord(&learnedPcfg, wordTOCYK);
		if(allInitWords[w][0] == 1){
			correntNotDetectedEval = parseProb > 0 ? correntNotDetectedEval : correntNotDetectedEval+1;
			oCorrectWordsEval++;
		}else{
			wrongNotDetectedEval = parseProb == 0 ? wrongNotDetectedEval : wrongNotDetectedEval+1;
			oWrongWordsEval++;
		}
		cout << " Words: ";
		for(unsigned int i=1; i<allInitWords[w].size(); i++){
			cout << intToSymbol[allInitWords[w][i]] << " ";
		}
		cout << " | prob = " << parseProb << " p = " << allInitWords[w][0] << endl;
	}
	string resultsFiles = dirpath + filename + ".res";
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


void checkGroundTruth(PCFG learned, Corpus corp, string dirpath, string filename, double L){

	//learned.toCnf();
	learned.prettyPrint();
	map<int, char> intToSymbol =  corp.symbolsToChars();
 	CYKparser cyk;

 	PCFG correct;
 	correct.createTestPCFG2();
 	cout << "------------------ Learned ----------------" <<endl;
 	learned.prettyPrint();
 	//cout << learned.rulesForTermSymbol[0] << " " << learned.rulesForTermSymbol[1] << " " << learned.rulesForTermSymbol[2]  << endl;
 	cout << "------------------ Correct ----------------" <<endl;
 	correct.prettyPrint();
 	//cout << correct.rulesForTermSymbol[0] << " " << correct.rulesForTermSymbol[1] << " " << correct.rulesForTermSymbol[2]  << endl;
 	
 	correct.toCnf();

	corp.initReduceForPCFG(&learned);
	
	learned.pruneProductions(0.0001);

	//exit(0);
	int total = 10000;
	int wrong = 0;
	int wrong2 = 0;
	for(int i=0; i<total; i++){
		vector<int> word = learned.generateWord();
		double prob = cyk.parseWord(&correct, word);
		if(prob <= 0){
			cout << "Wrong word generated: " << prob << endl;
			wrong++;
		}
	}
	cout << "Change roles" << endl;
	PCFG learned2 =learned;
	learned2.toCnf();
	for(int i=0; i<total; i++){
		vector<int> word = correct.generateWord();
		double prob = cyk.parseWord(&learned2,word);
		if(prob <= 0){
			cout << "Wrong word generated: " << prob << endl;
			wrong2++;
		}
	}
	ostringstream g1;
	g1 << dirpath << "/results/" << "strange" << "Grammar" << L << ".readable";
	ostringstream g2;
	g2 << dirpath << "/results/"  << "strange" << "Grammar" << L << ".unreadable";
	learned.prettyPrint(g1.str());
	learned.dumpPCFGtoFile(g2.str());


	ostringstream resultsFiles;
	resultsFiles << dirpath  << "/results/"  << "strange.res";
	ofstream myfile(resultsFiles.str().c_str(), std::ios_base::app);
	myfile << L << " Learned genearated: " <<  wrong << " " << total  << " Original genearated: " <<  wrong2 << " " << total << endl;
	myfile.close();

 	correct.prettyPrint();

	exit(0);
}

void omphalosParse(PCFG omphPcfg, Corpus currCorp, string filename){
	Corpus omphalosCorp;
 	CYKparser cyk;

	omphPcfg.toCnf();
	string omphTest = filename;
	size_t start_pos = omphTest.find("learn");
    omphTest.replace(start_pos, 5, "test");

	omphalosCorp.loadCorpusFromFile(omphTest, currCorp.charsToSymbols(), _omphalos_);
	omphalosCorp.dropPositive();

	Corpus::words allInitWords = omphalosCorp.getUniqueWords();
	cout << "Omphalos results: " << endl;
	for(unsigned int w = 0; w<allInitWords.size(); w++){
		double parseProb = cyk.parseWord(&omphPcfg, allInitWords[w]);
		if(parseProb > 0.0){
			cout << 1 << " ";
		}else{
			cout << 0 << " ";
		}
	}
	cout << endl;
}

