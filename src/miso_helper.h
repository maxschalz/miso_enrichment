#ifndef MISOENRICHMENT_SRC_MISO_HELPER_H_
#define MISOENRICHMENT_SRC_MISO_HELPER_H_

#include <map>
#include <string>
#include <vector>

#include "composition.h"
#include "material.h"

#include <nlohmann/json_fwd.hpp>

namespace misoenrichment {

namespace misotest {

bool CompareCompMap(cyclus::CompMap cm1, cyclus::CompMap cm2);
bool CompareCompMap(cyclus::CompMap cm1, cyclus::CompMap cm2, bool expected_true);
cyclus::Composition::Ptr comp_depletedU();
cyclus::Composition::Ptr comp_natU();
cyclus::Composition::Ptr comp_reprocessedU();
cyclus::Composition::Ptr comp_weapongradeU();
cyclus::Material::Ptr mat_natU();

}   // namespace misotest

const double kEpsDouble = 1e-5;
const double kEpsCompMap = 1e-5;
const int kIterMax = 200;

const std::vector<int> IsotopesNucID();
int IsotopeToNucID(int isotope);
int NucIDToIsotope(int nuc_id);
int ResBufIdx(
    const std::vector<cyclus::Composition::Ptr>& buf_compositions,
    const cyclus::Composition::Ptr& in_comp);

double MIsoAtomAssay(cyclus::Composition::Ptr comp);
double MIsoAtomAssay(cyclus::Material::Ptr rsrc);

double MIsoMassAssay(cyclus::Composition::Ptr comp);
double MIsoMassAssay(cyclus::Material::Ptr rsrc);

double MIsoAtomFrac(cyclus::Composition::Ptr composition,
                            int isotope);
double MIsoAtomFrac(cyclus::Material::Ptr rsrc, int isotope);
double MIsoMassFrac(cyclus::Composition::Ptr composition,
                            int isotope);
double MIsoMassFrac(cyclus::Material::Ptr rsrc, int isotope);

double MIsoAssay(cyclus::CompMap compmap);
double MIsoFrac(cyclus::CompMap compmap, int isotope);

// Calculates the stage separation factor for all isotopes starting from
// the given U235 overall separation factor.
//
// Returns a map containing the stage separation factors for all U isotopes
// with the keys being the isotopes' mass.
//
// Note that the stage separation factor is defined as the ratio of
// abundance ratio in product to abundance ratio in tails. This method
// follows Houston G. Wood 'Effects of Separation Processes on Minor
// Uranium Isotopes in Enrichment Cascades'. In: Science and Global
// Security, 16:26--36 (2008). ISSN: 0892-9882.
// DOI: 10.1080/08929880802361796
std::map<int,double> CalculateSeparationFactor(double gamma_235,
                                               std::string enrichment_process);

// Create a CompMap from a JSON file. 'key' must be "feed_composition",
// "product_composition" or "tails_composition".
cyclus::CompMap AtomCompMapFromJson(nlohmann::json obj, std::string key);

// Create a unique identifier based on the system time and on prefix and suffix
// provided by the user. The string will have the form
// 'prefix_{SYSTEMTIME}_suffix'.
std::string CreateUid(std::string prefix, std::string suffix);

}  // namespace misoenrichment

#endif  // MISOENRICHMENT_SRC_MISO_HELPER_H_
