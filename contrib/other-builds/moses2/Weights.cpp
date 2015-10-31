/*
 * Weights.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#include <cassert>
#include <string>
#include <vector>
#include "FeatureFunction.h"
#include "FeatureFunctions.h"
#include "Weights.h"
#include "moses/Util.h"

using namespace std;

Weights::Weights()
{
	// TODO Auto-generated constructor stub

}

Weights::~Weights() {
	// TODO Auto-generated destructor stub
}

void Weights::Init(const FeatureFunctions &ffs)
{
	size_t totalNumScores = ffs.GetNumScores();
	//cerr << "totalNumScores=" << totalNumScores << endl;
	m_weights.resize(totalNumScores, 1);
}

void Weights::Debug(std::ostream &out, const FeatureFunctions &ffs) const
{
	size_t numScores = ffs.GetNumScores();
	for (size_t i = 0; i < numScores; ++i) {
		out << m_weights[i] << " ";
	}

}

std::ostream& operator<<(std::ostream &out, const Weights &obj)
{

	return out;
}

void Weights::CreateFromString(const FeatureFunctions &ffs, const std::string &line)
{
	std::vector<std::string> toks = Moses::Tokenize(line);
	assert(toks.size());

	string ffName = toks[0];
	assert(ffName.back() == '=');

	ffName = ffName.substr(0, ffName.size() - 1);
	//cerr << "ffName=" << ffName << endl;

	const FeatureFunction &ff = ffs.FindFeatureFunction(ffName);
	size_t startInd = ff.GetStartInd();
	size_t numScores = ff.GetNumScores();
	assert(numScores == toks.size() -1);

	for (size_t i = 0; i < numScores; ++i) {
		SCORE score = Moses::Scan<SCORE>(toks[i+1]);
		m_weights[i + startInd] = score;
	}
}