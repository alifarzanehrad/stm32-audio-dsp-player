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
| Test duration | 48.245 s |
| Processed blocks | 2242 |
| Stability test | UI navigation, EQ changes, effect toggles, volume and track controls |

The reported pipeline time includes the real-time audio processing path.
Spectrum timing is reported separately because it runs as an analysis branch
every second block.

## DSP timing results

| Stage | CPU load | Average | Maximum | Calls |
|---|---:|---:|---:|---:|
| Complete pipeline | 51.1% | 11,006 us | 11,059 us | 2242 |
| PCM16 to float | 0.7% | 156 us | 169 us | 2242 |
| Noise reduction | 21.9% | 4,731 us | 4,745 us | 2242 |
| Equalizer | 7.6% | 1,652 us | 1,668 us | 2242 |
| Echo | 1.6% | 361 us | 368 us | 2242 |
| Reverb | 17.2% | 3,703 us | 3,737 us | 2242 |
| Limiter | 0.8% | 175 us | 188 us | 2242 |
| Float to PCM16 | 1.0% | 220 us | 229 us | 2242 |
| Spectrum FFT | 2.1% | 941 us | 953 us | 1120 |

### Real-time margin

A DMA half-buffer contains 1024 frames, therefore its processing deadline is:

```text
1024 / 48000 = 21.333 ms
```

Using the conservative sum of the maximum pipeline and maximum spectrum times:

```text
11.059 ms + 0.953 ms = 12.012 ms
21.333 ms - 12.012 ms = 9.321 ms
```

The worst measured case therefore retains approximately **9.32 ms**, or
**43.7%**, of the audio deadline. No deadline misses were observed.

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
| Worst pipeline + spectrum | Below 21.33 ms | **Pass: 12.012 ms** |
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
2. Set `AUDIO_BENCHMARK_ENABLED` to `1U` in
   `Core/Inc/audio_benchmark.h`.
3. Build and flash the firmware.
4. Open the UART at 115200 baud.
5. Enable noise reduction, echo, and reverb; exercise EQ, navigation, volume,
   and track controls.
6. Leave playback undisturbed for at least 30 seconds.
7. Pause playback to print the benchmark and runtime diagnostics.

Benchmark instrumentation is disabled in the normal release configuration to
remove measurement and UART overhead. Re-enable it only for profiling.

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
