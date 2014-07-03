

#include <Rcpp.h>
using namespace Rcpp;

// [[Rcpp::depends(RcppThreads)]]
#include <RcppThreads.h>


// [[Rcpp::export]]
unsigned hardwareConcurrency() {
  return tthread::thread::hardware_concurrency();
}
