#ifndef MODERNER_KRIEG_MOSUL_DEMO_H
#define MODERNER_KRIEG_MOSUL_DEMO_H

#include "mk_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MK_MOSUL_DEFAULT_SCENARIO_PATH "game/mosul/scenarios/market_commercial_streets_2003.mkscenario"

mk_result_t mk_mosul_load_scenario_file(
    const char *scenario_path,
    const char *project_root,
    mk_scenario_definition_t *out_scenario
);

mk_result_t mk_mosul_make_east_block_scenario(mk_scenario_definition_t *out_scenario);
mk_result_t mk_mosul_make_market_2003_scenario(mk_scenario_definition_t *out_scenario);
mk_result_t mk_mosul_make_market_2003_fixture_scenario(mk_scenario_definition_t *out_scenario);

#ifdef __cplusplus
}
#endif

#endif
