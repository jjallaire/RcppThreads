
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
  virtual void operator()(const IndexRange& range) const = 0;
};


namespace {

// Because tinythread allows us to pass only a plain C function
// we need to pass our body and range within a struct that we 
// can cast to/from void*
struct Work {
  Work(IndexRange range, const Body& body) 
    :  range(range), body(body)
  {
  }
  IndexRange range;
  const Body& body;
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
  
} // anonymous namespace


// Execute the Body over the IndexRange in parallel
void parallelFor(IndexRange range, const Body& body) {
  
  // just split into two threads until we write the divider
  
  IndexRange range1(range.begin(), (range.end() - range.begin()) / 2);
  tthread::thread t1(workerThread, static_cast<void*>(new Work(range1, body)));
  
  IndexRange range2(range1.end(), range.end());
  tthread::thread t2(workerThread, static_cast<void*>(new Work(range2, body)));
  
  // wait for all threads to complete
  t1.join();
  t2.join();
}



} // namespace RcppThreads

#endif // __RCPP_THREADS__
