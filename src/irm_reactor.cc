#include "irm_reactor.h"

#include <sstream>

namespace misoenrichment {

IRMReactor::IRMReactor(cyclus::Context* ctx)
    : cyclus::Facility(ctx),
      assem_size(0.),
      n_assem_batch(0),
      n_assem_core(0),
      n_assem_fresh(0),
      n_assem_spent(0),
      cycle_time(0),
      refuel_time(0),
      cycle_step(0),
      power_cap(0.),
      power_name("power"),
      latitude(0.),
      longitude(0.),
      coordinates(latitude, longitude) {}

/*
TODO go through this one by one

#pragma cyclus def clone misoenrichment::IRMReactor

#pragma cyclus def schema misoenrichment::IRMReactor

#pragma cyclus def annotations misoenrichment::IRMReactor

#pragma cyclus def infiletodb misoenrichment::IRMReactor

#pragma cyclus def snapshot misoenrichment::IRMReactor

#pragma cyclus def snapshotinv misoenrichment::IRMReactor

#pragma cyclus def initinv misoenrichment::IRMReactor

void IRMReactor::InitFrom(IRMReactor* m) {
  #pragma cyclus impl initfromcopy misoenrichment::IRMReactor
  cyclus::toolkit::CommodityProducer::Copy(m);
}

void IRMReactor::InitFrom(cyclus::QueryableBackend* b) {
  #pragma cyclus impl initfromdb misoenrichment::IRMReactor

  namespace tk = cyclus::toolkit;
  tk::CommodityProducer::Add(tk::Commodity(power_name),
                             tk::CommodInfo(power_cap, power_cap));

  for (int i = 0; i < side_products.size(); i++) {
    tk::CommodityProducer::Add(tk::Commodity(side_products[i]),
                               tk::CommodInfo(side_product_quantity[i],
                                              side_product_quantity[i]));
  }
}
*/

void IRMReactor::EnterNotify() {
  cyclus::Facility::EnterNotify();

  // Set all fuel preferences to a default value if unspecified by the user.
  if (fuel_prefs.size() == 0) {
    for (int i = 0; i < fuel_outcommods.size(); i++) {
      fuel_prefs.push_back(cyclus::kDefaultPref);
    }
  }

  // Check if side products have been defined.
  is_hybrid_ = side_products.size();

  // Check if the input is consistent:
  //   - with regards to recipe changes
  int n = recipe_change_times.size();
  std::stringstream ss;
  if (recipe_change_commods.size() != n) {
    ss << "prototype '" << prototype() << "' has "
       << recipe_change_commods.size()
       << " recipe_change_commods vals, expected " << n << "\n";
  }
  if (recipe_change_in.size() != n) {
    ss << "prototype '" << prototype() << "' has " << recipe_change_in.size()
       << " recipe_change_in vals, expected " << n << "\n";
  }
  if (recipe_change_out.size() != n) {
    ss << "prototype '" << prototype() << "' has " << recipe_change_out.size()
       << " recipe_change_out vals, expected " << n << "\n";
  }

  //   - with regards to preference changes.
  n = pref_change_times.size();
  if (pref_change_commods.size() != n) {
    ss << "prototype '" << prototype() << "' has " << pref_change_commods.size()
       << " pref_change_commods vals, expected " << n << "\n";
  }
  if (pref_change_values.size() != n) {
    ss << "prototype '" << prototype() << "' has " << pref_change_values.size()
       << " pref_change_values vals, expected " << n << "\n";
  }
  if (ss.str().size() > 0) {
    throw cyclus::ValueError(ss.str());
  }
  RecordPosition();
}

/*
void IRMReactor::Tick() {

}

void IRMReactor::Tock() {

}
*/

void IRMReactor::RecordPosition() {
  std::string specification = this->spec();
  context()->NewDatum("AgentPosition")
           ->AddVal("Spec", specification)
           ->AddVal("Prototype", this->prototype())
           ->AddVal("AgentId", id())
           ->AddVal("Latitude", latitude)
           ->AddVal("Longitude", longitude)
           ->Record();
}

extern "C" cyclus::Agent* ConstructIRMReactor(cyclus::Context* ctx) {
  return new IRMReactor(ctx);
}

}  // namespace misoenrichment
