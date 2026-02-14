#ifndef SVF_H
#define SVF_H

#include "audio_config.h"
#include <math.h>

/* ZDF State Variable Filter (trapezoidal integration, lowpass output) */

typedef struct {
    float ic1eq;
    float ic2eq;
} svf_state_t;

typedef struct {
    float g, r, h;
} svf_coeff_t;

static inline svf_coeff_t svf_compute_coeff(float freq_norm)
{
    svf_coeff_t c;
    c.g = tanf(PI_F * freq_norm);
    c.r = 2.0f;              /* Q = 0.5 => r = 1/Q = 2 */
    c.h = 1.0f / (1.0f + c.r * c.g + c.g * c.g);
    return c;
}

static inline float svf_process(svf_state_t *state,
                                svf_coeff_t coeff,
                                float input)
{
    float hp = (input - coeff.r * state->ic1eq - coeff.g * state->ic1eq - state->ic2eq) * coeff.h;
    float v1 = coeff.g * hp;
    float bp = v1 + state->ic1eq;
    state->ic1eq = v1 + bp;
    float v2 = coeff.g * bp;
    float lp = v2 + state->ic2eq;
    state->ic2eq = v2 + lp;
    return lp;
}

#endif /* SVF_H */
