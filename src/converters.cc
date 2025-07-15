#include "converters.h"

#include <nlohmann/json.hpp>

#include "miso_helper.h"  // CreateUid

namespace misoenrichment {

SwuConverter::SwuConverter(
    cyclus::Composition::Ptr feed_comp,
    double tails_assay,
    double gamma_235,
    std::string enrichment_process,
    std::string parent_id
) : feed_comp_(feed_comp),
    gamma_235_(gamma_235),
    enrichment_process_(enrichment_process),
    tails_assay_(tails_assay) {
  uid = CreateUid(parent_id, "swu_converter");
  python_enrichment = PythonEnrichment(uid);
}

double SwuConverter::convert(
    cyclus::Material::Ptr m,
    cyclus::Arc const * a,
    cyclus::ExchangeTranslationContext<cyclus::Material> const * ctx) const {

  double product_qty = m->quantity();
  double product_assay = MIsoAtomAssay(m);
  cyclus::CompMap feed_cm = feed_comp_->atom();

  double swu_used;
  double feed_qty = 1e299;
  double max_swu = 1e299;
  try {
    nlohmann::json enrichment_results = python_enrichment.RunEnrichment(
      feed_cm,
      product_assay,
      tails_assay_,
      max_swu,
      gamma_235_,
      enrichment_process_,
      feed_qty,
      product_qty
    );
    swu_used = enrichment_results["swu"];
  } catch (cyclus::Error& err) {
    std::stringstream ss;
    ss << "SWU converter " << " with feed " << " containing "
              << MIsoAtomAssay(feed_comp_) << " percent 235. Request for " << product_qty
              << " of " << product_assay << " percent enriched material.\n"
              << "PythonEnrichment msg:\n" << err.what();
    throw cyclus::ValueError(ss.str());
  }
  return swu_used;
}

bool SwuConverter::operator==(Converter& other) const {
  SwuConverter* cast = dynamic_cast<SwuConverter*>(&other);

  bool cast_not_null = cast != NULL;
  bool feed_eq = cyclus::compmath::AlmostEq(feed_comp_->atom(),
                                            cast->feed_comp_->atom(),
                                            kEpsCompMap);
  bool tails_eq = tails_assay_ == cast->tails_assay_;

  return cast != NULL && feed_eq && tails_eq;
}

FeedConverter::FeedConverter(
    cyclus::Composition::Ptr feed_comp,
    double tails_assay,
    double gamma_235,
    std::string enrichment_process,
    std::string parent_id
) : feed_comp_(feed_comp),
    gamma_235_(gamma_235),
    enrichment_process_(enrichment_process),
    tails_assay_(tails_assay) {
  uid = CreateUid(parent_id, "feed_converter");
  python_enrichment = PythonEnrichment(uid);
}

double FeedConverter::convert(
    cyclus::Material::Ptr m,
    cyclus::Arc const * a,
    cyclus::ExchangeTranslationContext<cyclus::Material> const * ctx) const {

  double product_qty = m->quantity();
  double product_assay = MIsoAtomAssay(m);
  cyclus::CompMap feed_cm = feed_comp_->atom();
  double feed_used;
  double feed_qty = 1e299;
  double max_swu = 1e299;
  try {
    nlohmann::json enrichment_results = python_enrichment.RunEnrichment(
      feed_cm,
      product_assay,
      tails_assay_,
      max_swu,
      gamma_235_,
      enrichment_process_,
      feed_qty,
      product_qty
    );
    feed_used = enrichment_results["feed_qty"];
  } catch (cyclus::Error& err) {
    std::stringstream ss;
    ss << "Feed converter " << " with feed " << " containing "
       << MIsoAtomAssay(feed_comp_) << " percent 235. precise composition:\n";
    for (auto const& x : feed_cm) {
      ss << x.first << ": " << x.second << "\n";
    }
    ss << "Request for " << product_qty
       << " of " << product_assay << " percent enriched material.\n"
       << "Enrichment calculator msg:\n" << err.what();

    throw cyclus::ValueError(ss.str());
  }

  cyclus::toolkit::MatQuery mq(m);
  std::vector<int> isotopes(IsotopesNucID());
  std::set<int> nucs(isotopes.begin(), isotopes.end());
  double feed_uranium_frac = mq.atom_frac(nucs);

  return feed_used / feed_uranium_frac;
}

bool FeedConverter::operator==(Converter& other) const {
  FeedConverter* cast = dynamic_cast<FeedConverter*>(&other);

  bool cast_not_null = cast != NULL;
  bool feed_eq = cyclus::compmath::AlmostEq(feed_comp_->atom(),
                                            cast->feed_comp_->atom(),
                                            kEpsCompMap);
  bool tails_eq = tails_assay_ == cast->tails_assay_;

  return cast != NULL && feed_eq && tails_eq;
}

}  // namespace misoenrichment
