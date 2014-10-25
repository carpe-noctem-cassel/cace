/*
 * JobStateEntity.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef JOBSTATEENTITY_H_
#define JOBSTATEENTITY_H_

#include "../timeManager/AgentTimeData.h"

namespace cace
{

	class JobStateEntity
	{
	public:
		JobStateEntity(int robotID, int retrys, ctime lastSent);
		virtual ~JobStateEntity();

		int robotID;
		ctime lastSent;
		int retrys;

	};

} /* namespace cace */

#endif /* JOBSTATEENTITY_H_ */
