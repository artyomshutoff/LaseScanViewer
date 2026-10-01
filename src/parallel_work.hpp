#pragma once
#include <algorithm>
#include <future>
#include <atomic>
#include <thread>
#include <vector>
// Independent jobs only. Each job owns its output; callers combine results in
// their original order so parallel execution does not alter numerical sums.
template<class Function> inline void parallelJobs(size_t count,Function function){
    unsigned workers=unsigned(std::min<size_t>(count,std::min(4u,std::max(1u,std::thread::hardware_concurrency()))));
    if(workers<=1){for(size_t i=0;i<count;i++)function(i);return;}
    std::atomic<size_t> next{0};std::vector<std::future<void>> jobs;
    for(unsigned worker=0;worker<workers;worker++)jobs.push_back(std::async(std::launch::async,[&]{for(;;){size_t i=next.fetch_add(1);if(i>=count)return;function(i);}}));
    for(auto& job:jobs)job.get();
}
