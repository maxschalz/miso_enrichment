#include "irm_reactor_tests.h"

#include "agent_tests.h"
#include "env.h"
#include "facility_tests.h"
#include "pyhooks.h"

namespace misoenrichment {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRMReactorTest::SetUp() {
  cyclus::PyStart();
  cyclus::Env::SetNucDataPath();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRMReactorTest::TearDown() {
  cyclus::PyStop();
}

}  // namespace misoenrichment

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Do not touch! Below section is required for the connection with Cyclus.
cyclus::Agent* IRMReactorConstructor(cyclus::Context* ctx) {
  return new misoenrichment::IRMReactor(ctx);
}

// Required to get functionality in Cyclus agent unit tests library
#ifndef CYCLUS_AGENT_TESTS_CONNECTED
int ConnectAgentTests();
static int cyclus_agent_tests_connected = ConnectAgentTests();
#define CYCLUS_AGENT_TESTS_CONNECTED
#endif  // CYCLUS_AGENT_TESTS_CONNECTED
INSTANTIATE_TEST_SUITE_P(IRMReactor, FacilityTests,
                         ::testing::Values(&IRMReactorConstructor));
INSTANTIATE_TEST_SUITE_P(IRMReactor, AgentTests,
                         ::testing::Values(&IRMReactorConstructor));
