#ifndef TEST_ARM_MATH_H
#define TEST_ARM_MATH_H

#include <stdint.h>

typedef float float32_t;

#ifndef PI
#define PI 3.14159265358979323846f
#endif

typedef struct
{
    uint32_t numStages;
    const float32_t *pCoeffs;
    float32_t *pState;
} arm_biquad_casd_df1_inst_f32;

void arm_biquad_cascade_df1_init_f32(
    arm_biquad_casd_df1_inst_f32 *instance,
    uint8_t numStages,
    const float32_t *coefficients,
    float32_t *state
);

void arm_biquad_cascade_df1_f32(
    const arm_biquad_casd_df1_inst_f32 *instance,
    const float32_t *source,
    float32_t *destination,
    uint32_t blockSize
);

#endif /* TEST_ARM_MATH_H */
