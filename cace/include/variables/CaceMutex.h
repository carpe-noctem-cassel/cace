/*
 * CaceMutex.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef CACEMUTEX_H_
#define CACEMUTEX_H_

#include <variables/ConsensusVariable.h>
#include <string>

namespace cace
{

	class CaceMutex : public ConsensusVariable
	{
	public:
		CaceMutex(Cace* cace, string name, unsigned long validityTime, unsigned long decissionTime, unsigned long lamportAge, bool strict);
		CaceMutex(const ConsensusVariable& v, Cace* cace, bool strict);

		void p();
		void p(vector<int> robots);
		bool pNoBlock();
		bool pNoBlock(vector<int> robots);
		bool isFree(vector<int> robots);
		bool isOwner(int robotID, vector<int> robots);
		bool v();

		bool strict;
		Cace* cace;
		static string mutexNamespace;
	};

} /* namespace cace */

#endif /* CACEMUTEX_H_ */
