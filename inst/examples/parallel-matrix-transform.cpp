/**
 * @title Transforming a Matrix in Parallel using RcppThreads
 * @author JJ Allaire
 * @license GPL (>= 2)
 * @tags matrix parallel featured
 * @summary Demonstrates transforming a matrix in parallel using 
 *   the RcppThreads package.
 */

/**
 * First a serial version of the matrix transformation. We take the square root 
 * of each item of a matrix and return a new matrix with the tranformed values. 
 * We do this by using `std::transform` to call the `sqrt` function on each 
 * element of the matrix:
 */

#include <Rcpp.h>
using namespace Rcpp;

#include <cmath>

// [[Rcpp::export]]
NumericMatrix matrixSqrt(NumericMatrix orig) {

  // allocate the matrix we will return
  NumericMatrix mat(orig.nrow(), orig.ncol());

  // transform it
  std::transform(orig.begin(), orig.end(), mat.begin(), ::sqrt);

  // return the new matrix
  return mat;
}

/**
 * Now we adapt our code to run in parallel using RcppThreads. We'll use the 
 * `parallelFor` function to do this. RcppThreads takes care of dividing up work
 * between threads, our job is to implement a "Body" functor that performs the
 * work over a given range of our input.
 * 
 * The `SqrtBody` functor below includes pointers to the input matrix as well as
 * the output matrix. Within it's `operator()` method it performs a
 * `std::transform` with the `sqrt` function on the array elements specified by
 * the `range` argument:
 */

// [[Rcpp::depends(RcppThreads)]]
#include <RcppThreads.h>
using namespace RcppThreads;

struct SqrtBody : public Body
{
   // source matrix
   double * const input;
   
   // destination matrix
   double* output;
   
   // initialize with source and destination
   SqrtBody(double * const input, double* output) 
      : input(input), output(output) {}
   
   // take the square root of the range of elements requested
   void operator()(const IndexRange& range) {
      std::transform(input + range.begin(),
                     input + range.end(),
                     output + range.begin(),
                     ::sqrt);
   }
};

/**
 * Here's the parallel version of our matrix transformation function that makes
 * uses of the `SqrtBody` functor. The main difference is that rather than
 * calling `std::transform` directly, the `parallelFor` function is called with
 * the range to operate on (based on the length of the input matrix) and an
 * instance of `SqrtBody`:
 */

// [[Rcpp::export]]
NumericMatrix parallelMatrixSqrt(NumericMatrix x) {
  
  NumericMatrix output(x.nrow(), x.ncol());
  
  SqrtBody body(x.begin(), output.begin());
  
  parallelFor(IndexRange(0, x.length()), body);
  
  return output;
}

/**
 * A comparison of the performance of the two functions shows the parallel 
 * version performing about 2.5 times as fast on a machine with 4 cores:
 */

/*** R

# allocate a matrix
m <- matrix(as.numeric(c(1:1000000)), nrow = 1000, ncol = 1000)

# ensure that serial and parallel versions give the same result
stopifnot(identical(matrixSqrt(m), parallelMatrixSqrt(m)))

# compare performance of serial and parallel
library(rbenchmark)
res <- benchmark(matrixSqrt(m),
                 parallelMatrixSqrt(m),
                 order="relative")
res[,1:4]
*/

/**
 * If you interested in learning more about RcppThreads see 
 * [https://github.com/jjallaire/RcppThreads](https://github.com/jjallaire/RcppThreads).
 */ 



