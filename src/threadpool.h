#pragma once

#include <thread>
#include <vector>

class threadpool {
		threadpool();
		~threadpool();
		void InitThreads(int numberOfThreads);
		void EnqueTask();

		std::vector<std::thread> threads;
};
