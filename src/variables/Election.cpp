/*
 * Election.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include <cace/CaceType.h>
#include <cace.h>
#include <communication/CaceCommunication.h>
#include <CaceTypes.h>
#include <variables/Election.h>
#include <limits>
#include <memory>
#include <string>
#include <vector>

using namespace std;

namespace cace
{
	string Election::electionNamespace = "e/";

	Election::Election(Cace* cace, string name, unsigned long validityTime, unsigned long decissionTime,
						unsigned long lamportAge) :
			ConsensusVariable(electionNamespace + name, acceptStrategy::TwoWayHandShakeElection, validityTime,
								cace->communication->getOwnID(), decissionTime, lamportAge, CaceType::CDouble)
	{

	}

	Election::Election(const ConsensusVariable& v) :
			ConsensusVariable(v)
	{
	}

	int Election::getWinner()
	{
		double bet = numeric_limits<double>::min();
		return getWinner(bet);
	}

	int Election::getWinner(double& bet)
	{
					int ret = 0;
			bet = numeric_limits<double>::min();
			double val = bet;

			if(hasValue) {
				getValue(&val);
				if(val > bet) {
					bet = val;
					ret = robotID;
				}
			}
			for(auto v : proposals) {
				if(v->hasValue) {
					v->getValue(&val);
					if(val > bet) {
						bet = val;
						ret = v->getRobotID();
					}
				}
			}
			return ret;
	}

} /* namespace cace */
