/*
 * Election.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef ELECTION_H_
#define ELECTION_H_

#include <iostream>
#include "ConsensusVariable.h"

using namespace std;


namespace cace
{
	class Cace;
} /* namespace cace */

namespace cace
{

	class Election : public ConsensusVariable
	{
	public:
		Election(Cace* cace, string name, unsigned long validityTime, unsigned long decissionTime, unsigned long lamportAge);
		Election(const ConsensusVariable& v);

		int getWinner();
		int getWinner(double& bet);

		static string electionNamespace;
	};

} /* namespace cace */

#endif /* ELECTION_H_ */
