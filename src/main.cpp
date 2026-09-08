#include "sim/sim.h"
#include "network/packets.h"
#include <fmt/format.h>
#include <stdlib.h>
#include <stdio.h>

SimITL::Sim* sim = nullptr;

// interface for c lib
extern "C" {
  void simitl_init(const StateInit& state){
    sim = &SimITL::Sim::getInstance();
    sim->init(state);
  }

  void simitl_reinit_physics(const StateInit& state){
    sim->reinitPhysics(state);
  }

  void simitl_update(const StateInput& state){
    sim->update(state);
  }

  StateOutput simitl_get_state(){
    return sim->getStateUpdate();
  }

  // Same as simitl_get_state, but writes through a pointer. Struct-by-value returns
  // are the fragile part of P/Invoke on arm64; game clients should prefer this.
  void simitl_get_state_into(StateOutput* out){
    if(out){ *out = sim->getStateUpdate(); }
  }

  // Layout check for foreign-language bindings: 0 = StateInit, 1 = StateInput, 2 = StateOutput.
  int32_t simitl_sizeof(int32_t which){
    switch(which){
      case 0: return static_cast<int32_t>(sizeof(StateInit));
      case 1: return static_cast<int32_t>(sizeof(StateInput));
      case 2: return static_cast<int32_t>(sizeof(StateOutput));
      default: return -1;
    }
  }

  void simitl_command(const CommandType cmd){
    sim->command(cmd);
  }

  void simitl_stop(){
    sim->stop();
  }
}
