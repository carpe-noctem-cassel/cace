#ifndef CACETYPES_H_
#define CACETYPES_H_

namespace cace
{
	typedef unsigned long ctime;

	enum acceptStrategy
	{
		NoDistribution = 0, FireAndForgetSet = 8, FireAndForgetElection = 9,
		//Default has to be the highest ID!
		FireAndForget = 10, TwoWayHandShakeSet = 36, TwoWayHandShakeElection = 37, TwoWayHandShakeMostRecent = 38,
		TwoWayHandShakeLowestID = 39,
		//Default has to be the highest ID!
		TwoWayHandShake = 40, ThreeWayHandShakeSet = 66, ThreeWayHandShakeElection = 67,
		ThreeWayHandShakeMostRecent = 68, ThreeWayHandShakeLowestID = 69,
		//Default has to be the highest ID!
		ThreeWayHandShake = 70
	};
}

#endif
