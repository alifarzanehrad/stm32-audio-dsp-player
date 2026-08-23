#include "arm_math.h"
#include "audio_echo.h"
#include "audio_reverb.h"
#include "noise_reduction.h"

#include <string.h>

void arm_biquad_cascade_df1_init_f32(
    arm_biquad_casd_df1_inst_f32 *instance,
    uint8_t numStages,
    const float32_t *coefficients,
    float32_t *state
)
{
    instance->numStages = numStages;
    instance->pCoeffs = coefficients;
    instance->pState = state;
    memset(state, 0, 4U * numStages * sizeof(float32_t));
}

void arm_biquad_cascade_df1_f32(
    const arm_biquad_casd_df1_inst_f32 *instance,
    const float32_t *source,
    float32_t *destination,
    uint32_t blockSize
)
{
    for (uint32_t stage = 0U; stage < instance->numStages; stage++)
    {
        const float32_t *c = &instance->pCoeffs[5U * stage];
        float32_t *s = &instance->pState[4U * stage];

        for (uint32_t i = 0U; i < blockSize; i++)
        {
            const float32_t input =
                (stage == 0U) ? source[i] : destination[i];

            const float32_t output =
                c[0] * input + c[1] * s[0] + c[2] * s[1] +
                c[3] * s[2] + c[4] * s[3];

            s[1] = s[0];
            s[0] = input;
            s[3] = s[2];
            s[2] = output;
            destination[i] = output;
        }
    }
}

/* Host-test substitutes for DSP stages not exercised by these tests. */
void AudioEcho_Init(float32_t sampleRate) { (void)sampleRate; }
void AudioEcho_Process(float32_t *left, float32_t *right, uint32_t count)
{
    (void)left;
    (void)right;
    (void)count;
}
void AudioEcho_SetEnabled(uint8_t enabled) { (void)enabled; }
uint8_t AudioEcho_IsEnabled(void) { return 0U; }

void AudioReverb_Init(float32_t sampleRate) { (void)sampleRate; }
void AudioReverb_Process(float32_t *left, float32_t *right, uint32_t count)
{
    (void)left;
    (void)right;
    (void)count;
}
void AudioReverb_SetEnabled(uint8_t enabled) { (void)enabled; }
uint8_t AudioReverb_IsEnabled(void) { return 0U; }

void AudioNoiseReduction_Init(void) {}
void AudioNoiseReduction_Process(
    float32_t *left,
    float32_t *right,
    uint32_t count
)
{
    (void)left;
    (void)right;
    (void)count;
}
void AudioNoiseReduction_SetEnabled(uint8_t enabled) { (void)enabled; }
uint8_t AudioNoiseReduction_IsEnabled(void) { return 0U; }
