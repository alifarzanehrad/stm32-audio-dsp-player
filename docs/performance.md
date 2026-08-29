# Performance and Runtime Validation

This document records the on-board performance and memory measurements for the
STM32 Audio DSP Player. Measurements were collected on the STM32F746G-DISCO
while processing stereo PCM16 audio at 48 kHz.

## Test configuration

| Item | Configuration |
|---|---|
| MCU | STM32F746NG, Arm Cortex-M7 |
| Audio format | Stereo PCM16, 48 kHz |
| DMA half-buffer | 1024 stereo frames |
| Real-time deadline | 21.33 ms per half-buffer |
| Compiler optimization | `-Ofast` |
| Active DSP | Noise reduction, 5-band EQ, echo, reverb, limiter |
| Spectrum | 1024-point FFT, evaluated every second audio block |
| Timing source | Cortex-M7 DWT cycle counter |
| Test duration | 27.445 s |
| Processed blocks | 1286 |
| Stability test | UI navigation, EQ changes, effect toggles, volume and track controls |

The reported pipeline time includes the real-time audio processing path.
Spectrum timing is reported separately because it runs as an analysis branch
every second block.

## DSP timing results

| Stage | CPU load | Average | Maximum | Calls |
|---|---:|---:|---:|---:|
| Complete pipeline | 60.4% | 12,892 us | 17,598 us | 1286 |
| PCM16 to float | 0.7% | 156 us | 180 us | 1286 |
| Noise reduction | 31.0% | 6,620 us | 6,664 us | 1286 |
| Equalizer | 7.7% | 1,652 us | 1,710 us | 1286 |
| Echo | 1.7% | 363 us | 3,584 us | 1286 |
| Reverb | 17.3% | 3,696 us | 5,122 us | 1286 |
| Limiter | 0.8% | 175 us | 184 us | 1286 |
| Float to PCM16 | 1.0% | 220 us | 229 us | 1286 |
| Spectrum FFT | 2.2% | 940 us | 953 us | 643 |

### Real-time margin

A DMA half-buffer contains 1024 frames, therefore its processing deadline is:

```text
1024 / 48000 = 21.333 ms
```

Using the conservative sum of the maximum pipeline and maximum spectrum times:

```text
17.598 ms + 0.953 ms = 18.551 ms
21.333 ms - 18.551 ms = 2.782 ms
```

The worst measured case therefore retains approximately **2.78 ms**, or
**13.0%**, of the audio deadline. No deadline misses were observed.

Noise reduction is the most expensive individual DSP stage, followed by the
Schroeder reverb. Spectrum decimation prevents the display analysis from
unnecessarily consuming CPU on every audio block.

## Flash and static RAM

Measured with:

```bash
arm-none-eabi-size Debug/V1.elf
```

```text
text      data      bss       dec       hex
369018    1136      92096     462250    70daa
```

| Resource | Used | Device capacity | Utilization |
|---|---:|---:|---:|
| Flash (`text + data`) | 370,154 bytes | 1,048,576 bytes | 35.3% |
| Static internal RAM (`data + bss`) | 93,232 bytes | 327,680 bytes | 28.5% |

Debug information stored in the ELF file is not programmed into MCU flash and
is not included in these utilization values.

## Runtime memory diagnostics

| Measurement | Minimum free space |
|---|---:|
| Audio task stack | 850 words / 3,400 bytes |
| TouchGFX task stack | 1,493 words / 5,972 bytes |
| FreeRTOS heap | 19,920 bytes |

The minimum-ever heap value matched the final free heap value. Stack
high-water marks remained above the acceptance limits, and no stack overflow
or allocation failure was observed.

## Acceptance criteria

| Check | Requirement | Result |
|---|---|---|
| Audio deadline misses | 0 | **Pass: 0** |
| Worst pipeline + spectrum | Below 21.33 ms | **Pass: 18.551 ms** |
| Audio stack reserve | At least 256 words | **Pass: 850 words** |
| TouchGFX stack reserve | At least 512 words | **Pass: 1493 words** |
| Minimum free heap | At least 8 KiB | **Pass: 19.45 KiB** |
| Playback/UI stability | No audible or functional failure | **Pass** |

During the final stress test, all effects were enabled, EQ values were changed,
screens were switched repeatedly, volume was adjusted, and tracks were
changed. No audible glitches, playback stalls, UI failures, or deadline misses
were observed.

## Reproducing the benchmark

1. Build with both C and C++ optimization set to `-Ofast`.
2. Build and flash the firmware.
3. Enable noise reduction, echo, and reverb and set the EQ bands as required.
4. Return to the Player screen so the display FFT remains active.
5. Leave playback undisturbed for at least 20 seconds.
6. Read live load, maximum DSP time, deadline misses, heap, and stack reserve
   from the Performance screen.

DWT instrumentation remains enabled because it supplies the on-device
Performance screen. Normal firmware does not print periodic benchmark data to
UART while audio is running.

## Measurements still planned

These measurements require external audio capture or power-test equipment and
are intentionally not estimated:

- EQ frequency-response curves
- Echo delay and decay verification
- Reverb impulse response and RT60
- Input/output SNR improvement at 5, 10, and 15 dB
- Speech-quality metrics such as STOI or SI-SDR
- THD+N, output noise floor, and clipping measurements
- End-to-end audio latency
- Board current and power consumption
