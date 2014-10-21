#ifndef QUANTIZER_HPP
#define QUANTIZER_HPP
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <limits>
#include <time.h>
#include <random>

using namespace std;
class Quantizer{

public:
	enum SPLIT {standard=0,uniform,normal,split_size};
private:
	void readAllFiles(vector<string> allFiles, vector<vector<double> > *allValues, double *min, double *max, int boundOfMinMax){
		*min = 100000;
		*max = -100000;
		for(unsigned int i = 0; i<allFiles.size(); i++){
			ifstream myfile (allFiles[i]);
			vector<double> tempV;
			while(myfile.is_open() && !myfile.eof()){
				double tmp;
				myfile >> tmp;
				tempV.push_back(tmp);
				if(i < boundOfMinMax){
					if(tmp < *min){
						*min = tmp;
					}
					if(tmp > *max){
						*max = tmp;
					}
				}
				//cout << tempV.size() << endl;
			}
			allValues->push_back(tempV);
			myfile.close();
		}
	}

	void createQuantizationFragments(vector<double> *frags, double min, double max, int symbols){
		frags->push_back(-std::numeric_limits<double>::infinity());
		double step = (max-min)/(double)symbols;
		for(int i=0; i<=symbols; i++){
			frags->push_back(min + step*((double)i));
		}
		frags->push_back(std::numeric_limits<double>::infinity());
	}

	void quantizeStreams(vector<vector<double> > *instream, vector<vector<char> > *outstream, vector<double> quantizationFragments){
		for(unsigned int i=0; i<instream->size(); i++){
			vector<char> tmpCharStream;
			for(unsigned int j=0; j<instream->at(i).size(); j++){
				for(unsigned int k=1; k<quantizationFragments.size(); k++){
					if(quantizationFragments[k-1] <= instream->at(i)[j] && quantizationFragments[k] > instream->at(i)[j]){
						tmpCharStream.push_back('a' + k - 1);
						break;
					}
				}
			}
			outstream->push_back(tmpCharStream);
		}

	}

	void breakStreamsIntoWords(vector<vector<vector<char> > > *outWords, vector<vector<char> > *quantizedValues, int wordSize, SPLIT breakMethod){
		default_random_engine generator;
		normal_distribution<double> distribution(wordSize,wordSize/2);

		for(unsigned int i=0; i<quantizedValues->size();i++){
			int start = 0;
			int end=0;
			int k=0;
			vector<vector<char> > wordsOfStream;
			while(start < quantizedValues->at(i).size()-1){
				if(breakMethod == standard){
					end = start + wordSize;
				}else if(breakMethod == uniform){
					int max = wordSize + wordSize/2;
					int min = wordSize - wordSize/2;
					end = start + rand()%(max-min+1) + min;
				}else if(breakMethod == normal){
					do{
						end = start + distribution(generator);
					}while(end - start < 2);
				}
				if(end >= quantizedValues->at(i).size()){
					end = quantizedValues->at(i).size()-1;
				}

				vector<char> tmpWord;
				for(unsigned j=start; j<end; j++){
					tmpWord.push_back(quantizedValues->at(i)[j]);
				}
				wordsOfStream.push_back(tmpWord);

				start = end;
				k++;
			}
			outWords->push_back(wordsOfStream);
		}
	}

	string writeWordsIntoFiles(vector<vector<vector<char> > > *words, vector<double> quantizeFragments, int wordSize, int symbols, SPLIT breakMethod, string fileLocation){
		string file = fileLocation + "_SY" + to_string(symbols) + "_SI" + to_string(wordSize) + "_";
		if(breakMethod == standard){
			file += "ST";
		}else if(breakMethod == uniform){
			file += "UN";
		}else{
			file += "NO";
		}
		for(unsigned int i=0; i<words->size(); i++){
			string tmp = file;
			if(i == 0){
				tmp += ".learn";
			}else if(i == 1){
				tmp += ".test";
			}else{
				tmp += ".eval";
			}
			ofstream myfile (tmp);
			for(unsigned int j=0; j<words->at(i).size(); j++){
				myfile << 1 << " ";
				for(unsigned int k=0; k<words->at(i)[j].size(); k++){
					if(k == 0){
						myfile << words->at(i)[j][k];
					}else{
						myfile << " " << words->at(i)[j][k];
					}
				}
				myfile << endl;
			}
			myfile.close();
		}
		string tmpfile = file;
		tmpfile += ".setup";
		ofstream myfile (tmpfile);
		myfile << quantizeFragments.size() << endl;
		for(unsigned int i=0; i<quantizeFragments.size(); i++){
			myfile << quantizeFragments[i] << endl;
			if(i != quantizeFragments.size()-1){
				myfile << char('a' + i) << endl;
			}
		}
		myfile.close();
		return file;
	}

public:
	string quantizeDataAndWriteToFile(vector<string> allFiles, string dirpath, string writeFilePrefix, int numberOfLearn, int numberOfTest, int numberOfEval, int symbols, int wordSize, SPLIT splitMethod){
		srand (time(NULL));

		vector<vector<double> > allValues;
		double min, max;
		readAllFiles(allFiles, &allValues, &min, &max, numberOfLearn+numberOfTest);


		vector<double> quantizationFragments;
		createQuantizationFragments(&quantizationFragments, min, max, symbols);

		vector<vector<char> > quantizedValues;
		quantizeStreams(&allValues, &quantizedValues, quantizationFragments);

		vector<vector<vector<char> > > breakedWords;
		breakStreamsIntoWords(&breakedWords, &quantizedValues, wordSize, splitMethod);

		//Merge all learn words into one vector
		vector<vector<vector<char> > > mergedBreakedWords;
		vector<vector<char> > tempV;

		for(unsigned int i=0; i<breakedWords.size(); i++){
			tempV.insert(tempV.end(), breakedWords[i].begin(), breakedWords[i].end());
			if(i == numberOfLearn-1 || i == numberOfLearn + numberOfTest - 1 || i == numberOfLearn+numberOfTest+numberOfEval-1){
				mergedBreakedWords.push_back(tempV);
				tempV.clear();
			}
		}
		string filename = writeWordsIntoFiles(&mergedBreakedWords, quantizationFragments, wordSize, symbols, splitMethod, dirpath + writeFilePrefix);
		return filename;
	}
};


#endif //QUANTIZER_HPP