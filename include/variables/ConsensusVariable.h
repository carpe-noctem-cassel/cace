/*
 * ConsensusVariable.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef CONSENSUSVARIABLE_H_
#define CONSENSUSVARIABLE_H_

#include <iostream>
#include <vector>
#include <limits>

#include "cace.h"


using namespace std;


namespace cace
{
	class ConsensusVariable;
	typedef bool (ConsensusVariable::*t_acceptFunction)(Cace&, vector<char>*);

	enum  acceptStrategy
	{
		NoDistribution=0,
		FireAndForgetSet=8,
		FireAndForgetElection=9,
		//Default has to be the highest ID!
		FireAndForget=10,
		TwoWayHandShakeSet=36,
		TwoWayHandShakeElection=37,
		TwoWayHandShakeMostRecent=38,
		TwoWayHandShakeLowestID=39,
		//Default has to be the highest ID!
		TwoWayHandShake=40,
		ThreeWayHandShakeSet=66,
		ThreeWayHandShakeElection=67,
		ThreeWayHandShakeMostRecent=68,
		ThreeWayHandShakeLowestID=69,
		//Default has to be the highest ID!
		ThreeWayHandShake=70
	};

	class ConsensusVariable
	{
	public:
		ConsensusVariable(const ConsensusVariable& v);
		ConsensusVariable (string name, acceptStrategy strategy, unsigned long validityTime, int robotID, unsigned long decissionTime, unsigned long lamportAge, short type);
		virtual ~ConsensusVariable();

		void update(ConsensusVariable& v);
		bool valueEqual(vector<char>* cmp);
		bool believeEqual(ConsensusVariable& v);
		bool isAcknowledged(Cace& c);
		bool isAgreed(Cace& c);
		bool checkConflict(Cace& c);
		string getScope();


		vector<char> getValue();
		void setValue(vector<char> value);
		string& getName();
		void setName(string name);

		short getType();
		void setType(short t);
		bool hasValue();
		int getRobotID();
		void setRobotID(int id);
		unsigned long getArrivalTime();
		void setArrivalTime(unsigned long at);
		unsigned long getDecissionTime();
		void setDecissionTime(unsigned long dt);
		unsigned long getValidityTime();
		void setValidityTime(unsigned long vt);
		unsigned long getLamportAge();
		void setLamportAge(unsigned long la);

		acceptStrategy getAcceptStrategy();
		void setAcceptStrategy(acceptStrategy aS);
		t_acceptFunction acceptFunction;
		void setAcceptFunction(t_acceptFunction func) {acceptFunction = func;};
		vector<ConsensusVariable> proposals;

		string valueAsString();
		string toString();

		bool getValue(double* out);
		void setValue(double in);

		bool defaultAcceptStrategy(Cace &c, vector<char>* commandedValue);
		bool lowestIDAcceptStrategy(Cace &c, vector<char>* commandedValue);
		bool mostRecentAcceptStrategy(Cace &c, vector<char>* commandedValue);
		bool electionAcceptStrategy(Cace &c, vector<char>* commandedValue);
		bool setAcceptStrategy(Cace &c, vector<char>* commandedValue);

	protected:
		vector<char> val;
		string name;
		short type;
		bool hasVal;
		int robotID;
		unsigned long arrivalTime;
		unsigned long decissionTime;
		unsigned long validityTime;
		unsigned long lamportAge;

		acceptStrategy strategy;

	};

} /* namespace cace */

#endif /* CONSENSUSVARIABLE_H_ */
