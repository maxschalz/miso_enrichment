#include "gpr_reactor.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <sstream>
#include <utility>

// Future changes relating to the implementation of Antonio's GPRs are marked 
// with the following comment:
// TODO ANTONIO GPR

namespace misoenrichment {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GprReactor::GprReactor(cyclus::Context* ctx) 
    : cyclus::Facility(ctx),
      in_commods(std::vector<std::string>()), 
      out_commods(std::vector<std::string>()), 
      in_recipes(std::vector<std::string>()), 
      out_recipes(std::vector<std::string>()), 
      fuel_prefs(std::vector<double>()),
      n_assem_core(0), 
      n_assem_batch(0), 
      assem_size(0), 
      n_assem_fresh(0), 
      n_assem_spent(0), 
      latitude(0.), 
      longitude(0.), 
      decom_transmute_all(false), 
      cycle_time(0), 
      refuel_time(0), 
      cycle_step(0), 
      discharged(false), 
      power_output(0.), 
      res_indexes(std::map<int,int>()), 
      is_hybrid(true),
      side_products(std::vector<std::string>()), 
      side_product_quantity(std::vector<double>()),
      unique_out_commods(std::set<std::string>())
  {;}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::set<cyclus::BidPortfolio<cyclus::Material>::Ptr> GprReactor::GetMatlBids(
    cyclus::CommodMap<cyclus::Material>::type& commod_requests) {
  using cyclus::BidPortfolio;
  using cyclus::Material;
  
  // TODO ensure that if statement below yields true despite initialiser list
  // in the class' constructor!
  if (unique_out_commods.empty()) {
    // TODO delete std::cout statement.
    std::cout << "\n\nGprRea: Confirmation that `unique_out_commods` is empty\n";
    for (int i = 0; i < out_commods.size(); ++i) {
      unique_out_commods.insert(out_commods[i]);
    }
  }

  std::set<BidPortfolio<Material>::Ptr> ports;
  std::map<std::string, cyclus::toolkit::MatVec> all_mats;
  std::set<std::string>::iterator it;
  // Loop over all out commodities.
  for (it = unique_out_commods.begin(); it != unique_out_commods.end(); ++it) {
    std::string commod = *it;
    std::vector<cyclus::Request<Material>*>& reqs = commod_requests[commod];
    if (reqs.size() == 0) {
      continue;
    } else {
      all_mats = PeekSpent_();
    }
    cyclus::toolkit::MatVec mats = all_mats[commod];
    if (mats.size() == 0) {
      continue;
    }
    BidPortfolio<Material>::Ptr port(new BidPortfolio<Material>());
    // Loop over all requests for the given out commodity.
    for (int j = 0; j < reqs.size(); ++j) {
      cyclus::Request<Material>* req = reqs[j];
      double total_bid = 0;
      // Loop over all available material of the given commidity.
      for (int k = 0; k < mats.size(); ++k) {
        Material::Ptr m = mats[k];
        total_bid += m->quantity();
        port->AddBid(req, m, this, true);
        if (total_bid >= req->target()->quantity()) {
          break;
        }
      }
    }
    double total_qty = 0;
    for (int j = 0; j < mats.size(); ++j) {
      total_qty += mats[j]->quantity();
    }
    cyclus::CapacityConstraint<Material> cap_constraint(total_qty);
    port->AddConstraint(cap_constraint);
    ports.insert(port);
  }
  return ports;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr> 
GprReactor::GetMatlRequests() {
  using cyclus::Material;
  using cyclus::Request;
  using cyclus::RequestPortfolio;

  std::set<RequestPortfolio<Material>::Ptr> ports;
  Material::Ptr m;

  int n_assem_order = n_assem_core - core.count() 
                      + n_assem_fresh - fresh_inv.count();
  // This if clause accounts for the fact that less assemblies may be needed if
  // the reactor retirement is near.
  if (exit_time() != -1) {
    // The '+ 1' accounts for the fact that the reactor is online and gets to
    // operate during its exit_time timestep.
    int time_left = exit_time() - context()->time() + 1;
    int time_left_cycle = cycle_time + refuel_time - cycle_step;
    double n_cycles_left = static_cast<double>(time_left - time_left_cycle)
                           / static_cast<double>(cycle_time + refuel_time);
    n_cycles_left = std::ceil(n_cycles_left);
    int n_needed = std::max(0.0, n_cycles_left*n_assem_batch - n_assem_fresh 
                                 + n_assem_core - core.count());
    n_assem_order = std::min(n_assem_order, n_needed);
  }

  if (n_assem_order == 0 || Retired_()) {
    return ports; 
  }

  // Make a request for each assembly.
  for (int i = 0; i < n_assem_order; ++i) {
    RequestPortfolio<Material>::Ptr port(new RequestPortfolio<Material>());
    std::vector<Request<Material>*> mutual_reqs;
    // Make mutual requests for each fuel incommod.
    for (int j = 0; j < in_commods.size(); ++j) {
      std::string commod = in_commods[j];
      double pref = fuel_prefs[j];
      cyclus::Composition::Ptr recipe = context()->GetRecipe(in_recipes[j]);
      m = Material::CreateUntracked(assem_size, recipe);

      Request<Material>* r = port->AddRequest(m, this, commod, pref, true);
      mutual_reqs.push_back(r);
    }
    std::vector<double>::iterator result;
    result = std::max_element(fuel_prefs.begin(), fuel_prefs.end());
    int max_index = std::distance(fuel_prefs.begin(), result);
    
    cyclus::toolkit::RecordTimeSeries<double>("demand" + in_commods[max_index],
                                              this, assem_size*n_assem_order);
    port->AddMutualReqs(mutual_reqs);
    ports.insert(port);
  }
  return ports;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string GprReactor::str() { return cyclus::Facility::str(); }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GprReactor::AcceptMatlTrades(
    const std::vector<std::pair<cyclus::Trade<cyclus::Material>, 
                                cyclus::Material::Ptr> >& responses) {
  std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                        cyclus::Material::Ptr> >::const_iterator trade;
  std::stringstream ss;
  // Number of assemblies that are loaded directly into the core.
  int n_load = std::min((int)responses.size(), n_assem_core - core.count());
  if (n_load > 0) {
    ss << n_load << " assemblies";
    Record_("LOAD", ss.str());
  }

  // Accept trades and push material to core or fresh fuel inventory.
  for (trade = responses.begin(); trade != responses.end(); ++trade) {
    std::string commod = trade->first.request->commodity();
    cyclus::Material::Ptr m = trade->second;
    IndexRes_(m, commod);

    if (core.count() < n_assem_core) {
      core.Push(m);
    } else {
      // TODO check if it is assured that the fresh inventory does not obtain 
      // more fuel than it can accept.
      fresh_inv.Push(m);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GprReactor::EnterNotify() {
  cyclus::Facility::EnterNotify();

  if (fuel_prefs.size() == 0) {
    for (int i = 0; i < out_commods.size(); i++) {
      fuel_prefs.push_back(cyclus::kDefaultPref);
    }
  }

  // Check if side products have been defined.
  if (side_products.size() == 0) {
    is_hybrid = false;
  }
  RecordPosition_();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GprReactor::GetMatlTrades(
    const std::vector<cyclus::Trade<cyclus::Material> >& trades,
    std::vector<std::pair<cyclus::Trade<cyclus::Material>, 
                                        cyclus::Material::Ptr> >& responses) {
  std::map<std::string, cyclus::toolkit::MatVec> mats = PopSpent_();
  for (int i = 0; i < trades.size(); i++) {
    std::string commod = trades[i].request->commodity();
    cyclus::Material::Ptr m = mats[commod].back();
    mats[commod].pop_back();
    responses.push_back(std::make_pair(trades[i], m));
    res_indexes.erase(m->obj_id());
  }
  PushSpent_(mats);  // return leftovers back to spent buffer
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GprReactor::Tick() {
  // Check if the reactor is already retired.
  if (Retired_()) {
    Record_("RETIRED", "");
    // Transmute remaining fuel exactly once.
    if (context()->time() == exit_time()+1) {
      if (decom_transmute_all == true) {
        Transmute_(std::ceil(static_cast<double>(n_assem_core)));
      }
      else {
        Transmute_(std::ceil(static_cast<double>(n_assem_core)/2.));
      }
    }
    // Empty the reactor core if this has not yet been done.
    while (core.count() > 0) {
      if (!Discharge_()) {
        break;
      }
    }

    while (fresh_inv.count() > 0 && spent_inv.space() >= assem_size) {
      spent_inv.Push(fresh_inv.Pop());
    }
    if (CheckDecommissionCondition()) {
      Decommission();
    }
    return;
  }

  // "Burn" the fuel, i.e., change its composition from fresh to spent fuel.
  if (cycle_step == cycle_time) {
    Transmute_();
    Record_("CYCLE_END", "");
  }

  // If the irradiation period is over and the fuel has not yet been discharged,
  // e.g., because of a full spent fuel inventory, then discharge it now if 
  // possible.
  if (cycle_step >= cycle_time && !discharged) {
    discharged = Discharge_();
  }
  
  // If the irradiation period is over, try to load fresh fuel into the reactor
  // core.
  if (cycle_step >= cycle_time) {
    Load_();
  }

  // In cycamore's Reactor implementation, the changing of preferences and 
  // recipes would take place here.
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GprReactor::Tock() {
  using cyclus::toolkit::RecordTimeSeries;

  if (Retired_()) {
    return;
  }

  // Check that the irradiation and refuelling period is over, that the core is
  // full and that the spent fuel was successfully discharged in this refuelling
  // period. If all of this is the case, start a new cycle.
  if (cycle_step >= cycle_time+refuel_time && core.count() == n_assem_core 
      && discharged == true) {
    discharged = false;
    cycle_step = 0;
  }
  
  if (cycle_step == 0 && core.count() == n_assem_core) {
    Record_("CYCLE_START", "");
  }

  // Normal reactor operation where power is produced
  if (cycle_step >= 0 && cycle_step < cycle_time 
      && core.count() == n_assem_core) {
    RecordTimeSeries<cyclus::toolkit::POWER>(this, power_output);
    RecordTimeSeries<double>("supplyPOWER", this, power_output);
    RecordSideProduct_(true);
  } else {
    RecordTimeSeries<cyclus::toolkit::POWER>(this, 0);
    RecordTimeSeries<double>("supplyPOWER", this, 0);
    RecordSideProduct_(false);
  }

  // This statement prevents that a newly deployed reactor (`cycle_step = 0`) 
  // increments `cycle_step` although the core might not have been filled yet.
  if (cycle_step > 0 || core.count() == n_assem_core) {
    cycle_step++;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Below are the private helper functions, not directly interacting with CYCLUS.
bool GprReactor::CheckDecommissionCondition() {
  return core.count()==0 && spent_inv.count()==0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GprReactor::Discharge_() {
  int n_pop = std::min(n_assem_batch, core.count());
  if (n_assem_spent-spent_inv.count() < n_pop) {
    // Not enough room in spent fuel buffer.
    Record_("DISCHARGE", "failed");
    return false;
  }
  std::stringstream ss;
  ss << n_pop << " assemblies";
  Record_("DISCHARGE", ss.str());
  spent_inv.Push(core.PopN(n_pop));

  std::map<std::string, cyclus::toolkit::MatVec> spent_mats;
  for (std::string commod : out_commods) {
    spent_mats = PeekSpent_();
    cyclus::toolkit::MatVec mats = spent_mats[commod];
    double total_spent = 0.;
    for (cyclus::Material::Ptr mat_ptr : mats) {
      total_spent += mat_ptr->quantity();
    }
    cyclus::toolkit::RecordTimeSeries<double>("supply"+commod, this, 
                                              total_spent);
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GprReactor::Retired_() {
 return exit_time() != -1 && context()->time() > exit_time();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::map<std::string, cyclus::toolkit::MatVec> GprReactor::PeekSpent_() {
  std::map<std::string, cyclus::toolkit::MatVec> mapped;
  cyclus::toolkit::MatVec mats = spent_inv.PopN(spent_inv.count());
  spent_inv.Push(mats);
  for (cyclus::Material::Ptr mat_ptr : mats) {
    std::string commod = OutCommod_(mat_ptr);
    mapped[commod].push_back(mat_ptr);
  }
  return mapped;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::map<std::string, cyclus::toolkit::MatVec> GprReactor::PopSpent_() {
  cyclus::toolkit::MatVec mats = spent_inv.PopN(spent_inv.count());
  std::map<std::string, cyclus::toolkit::MatVec> mapped;
  for (int i = 0; i < mats.size(); ++i) {
    std::string commod = OutCommod_(mats[i]);
    mapped[commod].push_back(mats[i]);
  }

  // Reverse to ensure oldest assemblies are traded away first.
  // TODO check this in cycamore Reactor, unsure why this is needed.
  std::map<std::string, cyclus::toolkit::MatVec>::iterator it;
  for (it = mapped.begin(); it != mapped.end(); ++it) {
    std::reverse(it->second.begin(), it->second.end());
  }
  return mapped;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
std::string GprReactor::InCommod_(cyclus::Material::Ptr m) {
  int i = res_indexes[m->obj_id()];
  if (i >= in_commods.size()) {
    throw cyclus::KeyError("misoenrichment::GprReactor - no incommod for material object");
  }
  return in_commods[i];
}
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string GprReactor::OutCommod_(cyclus::Material::Ptr m) {
  int i = res_indexes[m->obj_id()];
  if (i >= out_commods.size()) {
    throw cyclus::KeyError("misoenrichment::GprReactor - no outcommod for material object");
  }
  return out_commods[i];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
std::string GprReactor::InRecipe_(cyclus::Material::Ptr m) {
  int i = res_indexes[m->obj_id()];
  if (i >= in_recipes.size()) {
    throw cyclus::KeyError("misoenrichment::GprReactor - no inrecipe for material object");
  }
  return in_recipes[i];
}
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string GprReactor::OutRecipe_(cyclus::Material::Ptr m) {
  int i = res_indexes[m->obj_id()];
  if (i >= out_recipes.size()) {
    throw cyclus::KeyError("misoenrichment::GprReactor - no outrecipe for material object");
  }
  return out_recipes[i];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GprReactor::IndexRes_(cyclus::Resource::Ptr m, std::string incommod) {
  for (int i = 0; i < in_commods.size(); ++i) {
    if (in_commods[i] == incommod) {
      res_indexes[m->obj_id()] = i;
      return;
    }
  }
  throw cyclus::ValueError(
      "misoenrichment::GprReactor - received unsupported incommod material.");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GprReactor::Load_() {
  int n_load = std::min(n_assem_core - core.count(), fresh_inv.count());
  if (n_load == 0) {
    return;
  }
  std::stringstream ss;
  ss << n_load << " assemblies";
  Record_("LOAD", ss.str());
  core.Push(fresh_inv.PopN(n_load));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GprReactor::PushSpent_(std::map<std::string, 
                                     cyclus::toolkit::MatVec> mats) {
  std::map<std::string, cyclus::toolkit::MatVec>::iterator it;
  for (it = mats.begin(); it != mats.end(); ++it) {
    // Undo reverse in PopSpent to ensure oldest assemblies come out first.
    // TODO check this in cycamore Reactor, unsure why this is needed.
    std::reverse(it->second.begin(), it->second.end());
    spent_inv.Push(it->second);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GprReactor::Record_(std::string name, std::string val) {
  context()->NewDatum("ReactorEvents")
           ->AddVal("AgentId", id())
           ->AddVal("Time", context()->time())
           ->AddVal("Event", name)
           ->AddVal("Value", val)
           ->Record();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GprReactor::RecordPosition_() {
  context()->NewDatum("AgentPosition")
           ->AddVal("Spec", this->spec())
           ->AddVal("Prototype", this->prototype())
           ->AddVal("AgentId", id())
           ->AddVal("Latitude", latitude)
           ->AddVal("Longitude", longitude)
           ->Record();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GprReactor::RecordSideProduct_(bool is_producing) {
  if (is_hybrid) {
    double value;
    for (int i = 0; i < side_products.size(); ++i) {
      if (is_producing) {
        value = side_product_quantity[i];
      }
      else {
        value = 0;
      }
      context()->NewDatum("ReactorSideProducts")
               ->AddVal("AgentId", id())
               ->AddVal("Time", context()->time())
               ->AddVal("Product", side_products[i])
               ->AddVal("Value", value)
               ->Record();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GprReactor::Transmute_() { Transmute_(n_assem_batch); }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GprReactor::Transmute_(int n_assem) {
  cyclus::toolkit::MatVec old = core.PopN(std::min(n_assem, core.count()));
  core.Push(old);
  if (core.count() > old.size()) {
    // Rotate the untransmuted materials back to the back of the buffer.
    core.Push(core.PopN(core.count() - old.size()));
  }
  std::stringstream ss;
  ss << old.size() << " assemblies";
  Record_("TRANSMUTE", ss.str());

  // TODO somewhere at this place, Antonio's GPRs must be used.
  // TODO ANTONIO GPR
  for (int i = 0; i < old.size(); ++i) {
    old[i]->Transmute(context()->GetRecipe(OutRecipe_(old[i])));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// WARNING! Do not change the following this function!!! This enables your
// archetype to be dynamically loaded and any alterations will cause your
// archetype to fail.
extern "C" cyclus::Agent* ConstructGprReactor(cyclus::Context* ctx) {
  return new GprReactor(ctx);
}

}  // namespace misoenrichment
