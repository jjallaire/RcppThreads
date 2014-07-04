
#ifndef __RCPP_THREADS__
#define __RCPP_THREADS__

// tinythread library 
#include "tthread/tinythread.h"
#include "tthread/tinythread.inl"

namespace RcppThreads {

// Class which represents a range of indexes to perform work on
// (worker functions are passed this range so they know which
// elements are safe to read/write to)
class IndexRange {
public:

  // Initizlize with a begin and (exclusive) end index
  IndexRange(std::size_t begin, std::size_t end)
    : begin_(begin), end_(end)
  {
  }
  
  // Access begin() and end()
  std::size_t begin() const { return begin_; }
  std::size_t end() const { return end_; }
  
private:
  std::size_t begin_;
  std::size_t end_;
};


// Body of code to execute within a worker thread
struct Body {
  virtual ~Body() {} 
  virtual void operator()(const IndexRange& range) = 0;
  
  virtual Body* split(const Body& body) const { return NULL; }
  virtual void join(const Body& body) {}
};


namespace {

// Because tinythread allows us to pass only a plain C function
// we need to pass our body and range within a struct that we 
// can cast to/from void*
struct Work {
  Work(IndexRange range, Body& body) 
    :  range(range), body(body)
  {
  }
  IndexRange range;
  Body& body;
};

// Thread which performs work (then deletes the work object
// when it's done)
extern "C" void workerThread(void* data) {
  try
  {
    Work* pWork = static_cast<Work*>(data);
    pWork->body(pWork->range);
    delete pWork;
  }
  catch(...)
  {
  }
}

// Function to calculate the ranges for a given input
std::vector<IndexRange> splitInputRange(const IndexRange& range) {
  
  // max threads is based on hardware concurrency
  std::size_t threads = tthread::thread::hardware_concurrency();
  
  // determine the chunk size
  std::size_t length = range.end() - range.begin();
  std::size_t chunkSize = length / threads;
  
  // allocate ranges
  std::vector<IndexRange> ranges;
  std::size_t nextIndex = range.begin();
  for (std::size_t i = 0; i<threads; i++) {
    std::size_t begin = nextIndex;
    std::size_t end = std::min(begin + chunkSize, range.end());
    ranges.push_back(IndexRange(begin, end));
    nextIndex = end;
  }

  // return ranges  
  return ranges;
}

  
} // anonymous namespace


// Execute the Body over the IndexRange in parallel
void parallelFor(IndexRange range, Body& body) {
  
  using namespace tthread;
  
  // split the work
  std::vector<IndexRange> ranges = splitInputRange(range);
  
  // create threads
  std::vector<thread*> threads;
  for (std::size_t i = 0; i<ranges.size(); ++i) {
    threads.push_back(new thread(workerThread, new Work(ranges[i], body)));   
  }
  
  // join and delete them
  for (std::size_t i = 0; i<threads.size(); ++i) {
    threads[i]->join();
    delete threads[i];
  }
}

// Execute the Body over the IndexRange in parallel then join results
void parallelReduce(IndexRange range, Body& body) {
  
  using namespace tthread;
  
  // split the work
  std::vector<IndexRange> ranges = splitInputRange(range);
  
  // create threads (split for each thread and track the allocated bodies)
  std::vector<thread*> threads;
  std::vector<Body*> bodies;
  for (std::size_t i = 0; i<ranges.size(); ++i) {
    Body* pBody = body.split(body);
    bodies.push_back(pBody);
    threads.push_back(new thread(workerThread, new Work(ranges[i], *pBody)));   
  }
  
  // wait for each thread, join it's results, then delete the body & thread
  for (std::size_t i = 0; i<threads.size(); ++i) {
    
    // wait for thread
    threads[i]->join();
    
    // join the results
    body.join(*bodies[i]);
    
    // delete the body (which we split above) and the thread
    delete bodies[i];
    delete threads[i];
  }
}



} // namespace RcppThreads

#endif // __RCPP_THREADS__
