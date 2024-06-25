#ifndef MISOENRICHMENT_SRC_IRM_REACTOR_TESTS_H_
#define MISOENRICHMENT_SRC_IRM_REACTOR_TESTS_H_

#include <gtest/gtest.h>

#include "irm_reactor.h"

namespace misoenrichment {

class IRMReactorTest : public ::testing::Test {
 protected:
  IRMReactor* irm_reactor_facility;

  void SetUp();
  void TearDown();

};

}  // namespace misoenrichment

#endif  // MISOENRICHMENT_SRC_IRM_REACTOR_TESTS_H_
