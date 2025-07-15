#include "python_enrichment_tests.h"

#include <filesystem>  // std::filesystem::exists, std::filesystem::is_empty
#include <fstream>
#include <nlohmann/json.hpp>

namespace misoenrichment {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
nlohmann::json PythonEnrichmentTest::GetResults() {
  return p.EnrichmentResults_();
}

nlohmann::json PythonEnrichmentTest::DoRunEnrichment(
  std::map<int, double> feed_cm,
  double product_assay,
  double tails_assay,
  double max_swu,
  double gamma_235,
  std::string enrichment_process,
  double feed_qty,
  double product_qty
) {
  return p.RunEnrichment(feed_cm, product_assay, tails_assay, max_swu,
                         gamma_235, enrichment_process, feed_qty, product_qty);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(PythonEnrichmentTest, Constructor) {
  std::string s("test");

  EXPECT_NO_THROW(PythonEnrichment(s));
  EXPECT_NO_THROW(PythonEnrichment());
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(PythonEnrichmentTest, StoreEnrichmentParams) {
  std::string s("test");

  p = PythonEnrichment(s);
  std::string fname = GetFname();

  std::map<int, double> fm;
  fm[922350000] = 0.1;
  fm[922380000] = 0.9;
  double dummy_var = 0.;
  bool delete_outfile = false;

  ParamsToOutFile(fm, dummy_var, dummy_var, dummy_var, dummy_var, "",
                  dummy_var, dummy_var, delete_outfile);


  EXPECT_TRUE(std::filesystem::exists(fname));
  EXPECT_FALSE(std::filesystem::is_empty(fname));
  EXPECT_TRUE(std::filesystem::remove(fname));

  delete_outfile = true;
  ParamsToOutFile(fm, dummy_var, dummy_var, dummy_var, dummy_var, "",
                  dummy_var, dummy_var, delete_outfile);
  EXPECT_FALSE(std::filesystem::exists(fname));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(PythonEnrichmentTest, GetEnrichmentResults) {
  p = PythonEnrichment();

  nlohmann::json json_object;
  json_object["int"] = 0;
  json_object["map"]["test"] = 0.;
  std::ofstream file(GetFname(), std::ofstream::out | std::ofstream::trunc);
  file << json_object << "\n";
  file.close();

  nlohmann::json return_obj;
  EXPECT_NO_THROW({ return_obj = GetResults(); });
  EXPECT_EQ(json_object, return_obj);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(PythonEnrichmentTest, TestCompleteRun) {
  p = PythonEnrichment();

  std::map<int, double> feed_cm;
  feed_cm[922340000] = 0.001;
  feed_cm[922350000] = 0.1;
  feed_cm[922380000] = 0.899;
  double product_assay(0.9);
  double tails_assay(0.03);
  double max_swu(1000);
  double gamma_235(1.4);
  std::string enrichment_process("centrifuge");
  double feed_qty(1000);
  double product_qty(1000);
  bool delete_outfile = false;

  nlohmann::json return_obj;
  EXPECT_NO_THROW({
      return_obj = DoRunEnrichment(
        feed_cm, product_assay, tails_assay, max_swu, gamma_235,
        enrichment_process, feed_qty, product_qty);
  });
}

}  // namespace misoenrichment

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// required to get functionality in cyclus agent unit tests library
#ifndef CYCLUS_AGENT_TESTS_CONNECTED
int ConnectAgentTests();
static int cyclus_agent_tests_connected = ConnectAgentTests();
#define CYCLUS_AGENT_TESTS_CONNECTED cyclus_agent_tests_connected
#endif  // CYCLUS_AGENT_TESTS_CONNECTED
