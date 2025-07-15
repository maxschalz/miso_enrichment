#ifndef MISOENRICHMENT_SRC_PYTHON_ENRICHMENT_H_
#define MISOENRICHMENT_SRC_PYTHON_ENRICHMENT_H_

#include <map>
#include <string>

#include <nlohmann/json_fwd.hpp>

namespace misoenrichment {

// Data will be passed via a .json file. The structure and keywords used are
// as follows (file from MIsoEnrich to PythonEnrichment):
// json_file = {
//    "feed_composition": {
//      "922320000": double,
//      "922330000": double,
//      ...
//    },
//    "product_assay": double,
//    "tails_assay": double,
//    "feed_qty": double,
//    "product_qty": double,
//    "max_swu": double,
//    "process": string,  // either "diffusion" or "centrifuge"
//    "alpha_235": double
// }
//
//
// File from PythonEnrichment to MIsoEnrichment:
// json_file = {
//    "feed_composition": {
//      "922320000": double,
//      "922330000": double,
//      ...
//    },
//    "product_composition": {
//      "922320000": double,
//      ...
//    },
//    "tails_composition": {
//      "922320000": double,
//      ...
//    },
//    "feed_qty": double,
//    "product_qty": double,
//    "tails_qty": double,
//    "swu": double,
//    "process": string,  // either "diffusion" or "centrifuge"
//    "alpha_235": double,
//    "n_enriching": double,
//    "n_stripping": double
// }

class PythonEnrichment {
 public:
  friend class PythonEnrichmentTest;

  PythonEnrichment();
  PythonEnrichment(std::string uid);

  nlohmann::json RunEnrichment(
      std::map<int, double> feed_cm,
      double product_assay,
      double tails_assay,
      double max_swu,
      double gamma_235,
      std::string enrichment_process,
      double feed_qty,
      double product_qty
  ) const;

  inline std::string GetUid() { return uid; }

 private:
  nlohmann::json EnrichmentResults_() const;

  void ParamsToOutFile_(
      std::map<int, double> feed_cm,
      double product_assay_,
      double tails_assay_,
      double max_swu_,
      double gamma_235_,
      std::string enrichment_process_,
      double feed_qty_,
      double product_qty_,
      bool delete_outfile
  ) const;

  std::string uid;
  // File used to obtain the feed specifics, available SWU, etc. The same file
  // is also used to transfer the calculated enrichment results.
  std::string fname;
};

}  // namespace misoenrichment

#endif  // MISOENRICHMENT_SRC_PYTHON_ENRICHMENT_H_
