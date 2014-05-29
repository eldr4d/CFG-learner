#ifndef BEAM_HPP
#define BEAM_HPP

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <functional>

#include "../PCFG/PCFG.hpp"
#include "../Corpus/Corpus.hpp"
#include "../CYK/CYKparser.hpp"

#define L 0.01//50.0/10000.0
#define L2 1.0

typedef struct{
	PCFG pcfg;
	double currGain;
	double currPriori,currLikelihood;
	bool isChunk;
	bool conflict;
	bool expanded;
	int mergedId1,mergedId2;
}searchNode;

class mycomparison
{
public:
	mycomparison(){}
	bool operator() (const searchNode sN1, const searchNode sN2) const
	{
 			return sN1.currGain < sN2.currGain;
	}
};

PCFG searchForBestPCFG(Corpus corpus);
PCFG searchIncrementallyForBestPCFG(Corpus corpus);
searchNode doBeamSearch(searchNode root, int width, int depth);
std::vector<searchNode> findAllPossibleMoves(searchNode parent);
void doChunk(std::vector<searchNode> *listOfNodes, searchNode parent);
void doMerge(std::vector<searchNode> *listOfNodes, searchNode parent);

void recalculateProbabilities(Corpus *corpus, Corpus::words *allInitWords, Corpus::words *allWords, std::vector<double> *wordCount, PCFG *pcfg);
#endif //BEAM_HPP