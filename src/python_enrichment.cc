#define PY_SSIZE_T_CLEAN
#include <Python.h>  // PyRun_SimpleString

#include "python_enrichment.h"

#include <cstdio>  // std::remove to remove a file
#include <fstream>  // std::ifstream, std::ofstream
#include <sstream>

#include "cyclus.h"
#include "miso_helper.h"  // IsotopesNucID

#include <nlohmann/json.hpp>

namespace misoenrichment {

PythonEnrichment::PythonEnrichment() : PythonEnrichment("") {}

PythonEnrichment::PythonEnrichment(std::string uid) : uid(uid) {
  std::stringstream ss;
  ss << "enrichment_params_and_results";
  if (!uid.empty()) {
    ss << "_" << uid;
  }
  ss << ".json";
  fname = ss.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
nlohmann::json PythonEnrichment::EnrichmentResults_() const {
  nlohmann::json json_object;

  // Dump file contents into json object.
  std::ifstream file(fname, std::ifstream::in);
  if (!file.is_open()) {
    std::stringstream msg;
    msg << "Cannot find file '" << fname << "'";
    throw cyclus::IOError(msg.str());
  }
  file >> json_object;
  file.close();

  return json_object;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
nlohmann::json PythonEnrichment::RunEnrichment(
  std::map<int, double> feed_cm,
  double product_assay_,
  double tails_assay_,
  double max_swu_,
  double gamma_235_,
  std::string enrichment_process_,
  double feed_qty_,
  double product_qty_
) const {
  ParamsToOutFile_(
      feed_cm, product_assay_, tails_assay_, max_swu_, gamma_235_,
      enrichment_process_, feed_qty_, product_qty_, false);

  cyclus::PyStart();
  int python_exit_code = 0;
  std::stringstream ss;
  python_exit_code += PyRun_SimpleString("from misoenrichment import calculator");
  ss << "calculator.calculate_enrichment_from_file('" << fname << "', "
     << "suppress_warnings=True)";
  python_exit_code += PyRun_SimpleString(ss.str().c_str());
  if (python_exit_code != 0) {
    throw cyclus::Error("Execution of Python enrichment calculations unsuccessful.");
  }

  nlohmann::json enrichment_results = EnrichmentResults_();

  // Delete the .json files to prevent cluttering up the working directory.
  if (std::remove(fname.c_str()) != 0) {
    std::stringstream msg;
    msg << "Error deleting file '" << fname << "'.";
    throw cyclus::IOError(msg.str());
  }
  return enrichment_results;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PythonEnrichment::ParamsToOutFile_(
  std::map<int, double> feed_cm,
  double product_assay_,
  double tails_assay_,
  double max_swu_,
  double gamma_235_,
  std::string enrichment_process_,
  double feed_qty_,
  double product_qty_,
  bool delete_outfile) const {

  nlohmann::json json_object;

  cyclus::compmath::Normalize(&feed_cm);

  // Loop over permitted isotopes in composition, add them to the json
  // output file.
  for (int nuclide : IsotopesNucID()) {
    double fraction = feed_cm[nuclide];
    if (!cyclus::AlmostEq(fraction, 0.)) {
      json_object["feed_composition"][std::to_string(nuclide)] = fraction;
    }
  }
  json_object["product_assay"] = product_assay_;
  json_object["tails_assay"] = tails_assay_;
  json_object["feed_qty"] = feed_qty_;
  json_object["product_qty"] = product_qty_;
  json_object["max_swu"] = max_swu_;
  json_object["process"] = enrichment_process_;
  json_object["alpha_235"] = gamma_235_;

  std::ofstream file(fname, std::ofstream::out | std::ofstream::trunc);
  file << json_object << "\n";
  file.close();

  // Deleting the output file does not make sense except for unit tests to
  // prevent cluttering up the working directory where the unit tests are
  // performed.
  if (delete_outfile && (std::remove(fname.c_str()) != 0)) {
    std::stringstream msg;
    msg << "Error deleting file '" << fname << "'.\n";
    throw cyclus::IOError(msg.str());
  }
}

}  // namespace misoenrichment
