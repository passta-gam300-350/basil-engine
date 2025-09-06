#ifndef LIB_ECS_SCHEDULER
#define LIB_ECS_SCHEDULER

#include <coroutine>
#include <thread>

namespace ecs {

	//unit of work
	struct task {

	};

	//pipeline for work, task are pushed into the pipeline, owns the worker thread
	struct pipeline {
		struct task_list {
			
		};
	};

	//scheduler owns pipeline, pipe work from busy pipelines to idle
	struct scheduler {

	};
}


#endif