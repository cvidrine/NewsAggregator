/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
using namespace std;

condition_variable_any cv;
mutex m;
int thunksCompleted;
int thunksScheduled; 
mutex numLock;
bool finished;

void ThreadPool::startWorker(size_t workerID){
    availableLock.lock();
    available[workerID] = false;
    availableLock.unlock();
    functionsLock.lock();
    currFunctions[workerID] = scheduleQueue.front();
    scheduleQueue.pop();
    functionsLock.unlock();
    workerSemaphores[workerID]->signal();
}


void ThreadPool::dispatcher(){
    while(true){
        scheduleSem.wait();
        scheduleLock.lock();
        if(finished) return;
        availableWorker.wait();
        for(size_t workerID = 0; workerID < kNumThreads; workerID++){
            availableLock.lock();
            if(!available[workerID]){
                availableLock.unlock();
                continue;
            }
            availableLock.unlock();
            startWorker(workerID);
            break;
        }
        scheduleLock.unlock();
    }
}

void ThreadPool::worker(size_t workerID){
    while(true){
        workerSemaphores[workerID]->wait();
        if(finished) return;
        functionsLock.lock();
        function<void(void)> foo = currFunctions[workerID];
        functionsLock.unlock();
        foo();
        availableLock.lock();
        available[workerID] = true;
        availableLock.unlock();
        numLock.lock();
        thunksCompleted++;
        numLock.unlock();
        availableWorker.signal();
        if(thunksScheduled - thunksCompleted == 0) cv.notify_all();
    }
}

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), available(numThreads), workerSemaphores(numThreads), currFunctions(numThreads), availableWorker(0) {
    currFunctions = {NULL};
    kNumThreads = numThreads;
    thunksCompleted = 0;
    thunksScheduled = 0;
    finished = false;
    dt = thread([this](){
            dispatcher();
            }); 
    for(size_t workerID = 0; workerID < kNumThreads; workerID++){
         wts[workerID] = thread([this](size_t workerID){
                available[workerID] = true;
                workerSemaphores[workerID].reset(new semaphore(0));
                availableWorker.signal();
                worker(workerID);
                }, workerID);
   }
}
void ThreadPool::schedule(const function<void(void)>& thunk) {
    scheduleLock.lock();
    scheduleQueue.push(thunk);
    scheduleLock.unlock();
    scheduleSem.signal();
    lock_guard<mutex> lg(m);
    thunksScheduled++;
}

void ThreadPool::wait() {
    lock_guard<mutex> lg(m);
    cv.wait(m, []{return (thunksScheduled-thunksCompleted == 0);});
}

ThreadPool::~ThreadPool() {
    wait();
    finished = true;
    
    //for(size_t workerID=0; workerID < kNumThreads; workerID++) workerSemaphores[workerID]->signal();
    for(auto& workerSem: workerSemaphores){
        workerSem->signal();
    }
    scheduleSem.signal();
    for(auto& worker: wts)
        worker.join();
    dt.join();
}
