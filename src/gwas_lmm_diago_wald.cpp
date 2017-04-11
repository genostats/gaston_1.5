#include <Rcpp.h>
#include "diago2.h"
#include "matrix4.h"
#include <ctime>
#include <cmath>

//[[Rcpp::export]]
List GWAS_lmm_wald(XPtr<matrix4> pA, NumericVector mu, NumericVector Y, NumericMatrix X, 
                  int p, NumericVector Sigma, NumericMatrix U, int beg, int end, double tol) {

  int n = Sigma.size();
  int r = X.ncol();
  if(Y.size() != n || X.nrow() != n || U.nrow() != n || U.ncol() != n) 
    stop("Dimensions mismatch");
    
  // conversion en float...
  MatrixXf y0(n, 1);
  for(int i = 0; i < n; i++)
    y0(i,0) = Y[i];

  MatrixXf x0(n, r);
  for(int j = 0; j < r; j++) 
    for(int i = 0; i < n; i++)
      x0(i,j) = X(i,j);

  VectorXf sigma(n);
  for(int i = 0; i < n; i++) 
    sigma[i] = Sigma[i];

  MatrixXf u(n,n);
  for(int j = 0; j < n; j++) 
    for(int i = 0; i < n; i++)
      u(i,j) = U(i,j);

  MatrixXf x = u.transpose() * x0;
  MatrixXf y = u.transpose() * y0;

  // Zecteur SNPs
  VectorXf SNP(n);

  // declare vectors containing result
  NumericVector H2(end-beg+1);
  NumericVector BETA(end-beg+1);
  NumericVector SDBETA(end-beg+1);

  // object for likelihood maximization
  diag_likelihood<MatrixXf, VectorXf, float> A(p, y, x, sigma);

  clock_t chaviro(0);
  clock_t begin_t = clock();

  float h2 = 0;

  // Rcout << min_h2 << " < h2 < " << max_h2 << "\n";
  for(int i = beg; i <= end; i++) {
    if( std::isnan(mu(i)) || mu(i) == 0 || mu(i) == 2 ) {
      H2(i-beg) = NAN;
      BETA(i-beg) = NAN;
      SDBETA(i-beg) = NAN;
      continue;
    }

    // remplir dernière colonne de x : récupérer SNP, multiplier par u'...
    for(int ii = 0; ii < pA->true_ncol-1; ii++) {
      uint8_t x = pA->data[i][ii];
      for(int ss = 0; ss < 4; ss++) {
        SNP(4*ii+ss) = ((x&3) != 3)?(x&3):mu(i);
        x >>= 2;
      }
    }
    { int ii = pA->true_ncol-1;
      uint8_t x = pA->data[i][ii];
      for(int ss = 0; ss < 4 && 4*ii+ss < pA->ncol; ss++) {
        SNP(4*ii+ss) = ((x&3) != 3)?(x&3):mu(i);
        x >>= 2;
      }
    }
    A.X.col(r-1) = u.transpose() * SNP;

    // likelihood maximization
    begin_t = clock();
    A.newton_max(h2, 0, 1, tol, false);
    
    chaviro += clock() - begin_t;

    // CALCUL DES BLUPS 
    VectorXf beta, omega;
    A.blup(h2, beta, omega, false, true);
/* 
if(i < 5) Rcout << "beta = " << beta.transpose() << "\n" ;


    VectorXf sigmab = A.Sigma.bottomRows(n-p);
    VectorXf omega = h2 * sigmab.asDiagonal() * A.P0y;

      // Xb' Xb
      MatrixXf xtx( MatrixXf(r,r).setZero().selfadjointView<Lower>().rankUpdate( A.X.bottomRows(n-p).transpose() ));
      MatrixXf xtx0( xtx );
      MatrixXf xtxi(r,r); // et son inverse
      float d, ld;
      sym_inverse(xtx0, xtxi, d, ld, 1e-5); // détruit xtx0

      VectorXf z = A.Y;
      z.tail(n-p) -= omega + (1-h2)*A.P0y;
      VectorXf beta = xtxi * A.X.bottomRows(n-p).transpose() * z.bottomRows(n-p);
if(i < 5)  Rcout << "beta = " << beta.transpose() << "\n" ; */

    H2(i-beg) = h2;
    BETA(i-beg) = beta(r-1);
    SDBETA(i-beg) = sqrt(A.v*A.XViX_i(r-1,r-1));
  }

  //cout << (float) chaviro / CLOCKS_PER_SEC << " spent in likelihood maximization\n";
  List R;
  R["h2"] = H2;
  R["beta"] = BETA;
  R["sd"] = SDBETA;
  return R;
}


RcppExport SEXP gg_GWAS_lmm_wald(SEXP pASEXP, SEXP muSEXP, SEXP YSEXP, SEXP XSEXP, SEXP pSEXP, SEXP SigmaSEXP, SEXP USEXP, SEXP begSEXP, SEXP endSEXP, SEXP tolSEXP) {
BEGIN_RCPP
    SEXP __sexp_result;
    {
        Rcpp::RNGScope __rngScope;
        Rcpp::traits::input_parameter< XPtr<matrix4> >::type pA(pASEXP );
        Rcpp::traits::input_parameter< NumericVector >::type mu(muSEXP );
        Rcpp::traits::input_parameter< NumericVector >::type Y(YSEXP );
        Rcpp::traits::input_parameter< NumericMatrix >::type X(XSEXP );
        Rcpp::traits::input_parameter< int >::type p(pSEXP );
        Rcpp::traits::input_parameter< NumericVector >::type Sigma(SigmaSEXP );
        Rcpp::traits::input_parameter< NumericMatrix >::type U(USEXP );
        Rcpp::traits::input_parameter< int >::type beg(begSEXP );
        Rcpp::traits::input_parameter< int >::type end(endSEXP );
        Rcpp::traits::input_parameter< double >::type tol(tolSEXP );
        List __result = GWAS_lmm_wald(pA, mu, Y, X, p, Sigma, U, beg, end, tol);
        PROTECT(__sexp_result = Rcpp::wrap(__result));
    }
    UNPROTECT(1);
    return __sexp_result;
END_RCPP
}


