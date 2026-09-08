#include "network/packets.h"

extern "C" {
  extern void simitl_init(const StateInit& state);
  extern void simitl_reinit_physics(const StateInit& state);
  extern void simitl_update(const StateInput& state);
  extern StateOutput simitl_get_state();
  extern void simitl_set_air_density(float rho);
  extern float simitl_get_air_density();
  extern void simitl_command(const CommandType cmd);
  extern void simitl_stop();
}