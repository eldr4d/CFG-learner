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
#include "Quantizer/Quantizer.hpp"

using namespace std;

int main( int argc, char* argv[] )
{
	
	vector<string> learnFiles,testFiles,evalFiles;
	string dirpath;
	string filename;
	bool loadGrammarFlag = false;
	int wordSize = -1;
	int symbols = -1;
	int split = -1;
	char c;

	while ((c = getopt (argc, argv, "l:t:e:f:d:rp:w:s:")) != -1){
		switch(c){
			case 'l':
				learnFiles.push_back(string(optarg));
				break;
			case 't':
				testFiles.push_back(string(optarg));
				break;
			case 'e':
				evalFiles.push_back(string(optarg));
				break;
			case 'f':
				filename = string(optarg);
				break;
			case 'd':
				dirpath = string(optarg);
				break;
			case 'r':
				loadGrammarFlag = true;
				break;
			case 'w':
				wordSize = atoi(optarg);
				break;
			case 's':
				symbols = atoi(optarg);
				break;
			case 'p':
				split = atoi(optarg);
				break;
			default:
				cout << "Wrong arguments, aborting!" << endl;
				return -1;
		}
	}
	if(loadGrammarFlag == 0 && (learnFiles.size() == 0 || testFiles.size() == 0 || evalFiles.size() == 0 || symbols == -1 || wordSize == -1 || split == -1)){
		cout << "Wrong arguments, aborting!" << endl;
		return -1;
	}else if(filename == ""){
		cout << "No filename, aborting!" << endl;
		return -1;
	}
	vector<string> allFiles;
	allFiles.insert(allFiles.end(),learnFiles.begin(),learnFiles.end());
	allFiles.insert(allFiles.end(),testFiles.begin(),testFiles.end());
	allFiles.insert(allFiles.end(),evalFiles.begin(),evalFiles.end());
	Quantizer quant;

	Quantizer::SPLIT splitMethod;
	if(split == 0){
		splitMethod = Quantizer::standard;
	}else if(split == 1){
		splitMethod = Quantizer::uniform;
	}else if(split == 2){
		splitMethod = Quantizer::normal;
	}else{
		cout << "Wrong split, aborting!" << endl;
		return -1;
	}
	for(unsigned int i=0; i<allFiles.size(); i++){
		allFiles[i] = dirpath + allFiles[i];
	}
	cout << "Directory: " << dirpath << endl;
	cout << "Filename: " << dirpath+filename << endl;

	//Quantize the data from the noptilus missions
	string corpusFileName = quant.quantizeDataAndWriteToFile(allFiles, dirpath, filename, learnFiles.size(), testFiles.size(), evalFiles.size(), symbols, wordSize, splitMethod);

	string grammarLoad = corpusFileName + "Grammar.unreadable";
	string fileLearn = corpusFileName + ".learn";

	//Initialize the corpus
	Corpus currCorp;
	currCorp.loadCorpusFromFile(fileLearn, false);
	currCorp.normalizeCorpus();
	currCorp.printCorpus();

	cout << "--------------------Begin---------------------- " << endl;
	PCFG learnedPcfg;
	
	//Load
	if(loadGrammarFlag){
		learnedPcfg.readPCFGfromFile(grammarLoad);
	}else{
		string filenameForIO = corpusFileName; 
		double L = 0.001;
		learnedPcfg = searchIncrementallyForBestPCFG(currCorp, L, filenameForIO);
		string g1 = corpusFileName + "Grammar.readable";
		string g2 = corpusFileName + "Grammar.unreadable";
		learnedPcfg.prettyPrint(g1);
		learnedPcfg.dumpPCFGtoFile(g2);
	}
	
	PCFG forPrintPCFG = learnedPcfg;
	learnedPcfg.toCnf();
	learnedPcfg.prettyPrint();
	
	map<int, char> intToSymbol =  currCorp.symbolsToChars();

 	//Create parser
 	CYKparser cyk;
		
	Corpus::words allInitWords = currCorp.getInitUnsortedWords();
	int oCorrectWordsLearn = 0;
	int oWrongWordsLearn = 0;
	int correntNotDetectedLearn = 0;
	int wrongNotDetectedLearn = 0;
	for(unsigned int w = 0; w<allInitWords.size(); w++){
		double parseProb = cyk.parseWord(&learnedPcfg, allInitWords[w].word);
		correntNotDetectedLearn = parseProb > 0 ? correntNotDetectedLearn : correntNotDetectedLearn+1;
		oCorrectWordsLearn++;
	
		cout << " Words: ";
		for(unsigned int i=0; i<allInitWords[w].word.size(); i++){
			cout << intToSymbol[allInitWords[w].word[i]] << " ";
		}
		cout << " | prob = " << parseProb << endl;
		
	}
	forPrintPCFG.prettyPrint();

	cout << "Testing!!" << endl;
	Corpus currCorpTest;

	string fileTest = corpusFileName + ".test";
	currCorpTest.loadCorpusFromFile(fileTest,currCorp.charsToSymbols(), false);
	allInitWords = currCorpTest.getInitUnsortedWords();

	int oCorrectWordsTest = 0;
	int oWrongWordsTest = 0;
	int correntNotDetectedTest = 0;
	int wrongNotDetectedTest = 0;

	string fileForMatlabTest = corpusFileName + ".mtest";

	ofstream matlabFileTest;
	matlabFileTest.open(fileForMatlabTest.c_str());
	for(unsigned int w = 0; w<allInitWords.size(); w++){
		double parseProb = cyk.parseWord(&learnedPcfg, allInitWords[w].word);
		correntNotDetectedTest = parseProb > 0 ? correntNotDetectedTest : correntNotDetectedTest+1;
		oCorrectWordsTest++;
		
		cout << " Words: ";
		for(unsigned int i=0; i<allInitWords[w].word.size(); i++){
			cout << intToSymbol[allInitWords[w].word[i]] << " ";
		}
		cout << " | prob = " << parseProb << endl;
		matlabFileTest << allInitWords[w].word.size() << " " << parseProb << " " << allInitWords[w].positive << endl;
	}
	matlabFileTest.close();



	cout << "Evaluating!!" << endl;
	Corpus currCorpEval;
	string fileEval = corpusFileName + ".eval";
	currCorpEval.loadCorpusFromFile(fileEval,currCorp.charsToSymbols(), false);
	allInitWords = currCorpEval.getInitUnsortedWords();
	
	int oCorrectWordsEval = 0;
	int oWrongWordsEval = 0;
	int correntNotDetectedEval = 0;
	int wrongNotDetectedEval = 0;


	string fileForMatlabEval = corpusFileName + ".meval";
	ofstream matlabFileEval;
	matlabFileEval.open(fileForMatlabEval.c_str());
	for(unsigned int w = 0; w<allInitWords.size(); w++){
		vector<int> wordTOCYK = allInitWords[w].word;
		double parseProb = cyk.parseWord(&learnedPcfg, wordTOCYK);
		if(allInitWords[w].positive){
			correntNotDetectedEval = parseProb > 0 ? correntNotDetectedEval : correntNotDetectedEval+1;
			oCorrectWordsEval++;
		}else{
			wrongNotDetectedEval = parseProb <= 10e-12 ? wrongNotDetectedEval : wrongNotDetectedEval+1;
			oWrongWordsEval++;
		}
		cout << " Words: ";
		for(unsigned int i=0; i<allInitWords[w].word.size(); i++){
			cout << intToSymbol[allInitWords[w].word[i]] << " ";
		}
		cout << " | prob = " << parseProb << " p = " << allInitWords[w].positive << endl;
		matlabFileEval << allInitWords[w].word.size() << " " << parseProb << " " << allInitWords[w].positive << endl;
	}
	matlabFileEval.close();

	string resultsFiles = corpusFileName + ".res";
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

