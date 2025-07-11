#ifndef MISOENRICHMENT_SRC_MISO_ENRICH_H_
#define MISOENRICHMENT_SRC_MISO_ENRICH_H_

#include <string>
#include <vector>

#include "cyclus.h"

#include "enrichment_calculator.h"
#include "flexible_input.cc"
#include "miso_helper.h"

namespace misoenrichment {

class SwuConverter : public cyclus::Converter<cyclus::Material> {
 public:
  SwuConverter(cyclus::Composition::Ptr feed_comp, double tails_assay,
               double gamma_235, std::string enrichment_process,
               bool use_downblending, bool use_integer_stages)
      : feed_comp_(feed_comp), gamma_235_(gamma_235),
        enrichment_process_(enrichment_process),
        tails_assay_(tails_assay), use_downblending(use_downblending),
        use_integer_stages(use_integer_stages) {}

  virtual ~SwuConverter() {}

  virtual double convert(
      cyclus::Material::Ptr m, cyclus::Arc const * a = NULL,
      cyclus::ExchangeTranslationContext<cyclus::Material>
          const * ctx = NULL) const {

    double product_qty = m->quantity();
    double product_assay = MIsoAtomAssay(m);
    EnrichmentCalculator e(feed_comp_, product_assay, tails_assay_, gamma_235_,
                           enrichment_process_,
                           1e299, product_qty, 1e299, use_downblending,
                           use_integer_stages);
    double swu_used = e.SwuUsed();

    return swu_used;
  }

  virtual bool operator==(Converter& other) const {
    SwuConverter* cast = dynamic_cast<SwuConverter*>(&other);

    bool cast_not_null = cast != NULL;
    bool feed_eq = cyclus::compmath::AlmostEq(feed_comp_->atom(),
                                              cast->feed_comp_->atom(),
                                              kEpsCompMap);
    bool tails_eq = tails_assay_ == cast->tails_assay_;

    return cast != NULL && feed_eq && tails_eq;
  }

 private:
  bool use_downblending;
  bool use_integer_stages;
  cyclus::Composition::Ptr feed_comp_;
  double gamma_235_;
  std::string enrichment_process_;
  double tails_assay_;
};

class FeedConverter : public cyclus::Converter<cyclus::Material> {
 public:
  FeedConverter(cyclus::Composition::Ptr feed_comp, double tails_assay,
                double gamma_235, std::string enrichment_process,
                bool use_downblending,
                bool use_integer_stages)
      : feed_comp_(feed_comp), gamma_235_(gamma_235),
        enrichment_process_(enrichment_process),
        tails_assay_(tails_assay), use_downblending(use_downblending),
        use_integer_stages(use_integer_stages) {}

  virtual ~FeedConverter() {}

  virtual double convert(
      cyclus::Material::Ptr m, cyclus::Arc const * a = NULL,
      cyclus::ExchangeTranslationContext<cyclus::Material>
          const * ctx = NULL) const {

    double product_qty = m->quantity();
    double product_assay = MIsoAtomAssay(m);
    EnrichmentCalculator e(feed_comp_, product_assay, tails_assay_, gamma_235_,
                           enrichment_process_,
                           1e299, product_qty, 1e299, use_downblending,
                           use_integer_stages);
    double feed_used = e.FeedUsed();

    cyclus::toolkit::MatQuery mq(m);
    std::vector<int> isotopes(IsotopesNucID());
    std::set<int> nucs(isotopes.begin(), isotopes.end());
    double feed_uranium_frac = mq.atom_frac(nucs);

    return feed_used / feed_uranium_frac;
  }

  virtual bool operator==(Converter& other) const {
    FeedConverter* cast = dynamic_cast<FeedConverter*>(&other);

    bool cast_not_null = cast != NULL;
    bool feed_eq = cyclus::compmath::AlmostEq(feed_comp_->atom(),
                                              cast->feed_comp_->atom(),
                                              kEpsCompMap);
    bool tails_eq = tails_assay_ == cast->tails_assay_;

    return cast != NULL && feed_eq && tails_eq;
  }

 private:
  bool use_downblending;
  bool use_integer_stages;
  cyclus::Composition::Ptr feed_comp_;
  double gamma_235_;
  std::string enrichment_process_;
  double tails_assay_;
};

/// @class MIsoEnrich
///
/// @section intro
///
/// @section agentparams
///
/// @section optionalparams
///
/// @section detailed
class MIsoEnrich : public cyclus::Facility,
                   public cyclus::toolkit::Position {
 public:
  /// Constructor for MIsoEnrich Class
  /// @param ctx the cyclus context for access to simulation-wide parameters
  explicit MIsoEnrich(cyclus::Context* ctx);

  ~MIsoEnrich();

  friend class MIsoEnrichTest;

  #pragma cyclus

  #pragma cyclus note {"doc": "A stub facility is provided as a skeleton " \
                              "for the design of new facility agents."}

  void EnterNotify();
  void Tick();
  void Tock();
  void AdjustMatlPrefs(cyclus::PrefMap<cyclus::Material>::type& prefs);
  void AcceptMatlTrades(
      const std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                                  cyclus::Material::Ptr> >& responses);
  void GetMatlTrades(
      const std::vector<cyclus::Trade<cyclus::Material> >& trades,
      std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                            cyclus::Material::Ptr> >& responses);


  std::set<cyclus::BidPortfolio<cyclus::Material>::Ptr>
      GetMatlBids(cyclus::CommodMap<cyclus::Material>::type&
                  commod_requests);
  std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr>
      GetMatlRequests();
  std::string str();

 private:
  void AddMat_(cyclus::Material::Ptr mat);

  void AddFeedMat_(cyclus::Material::Ptr mat);

  cyclus::Material::Ptr Request_();

  // The Offer function only considers U235 content that needs to be
  // achieved and it ignores the minor isotopes. This has the advantage
  // that the evolution of minor isotopes does not need to be taken into
  // account when performing requests to a MIsoEnrich facility.
  cyclus::Material::Ptr Offer_(cyclus::Material::Ptr req);

  cyclus::Material::Ptr Enrich_(cyclus::Material::Ptr mat, double qty);

  bool ValidReq_(const cyclus::Material::Ptr& mat);

  ///  @brief records and enrichment with the cyclus::Recorder
  void RecordEnrichment_(double feed_qty, double swu, int feed_inv_idx);

  /// Records an agent's latitude and longitude to the output db
  void RecordPosition();

  #pragma cyclus var { \
    "tooltip": "feed commodity", \
    "doc": "feed commodity that the enrichment facility accepts",	\
    "uilabel": "Feed Commodity", \
    "uitype": "incommodity" \
  }
  std::string feed_commod;

  #pragma cyclus var { \
    "tooltip": "feed recipe",	\
    "doc": "recipe for enrichment facility feed commodity", \
    "uilabel": "Feed Recipe", \
    "uitype": "inrecipe" \
  }
  std::string feed_recipe;

  #pragma cyclus var { \
    "tooltip": "product commodity",	\
    "doc": "product commodity that the enrichment facility generates", \
    "uilabel": "Product Commodity", \
    "uitype": "outcommodity" \
  }
  std::string product_commod;

  #pragma cyclus var { \
    "tooltip": "tails commodity",	\
    "doc": "tails commodity supplied by enrichment facility", \
    "uilabel": "Tails Commodity", \
    "uitype": "outcommodity" \
  }
  std::string tails_commod;

  #pragma cyclus var { \
    "default": 0.003, \
    "tooltip": "tails assay (atom fraction)",	\
    "uilabel": "Tails Assay (atom fraction)", \
    "uitype": "range", \
    "doc": "tails assay from the enrichment process as atom/mole fraction", \
  }
  double tails_assay;

  #pragma cyclus var { \
    "default": 0, "tooltip": "initial uranium reserves (kg)",	\
    "uilabel": "Initial Feed Inventory", \
    "doc": "amount of natural uranium stored at the enrichment " \
    "facility at the beginning of the simulation (kg)" \
  }
  double initial_feed;

  #pragma cyclus var { \
    "default": 1e299, "tooltip": "max inventory of feed material (kg)", \
    "uilabel": "Maximum Feed Inventory", \
    "uitype": "range", \
    "range": [0.0, 1e299], \
    "doc": "maximum total inventory of natural uranium in "	\
           "the enrichment facility (kg)" \
  }
  double max_feed_inventory;

  #pragma cyclus var { \
    "default": 1.0,	\
    "tooltip": "maximum allowed enrichment fraction", \
    "doc": "maximum allowed atom/mole fraction of U235 in product", \
    "uilabel": "Maximum Allowed Enrichment", \
    "uitype": "range", \
    "range": [0.0,1.0], \
    "schema": '<optional>' \
        '          <element name="max_enrich">' \
        '              <data type="double">' \
        '                  <param name="minInclusive">0</param>' \
        '                  <param name="maxInclusive">1</param>' \
        '              </data>'	\
        '          </element>' \
        '      </optional>'	\
  }
  double max_enrich;

  #pragma cyclus var { \
    "default": 1,	\
    "userlevel": 10, \
    "tooltip": "Rank Material Requests by U235 Content", \
    "uilabel": "Prefer feed with higher U235 content", \
    "doc": "turn on preference ordering for input material " \
           "so that EF chooses higher U235 content first" \
  }
  bool order_prefs;

  #pragma cyclus var { \
    "default": 1.4, \
    "tooltip": "Separation factor U235", \
    "uilabel": "Separation factor for U235", \
    "doc": "overall stage separation factor for U235" \
  }
  double gamma_235;

  #pragma cyclus var { \
    "tooltip": "Enrichment process used, must be 'diffusion' or 'centrifuge'", \
    "uilabel": "Enrichment process used, must be 'diffusion' or 'centrifuge'", \
    "doc": "Enrichment process used, must be 'diffusion' or 'centrifuge'", \
  }
  std::string enrichment_process;

  double swu_capacity;
  double current_swu_capacity;

  double intra_timestep_swu;
  double intra_timestep_feed;

  // TODO think about how to include these variables in preprocessor
  //#pragma cyclus var {}
  std::vector<cyclus::toolkit::ResBuf<cyclus::Material> > feed_inv;
  //#pragma cyclus var {}
  std::vector<cyclus::Composition::Ptr> feed_inv_comp;

  int feed_idx;

  #pragma cyclus var {}
  cyclus::toolkit::ResBuf<cyclus::Material> tails_inv;

  #pragma cyclus var { \
    "default": 0.0, \
    "uilabel": "Geographical latitude in degrees as a double", \
    "doc": "Latitude of the agent's geographical position. The value " \
           "should be expressed in degrees as a double." \
  }
  double latitude;

  #pragma cyclus var { \
    "default": 0.0, \
    "uilabel": "Geographical longitude in degrees as a double", \
    "doc": "Longitude of the agent's geographical position. The value " \
           "should be expressed in degrees as a double." \
  }
  double longitude;

  cyclus::toolkit::Position coordinates;

  #pragma cyclus var {  \
    "default": [-1],  \
    "tooltip": "SWU capacity change times in timesteps from beginning " \
               "of deployment",  \
    "uilabel": "SWU capacity change times",  \
    "doc": "list of timesteps where the SWU is changed. The first " \
           "timestep has to be 0 as it sets the initial value and all " \
           "timesteps are measured from the moment of deployment of the " \
           "facility, not from the start of the simulation."  \
  }
  std::vector<int> swu_capacity_times;

  #pragma cyclus var {  \
    "default": [1e299],  \
    "tooltip": "SWU capacity list (kg SWU / month)",  \
    "uilabel": "SWU Capacity List",  \
    "doc": "list of separative work unit (SWU) capacity of enrichment " \
           "facility (kg SWU / month)"  \
  }
  std::vector<double> swu_capacity_vals;
  FlexibleInput<double> swu_flexible;

  #pragma cyclus var {  \
    "default": 1,  \
    "tooltip": "Downblend material",  \
    "uilabel": "Downblend the product to required enrichment level",  \
    "doc": "If set to true and if the enriched product exceeds the "  \
           "desired enrichment level, the product is downblended using "  \
           "enrichment feed to match the desired level. If this variable "  \
           "is set to 'true',  then 'use_integer_stages' must be 'true', as "  \
           "well."  \
  }
  bool use_downblending;

  #pragma cyclus var {  \
    "default": 1,  \
    "tooltip": "Use an integer number of stages",  \
    "uilabel": "Use an integer number of stages",  \
    "doc": "If set to true (default), then an integer number of stages is "  \
           "used such that the desired product assay is reached or exceeded "  \
           "and the desired tails assay is reached or undershot. If set to "  \
           "false, then a floating point number of stages is used such that "  \
           "the desired product and tails assays are obtained."  \
  }
  bool use_integer_stages;
};

}  // namespace misoenrichment

#endif  // MISOENRICHMENT_SRC_MISO_ENRICH_H_
