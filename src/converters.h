#ifndef MISOENRICHMENT_SRC_CONVERTERS_H_
#define MISOENRICHMENT_SRC_CONVERTERS_H_

#include <string>

#include "cyclus.h"

#include "python_enrichment.h"

namespace misoenrichment {

class SwuConverter : public cyclus::Converter<cyclus::Material> {
 public:
  SwuConverter(
      cyclus::Composition::Ptr feed_comp,
      double tails_assay,
      double gamma_235,
      std::string enrichment_process,
      std::string parent_id
  );
  virtual ~SwuConverter() {};
  virtual double convert(
      cyclus::Material::Ptr m, cyclus::Arc const * a = NULL,
      cyclus::ExchangeTranslationContext<cyclus::Material>
          const * ctx = NULL) const;
  virtual bool operator==(Converter& other) const;

 private:
  cyclus::Composition::Ptr feed_comp_;
  double gamma_235_;
  std::string enrichment_process_;
  double tails_assay_;
  std::string uid;
  PythonEnrichment python_enrichment;
};

class FeedConverter : public cyclus::Converter<cyclus::Material> {
 public:
  FeedConverter(
      cyclus::Composition::Ptr feed_comp,
      double tails_assay,
      double gamma_235,
      std::string enrichment_process,
      std::string parent_id
  );
  virtual ~FeedConverter() {};
  virtual double convert(
      cyclus::Material::Ptr m, cyclus::Arc const * a = NULL,
      cyclus::ExchangeTranslationContext<cyclus::Material>
          const * ctx = NULL) const;

  virtual bool operator==(Converter& other) const;

 private:
  cyclus::Composition::Ptr feed_comp_;
  double gamma_235_;
  std::string enrichment_process_;
  double tails_assay_;
  std::string uid;
  PythonEnrichment python_enrichment;
};

}  // namespace misoenrichment

#endif  // MISOENRICHMENT_SRC_CONVERTERS_H_
