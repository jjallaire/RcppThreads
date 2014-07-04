#include <Rcpp.h>
using namespace Rcpp;

// [[Rcpp::depends(RcppThreads)]]
#include <RcppThreads.h>
using namespace RcppThreads;

#include <cmath>



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
   void operator()(const IndexRange& range) const {
      std::transform(input + range.begin(),
                     input + range.end(),
                     output + range.begin(),
                     ::sqrt);
   }
};


// [[Rcpp::export]]
NumericVector squareRoot(NumericVector x) {
  
  NumericVector output(x.length());
  
  SqrtBody body(x.begin(), output.begin());
  
  parallelFor(x, body);
    
  return output;
}


/*** R

vec <- as.numeric(c(1:1000))

squareRoot(vec)
*/



