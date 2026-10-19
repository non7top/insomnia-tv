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

// ---------------------------------------------------------------------------
// Config hot-reload setters (#66)
// ---------------------------------------------------------------------------
void test_state_machine_timeout_change_applies() {
  MockClock clk;
  SleepStateMachine sm(clk, /*inactivityTimeoutMs=*/60000);
  clk.advanceMs(10000);
  sm.setInactivityTimeoutMs(5000);
  sm.tick();
  TEST_ASSERT_EQUAL(State::RAMPING, sm.getCurrentState());
}

void test_ramp_scheduler_config_change_applies() {
  MockClock clk;
  int steps = 0;
  int completed = 0;
  RampScheduler rs(
      clk, /*interval_ms=*/1000, /*max_steps=*/5, [&] { steps++; },
      [&] { completed++; });
  rs.start();
  rs.setIntervalMs(100);
  rs.setMaxSteps(2);
  clk.advanceMs(100);
  rs.tick();
  clk.advanceMs(100);
  rs.tick();
  TEST_ASSERT_EQUAL_INT(2, steps);
  TEST_ASSERT_EQUAL_INT(1, completed);
}

// ---------------------------------------------------------------------------
// Idle time reflects the sleep cycle, not just IR activity (#74)
// ---------------------------------------------------------------------------
void test_state_machine_ms_since_reset() {
  MockClock clk;
  SleepStateMachine sm(clk, /*inactivityTimeoutMs=*/5000);
  clk.advanceMs(3000);
  TEST_ASSERT_EQUAL_UINT32(3000, sm.msSinceReset());

  sm.onIrActivity();
  TEST_ASSERT_EQUAL_UINT32(0, sm.msSinceReset());
}

void test_state_machine_ms_since_reset_after_cycle() {
  MockClock clk;
  SleepStateMachine sm(clk, /*inactivityTimeoutMs=*/1000);
  clk.advanceMs(1000);
  sm.tick();  // RAMPING
  sm.onRampComplete();
  clk.advanceMs(500);
  sm.tick();  // VERIFYING -> POWERING_OFF
  sm.tick();  // POWERING_OFF -> MONITORING, idle clock restarts
  TEST_ASSERT_EQUAL(State::MONITORING, sm.getCurrentState());
  TEST_ASSERT_EQUAL_UINT32(0, sm.msSinceReset());
}

void test_tv_powered_on_restarts_the_idle_clock() {
  MockClock clk;
  SleepStateMachine sm(clk, /*inactivityTimeoutMs=*/5000);
  clk.advanceMs(4000);
  sm.onTvPoweredOn();
  clk.advanceMs(4000);
  sm.tick();
  TEST_ASSERT_EQUAL(State::MONITORING, sm.getCurrentState());
  clk.advanceMs(1000);
  sm.tick();
  TEST_ASSERT_EQUAL(State::RAMPING, sm.getCurrentState());
}

void test_tv_powered_on_does_not_cancel_a_ramp() {
  MockClock clk;
  SleepStateMachine sm(clk, /*inactivityTimeoutMs=*/1000);
  clk.advanceMs(1000);
  sm.tick();
  TEST_ASSERT_EQUAL(State::RAMPING, sm.getCurrentState());
  sm.onTvPoweredOn();
  TEST_ASSERT_EQUAL(State::RAMPING, sm.getCurrentState());
}

void test_activity_during_ramping_cancels_the_ramp_callback() {
  MockClock clk;
  SleepStateMachine sm(clk, /*inactivityTimeoutMs=*/1000);
  int cancels = 0;
  sm.setRampCancelCallback([&] { cancels++; });
  clk.advanceMs(1000);
  sm.tick();
  TEST_ASSERT_EQUAL(State::RAMPING, sm.getCurrentState());

  sm.onIrActivity();
  TEST_ASSERT_EQUAL_INT(1, cancels);
  TEST_ASSERT_EQUAL(State::MONITORING, sm.getCurrentState());
}

void test_activity_outside_ramping_does_not_cancel() {
  MockClock clk;
  SleepStateMachine sm(clk, /*inactivityTimeoutMs=*/1000);
  int cancels = 0;
  sm.setRampCancelCallback([&] { cancels++; });
  sm.onIrActivity();
  TEST_ASSERT_EQUAL_INT(0, cancels);
}

void test_finished_cycle_does_not_fire_the_cancel_callback() {
  MockClock clk;
  SleepStateMachine sm(clk, /*inactivityTimeoutMs=*/1000);
  int cancels = 0;
  sm.setRampCancelCallback([&] { cancels++; });
  clk.advanceMs(1000);
  sm.tick();  // RAMPING
  sm.onRampComplete();
  sm.tick();  // VERIFYING -> power-off -> MONITORING
  TEST_ASSERT_EQUAL(State::MONITORING, sm.getCurrentState());
  TEST_ASSERT_EQUAL_INT(0, cancels);
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
  RUN_TEST(test_activity_during_ramping_cancels_the_ramp_callback);
  RUN_TEST(test_activity_outside_ramping_does_not_cancel);
  RUN_TEST(test_finished_cycle_does_not_fire_the_cancel_callback);
  RUN_TEST(test_tv_powered_on_restarts_the_idle_clock);
  RUN_TEST(test_tv_powered_on_does_not_cancel_a_ramp);
  RUN_TEST(test_state_machine_timeout_change_applies);
  RUN_TEST(test_ramp_scheduler_config_change_applies);
  RUN_TEST(test_state_machine_ms_since_reset);
  RUN_TEST(test_state_machine_ms_since_reset_after_cycle);
  return UNITY_END();
}
