#ifndef MISOENRICHMENT_SRC_PYTHON_ENRICHMENT_TESTS_H_
#define MISOENRICHMENT_SRC_PYTHON_ENRICHMENT_TESTS_H_

#include <gtest/gtest.h>

#include "python_enrichment.h"

#include <map>
#include <string>

#include <nlohmann/json_fwd.hpp>

namespace misoenrichment {

class PythonEnrichmentTest : public ::testing::Test {
 protected:
  PythonEnrichment p;

  nlohmann::json GetResults();
  inline std::string GetFname() { return p.fname; }
  inline void ParamsToOutFile(
    std::map<int, double> feed_cm,
    double product_assay_,
    double tails_assay_,
    double max_swu_,
    double gamma_235_,
    std::string enrichment_process_,
    double feed_qty_,
    double product_qty_,
    bool delete_outfile
  ) {
    p.ParamsToOutFile_(
      feed_cm, product_assay_, tails_assay_, max_swu_, gamma_235_,
      enrichment_process_, feed_qty_, product_qty_, delete_outfile
    );
  }
  nlohmann::json DoRunEnrichment(
    std::map<int, double> feed_cm,
    double product_assay,
    double tails_assay,
    double max_swu,
    double gamma_235,
    std::string enrichment_process,
    double feed_qty,
    double product_qty
  );

  /*
  EnrichmentCalculatorTest();

  EnrichmentCalculator e;

  // Values to be calculated by EnrichmentCalculator
  cyclus::CompMap product_comp, tails_comp;
  double feed_qty, product_qty, tails_qty, swu_used;
  double n_enriching, n_stripping;

  const cyclus::CompMap expect_product_comp;
  const cyclus::CompMap expect_tails_comp;

  const double expect_n_enriching;
  const double expect_n_stripping;

  const double expect_feed_qty;
  const double expect_product_qty;
  const double expect_tails_qty;
  const double expect_swu_used;
  */

};

}  // namespace misoenrichment

#endif  // MISOENRICHMENT_SRC_PYTHON_ENRICHMENT_TESTS_H_
