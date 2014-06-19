/*
 * CaceMutex.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include <cace.h>
#include <caceSpace.h>
#include <communication/CaceCommunication.h>
#include <CaceTypes.h>
#include <variables/CaceMutex.h>
#include <chrono>
#include <thread>
#include <vector>

using namespace std;

namespace cace
{
	string CaceMutex::mutexNamespace = "m/";

	CaceMutex::CaceMutex(Cace* cace, string name, unsigned long validityTime, unsigned long decissionTime,
							unsigned long lamportAge, bool strict) :
			ConsensusVariable(mutexNamespace + name, acceptStrategy::ThreeWayHandShake, validityTime,
								cace->communication->getOwnID(), decissionTime, lamportAge, type)
	{
		setValue((int)0);
		this->strict = strict;
		this->cace = cace;
	}

	CaceMutex::CaceMutex(const ConsensusVariable& v, Cace* cace, bool strict) :
			ConsensusVariable(v)
	{
		this->strict = strict;
		this->cace = cace;
	}

	void CaceMutex::p()
	{
		p(cace->activeRobots);
	}

	void CaceMutex::p(vector<int> robots)
	{
		while (pNoBlock(robots))
		{
			this_thread::sleep_for(chrono::milliseconds(1));
		}
	}

	bool CaceMutex::pNoBlock()
	{
		return pNoBlock(cace->activeRobots);
	}

	bool CaceMutex::pNoBlock(vector<int> robots)
	{
		int bel = 0;
		getValue(&bel);

		//Check whether semaphore is "free"
		if (!isFree(robots))
			return false;

		//check whether "we" already "own" the cs
		if (isOwner(cace->communication->getOwnID(), robots))
			return true;

		//if semaphore is free, but not owned by us
		if (this->isAcknowledged(*cace))
			cace->caceSpace->distributeValue(name, cace->communication->getOwnID(), strategy);
		return false;
	}

	bool CaceMutex::isFree(vector<int> robots)
	{
		int bel = 0;
		getValue(&bel);
		if (hasValue && bel > 0)
		{
			return false;
		}
		if (!strict)
		{
			for (auto v : proposals)
			{
				if (!v->hasValue)
				{
					continue;
				}
				int b;
				v->getValue(&b);
				bool contains = false;
				for (int r : robots)
				{
					if (r == v->getRobotID())
					{
						contains = true;
					}
				}
				if (b > 0 && contains)
				{
					return false;
				}
			}
		}
		else
		{
			for (auto v : proposals)
			{
				if (!v->hasValue)
				{
					continue;
				}
				int b;
				v->getValue(&b);
				if (b > 0)
				{
					return false;
				}
			}
		}
		return true;
	}

	bool CaceMutex::isOwner(int robotID, vector<int> robots)
	{
		int bel = 0;
		bool own = true;
		getValue(&bel);
		if (!hasValue || bel != robotID)
		{
			own = false;
		}
		int believer = 0;
		if (!strict)
		{
			//in non-strict semaphore we only care on robots in the list
			for (auto v : proposals)
			{
				if (!v->hasValue)
				{
					bool contains = false;
					for (int i : robots)
					{
						if (i == bel)
						{
							contains = true;
						}
					}
					if (contains)
						own = false;
					continue;
				}
				v->getValue(&bel);
				bool contains = false;
				for (int i : robots)
				{
					if (i == bel)
					{
						contains = true;
					}
				}
				if (bel != robotID && contains)
				{
					own = false;
				}
				else
					believer++;
			}
			if (believer < robots.size())
				own = false;
		}
		else
		{
			//In strict semaphores ALL robots (even inactive have to agree)
			for (auto v : proposals)
			{
				if (!v->hasValue)
				{
					own = false;
					continue;
				}
				v->getValue(&bel);
				if (bel != robotID)
				{
					own = false;
				}
				else
					believer++;
			}
			if (believer < robots.size())
				own = false;
		}
		return own;
	}

	bool CaceMutex::v()
	{
		int bel = 0;
		getValue(&bel);
		if (bel != cace->communication->getOwnID())
			return false;

		cace->caceSpace->distributeValue(name, -1, strategy);
		return true;
	}

} /* namespace cace */
