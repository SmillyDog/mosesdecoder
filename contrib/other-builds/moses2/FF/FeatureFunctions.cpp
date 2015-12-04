/*
 * FeatureFunctions.cpp
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include "FeatureFunctions.h"
#include "StatefulFeatureFunction.h"
#include "../System.h"
#include "../Scores.h"
#include "../MemPool.h"

#include "SkeletonStatelessFF.h"
#include "SkeletonStatefulFF.h"
#include "WordPenalty.h"
#include "PhrasePenalty.h"
#include "Distortion.h"
#include "../TranslationModel/PhraseTableMemory.h"
#include "../TranslationModel/ProbingPT.h"
#include "../TranslationModel/UnknownWordPenalty.h"
#include "../LM/LanguageModel.h"
#include "../LM/KENLM.h"
#include "util/exception.hh"

using namespace std;

FeatureFunctions::FeatureFunctions(System &system)
:m_system(system)
,m_ffStartInd(0)
{
	// TODO Auto-generated constructor stub

}

FeatureFunctions::~FeatureFunctions() {
	RemoveAllInColl(m_featureFunctions);
}

void FeatureFunctions::Create()
{
  const Parameter &params = m_system.params;

  const PARAM_VEC *ffParams = params.GetParam("feature");
  UTIL_THROW_IF2(ffParams == NULL, "Must have [feature] section");

  BOOST_FOREACH(const std::string &line, *ffParams) {
	  cerr << "line=" << line << endl;
	  FeatureFunction *ff = Create(line);

	  m_featureFunctions.push_back(ff);

	  StatefulFeatureFunction *sfff = dynamic_cast<StatefulFeatureFunction*>(ff);
	  if (sfff) {
		  sfff->SetStatefulInd(m_statefulFeatureFunctions.size());
		  m_statefulFeatureFunctions.push_back(sfff);
	  }

	  PhraseTable *pt = dynamic_cast<PhraseTable*>(ff);
	  if (pt) {
		  pt->SetPtInd(m_phraseTables.size());
		  m_phraseTables.push_back(pt);
	  }
  }
}

void FeatureFunctions::Load()
{
  // load, everything but pts
  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
	  FeatureFunction *nonConstFF = const_cast<FeatureFunction*>(ff);
	  PhraseTable *pt = dynamic_cast<PhraseTable*>(nonConstFF);

	  if (pt) {
		  // do nothing. load pt last
	  }
	  else {
		  cerr << "Loading " << nonConstFF->GetName() << endl;
		  nonConstFF->Load(m_system);
		  cerr << "Finished loading " << nonConstFF->GetName() << endl;
	  }
  }

  // load pt
  BOOST_FOREACH(const PhraseTable *pt, m_phraseTables) {
	  PhraseTable *nonConstPT = const_cast<PhraseTable*>(pt);
	  cerr << "Loading " << nonConstPT->GetName() << endl;
	  nonConstPT->Load(m_system);
	  cerr << "Finished loading " << nonConstPT->GetName() << endl;
  }
}

FeatureFunction *FeatureFunctions::Create(const std::string &line)
{
	vector<string> toks = Tokenize(line);

	FeatureFunction *ret;
	if (toks[0] == "PhraseDictionaryMemory") {
		ret = new PhraseTableMemory(m_ffStartInd, line);
	}
	else if (toks[0] == "ProbingPT") {
		ret = new ProbingPT(m_ffStartInd, line);
	}
	else if (toks[0] == "UnknownWordPenalty") {
		ret = new UnknownWordPenalty(m_ffStartInd, line);
	}
	else if (toks[0] == "WordPenalty") {
		ret = new WordPenalty(m_ffStartInd, line);
	}
	else if (toks[0] == "Distortion") {
		ret = new Distortion(m_ffStartInd, line);
	}
	else if (toks[0] == "LanguageModel") {
		ret = new LanguageModel(m_ffStartInd, line);
	}
	else if (toks[0] == "KENLM") {
		ret = new KENLM(m_ffStartInd, line);
		//ret = new KENLMBatch(m_ffStartInd, line);
	}
	else if (toks[0] == "PhrasePenalty") {
		ret = new PhrasePenalty(m_ffStartInd, line);
		//ret = new KENLMBatch(m_ffStartInd, line);
	}
	else {
		//ret = new SkeletonStatefulFF(m_ffStartInd, line);
		ret = new SkeletonStatelessFF(m_ffStartInd, line);
	}

	m_ffStartInd += ret->GetNumScores();

	if (ret->HasVocabInd()) {
		ret->SetVocabInd(hasVocabInd.size());
		hasVocabInd.push_back(ret);
	}

	return ret;
}

const FeatureFunction &FeatureFunctions::FindFeatureFunction(const std::string &name) const
{
  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
	  if (ff->GetName() == name) {
		  return *ff;
	  }
  }
  UTIL_THROW2(name << " not found");

}

const PhraseTable *FeatureFunctions::GetPhraseTablesExcludeUnknownWordPenalty(size_t ptInd)
{
	// assume only 1 unk wp
	std::vector<const PhraseTable*> tmpVec(m_phraseTables);
	std::vector<const PhraseTable*>::iterator iter;
	for (iter = tmpVec.begin(); iter != tmpVec.end(); ++iter) {
		const PhraseTable *pt = *iter;
		  const UnknownWordPenalty *unkWP = dynamic_cast<const UnknownWordPenalty *>(pt);
		  if (unkWP) {
			  tmpVec.erase(iter);
			  break;
		  }
	}

	const PhraseTable *pt = tmpVec[ptInd];
	return pt;
}

void
FeatureFunctions::EvaluateInIsolation(MemPool &pool, const System &system,
		  const Phrase &source, TargetPhrase &targetPhrase) const
{
  size_t numScores = system.featureFunctions.GetNumScores();
  Scores *estimatedScores = new (pool.Allocate<Scores>()) Scores(system, pool, numScores);

  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
	  Scores& scores = targetPhrase.GetScores();
	  ff->EvaluateInIsolation(system, source, targetPhrase, scores, estimatedScores);

  }

  if (estimatedScores) {
	  targetPhrase.SetEstimatedScore(estimatedScores->GetTotalScore());
  }
}