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
		/*!
		 * Copy Constructor (only copys references)
		 */
		ConsensusVariable(const ConsensusVariable& v);

		/*!
		 * Name: Name of the Variable
		 * consensusLevel: enum ConsensusLevel, automatically sets conflict resolution
		 * validityTime: validity time of the variable as absolute value
		 * robot id: the robot how believes this
		 * decissionTime: time the variable value has been decided. Mostly: TimeManager.DistributedTime
		 * lamportAge: TimeManager.LamportAge
		 * type: Variabletype (RosCS.ConsensusEngine.CaceType.*)
		 */
		ConsensusVariable(string name, acceptStrategy strategy, unsigned long validityTime, int robotID,
							unsigned long decissionTime, unsigned long lamportAge, short type);
		virtual ~ConsensusVariable();

		/*!
		 * "Accepts" values of 'var', except lamport age
		 */
		void update(ConsensusVariable& v);

		/*!
		 * Checks whether own value(in bytes) equals byte list
		 */
		bool valueEqual(vector<uint8_t>* cmp);

		/*!
		 * Checks whether own believe equals to a variable 'var'
		 */
		bool believeEqual(ConsensusVariable& v);

		/*!
		 * Returns whether the current Variable believe is known to all other agents
		 */
		bool isAcknowledged(Cace& c);

		/*!
		 * Returns whether the current Variable believe is known to all other agents and they agree to our believe
		 * (i.e. they have an equal believe)
		 */
		bool isAgreed(Cace& c);

		/*!
		 * Check for Conflict wrt. strategy
		 */
		bool checkConflict(Cace& c);

		/*!
		 * Returns the scope of the variable
		 */
		string getScope();

		vector<uint8_t> getValue();

		/*!
		 * Own Variable Value Believe
		 */
		void setValue(vector<uint8_t> value);

		/*
		 * Variable Name (includes full scope)
		 */
		string& getName();
		void setName(string name);

		/*!
		 * Variable Type (see msg/CaceType.msg)
		 */
		short getType();
		void setType(short t);

		/*!
		 * Indicates wheter the variable has a value
		 */
		bool hasValue;

		/*!
		 * Variable Name (includes full Local Address)
		 */
		int getRobotID();
		void setRobotID(int id);

		/*!
		 * The time the value has been set as local timestamp
		 */
		unsigned long getArrivalTime();
		void setArrivalTime(unsigned long at);

		/*!
		 * Decission Time
		 */
		unsigned long getDecissionTime();
		void setDecissionTime(unsigned long dt);

		/*!
		 * Validity Time as absolute time (use TimeManager.DistributedTime)
		 */
		unsigned long getValidityTime();
		void setValidityTime(unsigned long vt);

		/*!
		 * Lamport Time
		 */
		unsigned long getLamportAge();
		void setLamportAge(unsigned long la);

		/*!
		 * Consensus Level (low for weak, high for strong consensus)
		 */
		acceptStrategy getAcceptStrategy();
		void setAcceptStrategy(acceptStrategy aS);

		/*!
		 * Current Acceptance Functions
		 * Note: Muste be Part of this class
		 */
		t_acceptFunction acceptFunction;

		/*!
		 * Call for Value Acceptance according to strategy
		 */
		bool acceptProposals(Cace& cace, vector<uint8_t>* value);
		void setAcceptFunction(t_acceptFunction func)
		{
			acceptFunction = func;
		}

		/*!
		 * List of all Robots' proposals
		 */
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

		/*!
		 * Custom Type Converter
		 */
		template<class T>
		inline void setValue(const T& obj)
		{
			val.clear();
			serialize(obj, val);
			type = CaceType::Custom;
		}

		/*!
		 * Custom Type Converter
		 */
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

		/*!
		 * Register of functions that are called when an variable change occured
		 */
		vector<delegate<void(ConsensusVariable*)>> changeNotify;

		/*!
		 * Notifies all register functions for variable changes
		 */
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
