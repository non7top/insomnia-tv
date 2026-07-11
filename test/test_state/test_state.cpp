// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>

#include "../../src/hal/IClock.h"
#include "../../src/state/RampScheduler.h"
#include "../../src/state/SleepStateMachine.h"

using InsomniaTV::IClock;
using InsomniaTV::RampScheduler;
using InsomniaTV::SleepStateMachine;
using InsomniaTV::State;

// ---------------------------------------------------------------------------
// Minimal clock mock
// ---------------------------------------------------------------------------
class MockClock : public IClock {
public:
  uint32_t nowMs() const override { return ms_; }
  void advanceMs(uint32_t ms) override { ms_ += ms; }
  uint32_t ms_ = 0;
};

// ---------------------------------------------------------------------------
// Initial state
// ---------------------------------------------------------------------------
void test_state_machine_initial_state() {
  MockClock clk;
  SleepStateMachine sm(clk);
  TEST_ASSERT_EQUAL(State::MONITORING, sm.getCurrentState());
}

// ---------------------------------------------------------------------------
// IR activity resets to MONITORING
// ---------------------------------------------------------------------------
void test_state_machine_activity_reset() {
  MockClock clk;
  SleepStateMachine sm(clk);
  sm.onIrActivity();
  TEST_ASSERT_EQUAL(State::MONITORING, sm.getCurrentState());
}

// ---------------------------------------------------------------------------
// Inactivity timeout triggers RAMPING
// ---------------------------------------------------------------------------
void test_state_machine_inactivity_to_ramping() {
  MockClock clk;
  SleepStateMachine sm(clk, /*inactivityTimeoutMs=*/5000);
  clk.advanceMs(4999);
  sm.tick();
  TEST_ASSERT_EQUAL(State::MONITORING, sm.getCurrentState());

  clk.advanceMs(1);
  sm.tick();
  TEST_ASSERT_EQUAL(State::RAMPING, sm.getCurrentState());
}

// ---------------------------------------------------------------------------
// IR activity during RAMPING resets to MONITORING
// ---------------------------------------------------------------------------
void test_state_machine_ir_during_ramping() {
  MockClock clk;
  SleepStateMachine sm(clk, 5000);
  clk.advanceMs(6000);
  sm.tick();
  TEST_ASSERT_EQUAL(State::RAMPING, sm.getCurrentState());

  sm.onIrActivity();
  TEST_ASSERT_EQUAL(State::MONITORING, sm.getCurrentState());
}

// ---------------------------------------------------------------------------
// onRampComplete transitions RAMPING → VERIFYING
// ---------------------------------------------------------------------------
void test_state_machine_ramp_complete_to_verifying() {
  MockClock clk;
  SleepStateMachine sm(clk, 5000);
  clk.advanceMs(6000);
  sm.tick();
  TEST_ASSERT_EQUAL(State::RAMPING, sm.getCurrentState());

  sm.onRampComplete();
  TEST_ASSERT_EQUAL(State::VERIFYING, sm.getCurrentState());
}

// ---------------------------------------------------------------------------
// VERIFYING without TvStateMachine → fires power-off, returns MONITORING
// ---------------------------------------------------------------------------
void test_state_machine_verifying_no_tv_sm() {
  MockClock clk;
  SleepStateMachine sm(clk, 5000);
  int powerOffCalls = 0;
  sm.setPowerOffCallback([&] { powerOffCalls++; });

  clk.advanceMs(6000);
  sm.tick();            // → RAMPING
  sm.onRampComplete();  // → VERIFYING
  sm.tick();            // → fires callback, → MONITORING

  TEST_ASSERT_EQUAL(1, powerOffCalls);
  TEST_ASSERT_EQUAL(State::MONITORING, sm.getCurrentState());
}

// ---------------------------------------------------------------------------
// SleepStateMachine invokes the ramp-start callback on entering RAMPING
// ---------------------------------------------------------------------------
void test_state_machine_ramp_start_callback_fires() {
  MockClock clk;
  SleepStateMachine sm(clk, 5000);
  int rampStarts = 0;
  sm.setRampStartCallback([&] { rampStarts++; });

  clk.advanceMs(4999);
  sm.tick();
  TEST_ASSERT_EQUAL(0, rampStarts);

  clk.advanceMs(1);
  sm.tick();
  TEST_ASSERT_EQUAL(1, rampStarts);
  TEST_ASSERT_EQUAL(State::RAMPING, sm.getCurrentState());
}

// ---------------------------------------------------------------------------
// RampScheduler: no step before interval elapses
// ---------------------------------------------------------------------------
void test_ramp_scheduler_no_step_before_interval() {
  MockClock clk;
  int steps = 0;
  RampScheduler rs(clk, 1000, 3, [&] { steps++; }, [] {});
  rs.start();

  clk.advanceMs(999);
  rs.tick();
  TEST_ASSERT_EQUAL(0, steps);
}

// ---------------------------------------------------------------------------
// RampScheduler: fires a step once the interval elapses
// ---------------------------------------------------------------------------
void test_ramp_scheduler_fires_step_at_interval() {
  MockClock clk;
  int steps = 0;
  RampScheduler rs(clk, 1000, 3, [&] { steps++; }, [] {});
  rs.start();

  clk.advanceMs(1000);
  rs.tick();
  TEST_ASSERT_EQUAL(1, steps);
  TEST_ASSERT_EQUAL(1, rs.stepCount());
}

// ---------------------------------------------------------------------------
// RampScheduler: fires on_complete and stops after max_steps
// ---------------------------------------------------------------------------
void test_ramp_scheduler_completes_after_max_steps() {
  MockClock clk;
  int steps = 0;
  int completes = 0;
  RampScheduler rs(clk, 1000, 3, [&] { steps++; }, [&] { completes++; });
  rs.start();

  for (int i = 0; i < 3; i++) {
    clk.advanceMs(1000);
    rs.tick();
  }

  TEST_ASSERT_EQUAL(3, steps);
  TEST_ASSERT_EQUAL(1, completes);
  TEST_ASSERT_FALSE(rs.isActive());

  // No further steps once complete, even if more time passes.
  clk.advanceMs(1000);
  rs.tick();
  TEST_ASSERT_EQUAL(3, steps);
}

// ---------------------------------------------------------------------------
// RampScheduler: stop() cancels the sequence
// ---------------------------------------------------------------------------
void test_ramp_scheduler_stop_cancels() {
  MockClock clk;
  int steps = 0;
  RampScheduler rs(clk, 1000, 3, [&] { steps++; }, [] {});
  rs.start();
  rs.stop();

  clk.advanceMs(1000);
  rs.tick();
  TEST_ASSERT_EQUAL(0, steps);
  TEST_ASSERT_FALSE(rs.isActive());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_state_machine_initial_state);
  RUN_TEST(test_state_machine_activity_reset);
  RUN_TEST(test_state_machine_inactivity_to_ramping);
  RUN_TEST(test_state_machine_ir_during_ramping);
  RUN_TEST(test_state_machine_ramp_complete_to_verifying);
  RUN_TEST(test_state_machine_verifying_no_tv_sm);
  RUN_TEST(test_state_machine_ramp_start_callback_fires);
  RUN_TEST(test_ramp_scheduler_no_step_before_interval);
  RUN_TEST(test_ramp_scheduler_fires_step_at_interval);
  RUN_TEST(test_ramp_scheduler_completes_after_max_steps);
  RUN_TEST(test_ramp_scheduler_stop_cancels);
  return UNITY_END();
}
