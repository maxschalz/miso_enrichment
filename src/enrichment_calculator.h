#ifndef MISOENRICHMENT_SRC_ENRICHMENT_CALCULATOR_H_
#define MISOENRICHMENT_SRC_ENRICHMENT_CALCULATOR_H_

#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "include/cppoptlib/function.h"  // from external/CppNumericalSolvers
#include "composition.h"

class EnrichmentProblem;

namespace misoenrichment {

class EnrichmentCalculator {
 public:
  friend class EnrichmentProblem;
  FRIEND_TEST(EnrichmentCalculatorTest, AssignmentOperator);

  EnrichmentCalculator();
  EnrichmentCalculator(cyclus::CompMap feed_comp,
                       double target_product_assay,
                       double target_tails_assay, double gamma,
                       std::string enrichment_process,
                       double feed_qty, double product_qty,
                       double max_swu, bool use_downblending=true,
                       bool use_integer_stages=true,
                       double n_init_enriching=-1, double n_init_stripping=-1);
  EnrichmentCalculator(const EnrichmentCalculator& e);
  EnrichmentCalculator& operator= (const EnrichmentCalculator& e);

  void PPrint();

  void BuildMatchedAbundanceRatioCascade();

  void SetInput(cyclus::CompMap new_feed_composition,
      double new_target_product_assay, double new_target_tails_assay,
      double new_feed_qty, double new_product_qty, double new_max_swu,
      double gamma_235, std::string enrichment_process, bool use_downblending);

  void EnrichmentOutput(cyclus::CompMap& product_compmap,
                        cyclus::CompMap& tails_compmap,
                        double& feed_used, double& swu_used,
                        double& product_produced, double& tails_produced,
                        double& n_enrich, double& n_strip);
  void ProductOutput(cyclus::CompMap&, double&);

  inline double FeedUsed() { return feed_qty; }
  inline double SwuUsed() { return swu; }

 private:
  bool use_downblending;  // Use only in conjunction with `use_integer_stages`.
  bool use_integer_stages;  // Else use floating-point number of stages


  // All assays, compositions, cyclus::CompMap are assumed to be atom fraction
  cyclus::CompMap feed_composition;
  cyclus::CompMap product_composition;
  cyclus::CompMap tails_composition;

  double target_product_assay;
  double target_tails_assay;
  // Units of all of the streams are kg timestep^-1
  double target_feed_qty;
  double target_product_qty;
  double feed_qty;
  double product_qty;
  double tails_qty;
  double max_swu;  // in kg SWU timestep^-1
  double swu = 0;  // Separative work that has been performed
                   // in kg SWU timestep^-1

  const std::vector<int> isotopes;
  std::string enrichment_process;
  std::map<int,double> separation_factors;
  std::map<int,double> alpha_star;

  // Number of stages in the enriching and in the stripping section
  double n_enriching;
  double n_stripping;
  double n_init_enriching;
  double n_init_stripping;

  double gamma_235;  // The overall separation factor for U-235

  void CalculateGammaAlphaStar_();
  void CalculateIntegerStages_();
  void CalculateDecimalStages_();
  void CalculateFlows_();
  void CalculateSwu_();
  void CalculateConcentrations_();
  void Downblend_();
  void CalculateSums(double& sum_e, double& sum_s);

  double ValueFunction_(const cyclus::CompMap& composition);
};


class EnrichmentProblem
  : public cppoptlib::function::FunctionCRTP<
        EnrichmentProblem,
        double,
        cppoptlib::function::DifferentiabilityMode::None,
        2> {
 public:
  //EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  EnrichmentProblem(EnrichmentCalculator* c) : calculator(c) {}

  // Function to be minimised (must *not* be renamed).
  // Calculates the relative difference between actual and desired uranium
  // assay for an enrichment process with a given staging; for both product and
  // tails output.
  //
  // I do not manage to imnplement the function in the source file, if I do I
  // keep getting the following error: 'ScalarType' in namespace 'cppoptlib'
  // does not name a type.
  ScalarType operator()(const VectorType &staging) const {
    calculator->n_enriching = staging(0);
    calculator->n_stripping = staging(1);
    calculator->CalculateConcentrations_();

    double target_p = calculator->target_product_assay;
    double target_t = calculator->target_tails_assay;

    return pow((calculator->product_composition[922350000] - target_p) / target_p, 2)
           + pow((calculator->tails_composition[922350000] - target_t) / target_t, 2);
  }

 private:
  EnrichmentCalculator* calculator;
};

}  // namespace misoenrichment

#endif  // MISOENRICHMENT_SRC_ENRICHMENT_CALCULATOR_H_
