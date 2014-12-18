/*
 * CaceVariableSync.h
 *
 *  Created on: 04.12.2014
 *      Author: endy
 */

#ifndef CACEVARIABLESYNC_H_
#define CACEVARIABLESYNC_H_

#include "engine/constraintmodul/IVariableSyncModule.h"
#include "cace.h"

using namespace std;

namespace alicaCaceProxy
{
	class AlicaEngine;
	class Variable;
	struct SolverResult;

	class CaceVariableSync : public alica::IVariableSyncModule
	{
	public:
		CaceVariableSync(cace::Cace* cace);
		virtual ~CaceVariableSync();

		virtual void init();
		virtual void close();
		virtual void clear();
		virtual void onSolverResult(shared_ptr<SolverResult> msg);

		virtual void postResult(long vid, shared_ptr<vector<uint8_t>>& result);
		virtual shared_ptr<vector<shared_ptr<vector<shared_ptr<vector<uint8_t>>>>>> getSeeds(shared_ptr<vector<Variable*>> query, shared_ptr<vector<shared_ptr<vector<double>>>> limits);

	protected:
		cace::Cace* cace;
	};

} /* namespace alicaCaceProxy */

#endif /* CACEVARIABLESYNC_H_ */
