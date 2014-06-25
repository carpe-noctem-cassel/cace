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

#include <cace/CaceType.h>

#include "cace.h"
#include "CaceTypes.h"
#include "serializer/serialize.h"
#include "util/Delegate.h"

using namespace std;

namespace cace
{
	class ConsensusVariable;
	class Cace;

	typedef bool (ConsensusVariable::*t_acceptFunction)(Cace&, vector<uint8_t>*);

	class ConsensusVariable
	{
	public:
		ConsensusVariable(const ConsensusVariable& v);
		ConsensusVariable(string name, acceptStrategy strategy, unsigned long validityTime, int robotID,
							unsigned long decissionTime, unsigned long lamportAge, short type);
		virtual ~ConsensusVariable();

		void update(ConsensusVariable& v);
		bool valueEqual(vector<uint8_t>* cmp);
		bool believeEqual(ConsensusVariable& v);
		bool isAcknowledged(Cace& c);
		bool isAgreed(Cace& c);
		bool checkConflict(Cace& c);
		string getScope();

		vector<uint8_t> getValue();
		void setValue(vector<uint8_t> value);
		string& getName();
		void setName(string name);

		short getType();
		void setType(short t);
		bool hasValue;
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
		bool acceptProposals(Cace& cace, vector<uint8_t>* value);
		void setAcceptFunction(t_acceptFunction func)
		{
			acceptFunction = func;
		}
		;
		vector<shared_ptr<ConsensusVariable>> proposals;

		string valueAsString();
		string toString();

		bool getValue(double* out);
		void setValue(double in);
		bool getValue(int* out);
		void setValue(int in);
		bool getValue(string& out);
		void setValue(string* in);

		bool getValue(vector<double>& out);
		void setValue(vector<double>* in);
		bool getValue(vector<int>& out);
		void setValue(vector<int>* in);
		bool getValue(vector<string>& out);
		void setValue(vector<string>* in);

		template<class T>
		inline void setValue(const T& obj)
		{
			val.clear();
			serialize(obj, val);
			type = CaceType::Custom;
		}

		template<class T>
		inline bool getValue(T& obj)
		{
			if (type == CaceType::Custom)
			{
				obj = deserialize<T>(val);
			}
			return type == CaceType::Custom;
		}

		bool defaultAcceptStrategy(Cace &c, vector<uint8_t>* commandedValue);
		bool lowestIDAcceptStrategy(Cace &c, vector<uint8_t>* commandedValue);
		bool mostRecentAcceptStrategy(Cace &c, vector<uint8_t>* commandedValue);
		bool electionAcceptStrategy(Cace &c, vector<uint8_t>* commandedValue);
		bool listAcceptStrategy(Cace &c, vector<uint8_t>* commandedValue);

		vector<delegate<void (ConsensusVariable*)>> changeNotify;
		void notify();

	protected:

		vector<uint8_t> val;
		string name;
		short type;
		int robotID;
		unsigned long arrivalTime;
		unsigned long decissionTime;
		unsigned long validityTime;
		unsigned long lamportAge;

		acceptStrategy strategy;

	};

} /* namespace cace */

#endif /* CONSENSUSVARIABLE_H_ */
