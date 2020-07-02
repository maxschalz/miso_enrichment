#include "multi_isotope_helper.h"

#include <algorithm>

#include "error.h"

namespace multiisotopeenrichment {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IsotopesNucID(std::vector<int> &isotopes) {
  isotopes = {232, 233, 234, 235, 236, 238};
  for (int i = 0; i < isotopes.size(); i++) {
    isotopes[i] = (92*1000 + isotopes[i]) * 10000;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int IsotopeToNucID(int isotope) {
  std::vector<int> isotopes = {232, 233, 234, 235, 236, 238};
  std::vector<int>::iterator it;
  
  it = std::find(isotopes.begin(), isotopes.end(), isotope);
  if (it == isotopes.end()) {
    throw cyclus::ValueError("Invalid (non-uranium) isotope!");
  }
  return (92*1000 + isotope) * 10000;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int NucIDToIsotope(int nuc_id) {
  std::vector<int> isotopes;
  IsotopesNucID(isotopes);
  std::vector<int>::iterator it;
  
  it = std::find(isotopes.begin(), isotopes.end(), nuc_id);
  if (it == isotopes.end()) {
    throw cyclus::ValueError("Invalid (non-uranium) isotope!");
  }
  return nuc_id/10000 - 92*1000;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double MultiIsotopeAtomAssay(cyclus::Composition::Ptr comp) {
  return MultiIsotopeAtomFrac(comp, IsotopeToNucID(235));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double MultiIsotopeAtomAssay(cyclus::Material::Ptr rsrc) {
  return MultiIsotopeAtomFrac(rsrc, IsotopeToNucID(235));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double MultiIsotopeAtomAssay(std::map<int,double> compmap) {
  return MultiIsotopeAtomFrac(compmap, IsotopeToNucID(235));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double MultiIsotopeMassAssay(cyclus::Composition::Ptr comp) {
  return MultiIsotopeMassFrac(comp, IsotopeToNucID(235));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double MultiIsotopeMassAssay(cyclus::Material::Ptr rsrc) {
  return MultiIsotopeMassFrac(rsrc, IsotopeToNucID(235));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double MultiIsotopeMassAssay(std::map<int,double> compmap) {
  return MultiIsotopeMassFrac(compmap, IsotopeToNucID(235));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double MultiIsotopeAtomFrac(cyclus::Composition::Ptr composition, 
                            int isotope) {
  return MultiIsotopeAtomFrac(composition->atom(), isotope);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double MultiIsotopeAtomFrac(cyclus::Material::Ptr rsrc, int isotope) {
  return MultiIsotopeAtomFrac(rsrc->comp(), isotope);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double MultiIsotopeAtomFrac(cyclus::CompMap compmap, int isotope) {
  std::vector<int> isotopes;
  IsotopesNucID(isotopes);
  
  double isotope_assay;
  double uranium_atom_frac = 0;

  // Get total uranium mole fraction, all non-uranium elements are not 
  // considered here as they are directly sent to the tails.
  for (int i : isotopes) {
    if (compmap.find(i) != compmap.end()) {
      uranium_atom_frac += compmap.at(i);
      if (i==isotope) {
        isotope_assay = compmap.at(i);
      }
    }
  }
  isotope_assay /= uranium_atom_frac;
  return isotope_assay;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double MultiIsotopeMassFrac(cyclus::Composition::Ptr composition, 
                            int isotope) {
  return MultiIsotopeMassFrac(composition->mass(), isotope);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double MultiIsotopeMassFrac(cyclus::Material::Ptr rsrc, int isotope) {
  return MultiIsotopeMassFrac(rsrc->comp(), isotope);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double MultiIsotopeMassFrac(std::map<int,double> compmap, int isotope) {
  std::vector<int> isotopes;
  IsotopesNucID(isotopes);

  double isotope_assay;
  double uranium_mass_frac = 0;

  // Get total uranium mass fraction, all non-uranium elements are not 
  // considered here as they are directly sent to the tails.
  for (int i : isotopes) {
    if (compmap.find(i) != compmap.end()) {
      uranium_mass_frac += compmap.at(i);
      if (i==isotope) {
        isotope_assay = compmap.at(i);
      }
    }
  }
  isotope_assay /= uranium_mass_frac;
  return isotope_assay;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::map<int,double> CalculateSeparationFactor(double alpha_235) {
  std::vector<int> isotopes;
  IsotopesNucID(isotopes);
  std::map<int,double> separation_factors;
  
  // Convert the product to feed separation factor to overall stage 
  // separation factor.
  alpha_235 *= alpha_235;
  
  // We consider U-238 to be the key component hence the mass differences
  // are calculated with respect to this isotope.
  for (int i : isotopes) {
    double delta_mass = 238. - NucIDToIsotope(i);
    double alpha = 1. + delta_mass*(alpha_235-1.) / (238.-235.);
    separation_factors[i] = alpha;
    }
  return separation_factors;
}

} // namespace multiisotopeenrichment
