# libminiaudio

Echo does not come with a speaker. libminiaudio is the high-level miniaudio engine, vendored and compiled into your module, so `epm add` is the whole install.

If you know `ma_engine_init` and `ma_sound_start`, you know `ma::Engine::create` and `$sound->start()`. miniaudio's own docs still own the ideas: frames versus samples, spatialization, streaming. The catch is the boundary. Echo never sees a miniaudio struct. Everything with identity is a heap pointer behind `ma::Engine`, `ma::Sound`, `ma::SoundGroup`, and `ma::PcmStream`. A `Sound` is a file, encoded bytes, or a tone.

## Install

From your project directory:

```bash
epm add echolang/libminiaudio --git https://github.com/echolang/libminiaudio --range ^0.1
```

That writes a `#[requires:]` line and vendors the sources. Echo sees the `ma` namespace as soon as the module loads.

The module builds on Darwin, iOS, Linux, and Windows. On Darwin it links CoreAudio. On iOS the implementation is compiled as Objective-C (AVAudioSession) and the engine opens a Playback session: no microphone permission. On Linux it needs `-ldl -lpthread -lm`, which travel with the module. There is no system miniaudio to install.

## Play a file

Sometimes you just want a file to come out of the speakers:

```echo
ma::Engine $engine = guard ma::Engine::create() else ($e) {
    die("audio: {$e}");
}

ma::Sound $shot = guard $engine->sound('assets/shot.wav') else ($e) {
    die("load: {$e}");
}

guard $shot->start() else ($e) {
    die("start: {$e}");
}
```

`create` opens the default playback device and starts it. `sound` loads a file. WAV, FLAC, and MP3 are built in. Vorbis and Opus are not, yet.

A sound is not playing just because you loaded it. `start` is a separate call.

The process will exit and take the engine with it. Wait until the sound is done:

```echo
use std::time;

while ($shot->isPlaying()) {
    time::sleep(.millis(20));
}
```

A slightly longer version of that lives in `examples/`. Pass the file you want to hear:

```bash
cd examples
echoc run --target play -- resources/click.wav
```

If you want the device closed until you say so, pass `ma::EngineConfig($noAutoStart: true)` and call `$engine->start()` yourself.

### Errors

A call that can fail returns `result<T, ma::Error>` when there is a value, `status<ma::Error>` when there is not. `guard` is how you unwrap it. `"{$e}"` prints miniaudio's own sentence for the code.

Named cases cover the codes you will actually branch on: `doesNotExist`, `invalidFile`, `noDevice`, and the rest. Anything newer lands in `other`, and `$e->value()` is still the integer C handed us, so a miniaudio upgrade cannot break your `match`. I want that more than I want a perfect 1:1 with every `ma_result` miniaudio has ever shipped.

This will not compile, and that is the point:

```echo
if (!ma::Engine::create()) {
    // will not compile: create() is a result, not a bool
}
```

### Fire and forget

Occasionally you do not need a `Sound` handle at all. `play` is the engine recycling a one-shot for you:

```echo
guard $engine->play('assets/click.wav') else ($e) {
    die("{$e}");
}
```

You cannot pause it, seek it, or change its volume afterwards. Reach for `sound` when you want any of that.

`play` has no group argument. A fire-and-forget sound would outlive an Echo group handle. Attach with `sound(..., group:)` instead.

### Encoded bytes already in memory

A packed asset, a download, `io::readfile`. Same `Sound` as a path. The C layer copies, so the string can die.

```echo
use std::io;

string $bytes = guard io::readfile('assets/shot.wav') else ($e) {
    die("{$e->message()}");
}

ma::Sound $shot = guard $engine->sound(memory: $bytes) else ($e) {
    die("load: {$e}");
}
```

`$decode` still pre-decodes. `$stream` does nothing: there is no disk. Empty bytes are `invalidArgs`.

## Play a tone

A file is someone else's samples. A tone is a looping oscillator the engine generates: sine, square, triangle, saw. You get a `Sound` back. Same start, volume, pan, fade. Frequency is Hz.

```echo
ma::Sound $a = guard $engine->tone(ma::Waveform::sine, 440.0) else ($e) {
    die("{$e}");
}

guard $a->start() else ($e) {
    die("start: {$e}");
}

guard $a->setFrequency(523.25) else ($e) {
    die("{$e}");
}

$a->setFade(1.0f, 0.0f, 16800); // ~350 ms at 48 kHz
```

`setFade` is a multiplier on volume, not a replacement for it. Fade from 0 to 1 to bring a note in; pass `-1` as the start to mean "whatever it is right now." If volume is 0, fading does not make a sound.

`setFrequency`, `setWaveform`, and `setAmplitude` are for tones. Call them on a file or encoded bytes and you get `invalidOperation`. Amplitude is the generator's gain, not the sound's volume. Default is 1.0. `hz <= 0` is `invalidArgs`.

`stream` and `decode` on `SoundOptions` do nothing here: there is no file. The shim loops. `noPitch`, `noSpatialization`, and `group` still apply.

## SoundOptions

Sometimes you want a music track streamed from disk rather than decoded all at once. Say so with `SoundOptions`. The fields are bools with defaults of `false`. Name the ones you want:

```echo
ma::Sound $music = guard $engine->sound(
    'assets/theme.wav',
    ma::SoundOptions($stream: true, $looping: true)
) else ($e) {
    die("{$e}");
}

guard $music->start() else ($e) {
    die("start: {$e}");
}
```

`$decode` pre-decodes into memory so the audio thread does less work. That still applies when the bytes are already in memory. `$stream` is a file thing: encoded bytes have nowhere to stream from, so it is ignored there. `$noPitch` and `$noSpatialization` are optimizations when you know you will not use those features. Async loading is not here: miniaudio's async flag needs a fence, and this library does not wrap fences.

There are no `MA_SOUND_FLAG_*` integers on the public surface. A new option is a new field with a default of `false`. Existing call sites keep compiling.

## After it is loaded

Start it, then tune it:

```echo
$shot->setVolume(0.8f);
$shot->setPan(-0.25f);
$shot->setPitch(1.1f);
```

Those setters return `void`. The getters are `volume()`, `pan()`, and `pitch()`.

`stop` does not rewind. That is miniaudio's rule, not ours, and I am not wrapping a different one on top. `seek(0)` goes back to the start:

```echo
guard $shot->stop() else ($e) {
    die("{$e}");
}

guard $shot->seek(0) else ($e) {
    die("{$e}");
}
```

Looping is a load option and a live switch. Fades are in PCM frames, not milliseconds:

```echo
$shot->setLooping(true);
$shot->setFade(1.0f, 0.0f, 24000); // half a second at 48 kHz
```

`cursor()` and `length()` are PCM frames too, and they can fail:

```echo
uint64 $len = guard $shot->length() else ($e) {
    die("{$e}");
}

uint64 $pos = guard $shot->cursor() else ($e) {
    die("{$e}");
}
```

`isPlaying()` and `atEnd()` are bools. No `result` to unwrap.

The engine has a master volume of its own. Unlike the sound setters, this one can fail:

```echo
guard $engine->setVolume(0.5f) else ($e) {
    die("{$e}");
}
```

`volume()`, `channels()`, `sampleRate()`, and `time()` (global time in PCM frames) are queries.

## Groups

If you want SFX and music on separate buses, create a `SoundGroup` and attach sounds to it. The group's volume scales everything in it.

```echo
ma::SoundGroup $sfx = guard $engine->group() else ($e) {
    die("{$e}");
}

ma::Sound $shot = guard $engine->sound('assets/shot.wav', group: $sfx) else ($e) {
    die("{$e}");
}

$sfx->setVolume(0.7f);
```

## Spatialization

Sounds spatialize relative to listener 0 by default. The coordinate system is OpenGL's: +X right, +Y up, -Z forward.

```echo
$engine->setListenerPosition(ma::Vec3(0.0f, 0.0f, 0.0f));
$engine->setListenerDirection(ma::Vec3(0.0f, 0.0f, -1.0f));
$shot->setPosition(ma::Vec3(4.0f, 0.0f, 0.0f));
```

`ma::Vec3` is a three-float struct in this module. You do not need a math library to move a sound. `setListenerWorldUp` is there when the default +Y is the wrong up.

If a sound will never be spatialized, load it with `$noSpatialization: true`. You can also flip it later with `setSpatializationEnabled`.

`examples/spatial` walks a looping click around the listener on a circle:

```bash
cd examples
echoc run --target spatial
```

## Push PCM

A file is someone else's samples. If you want to generate them, you write interleaved f32 into a `PcmStream` and the engine reads it. Echo does not run on the audio thread. You fill the ring from your thread; the mixer is the reader. One writer. An empty ring is silence, not an error.

```echo
ma::PcmStream $out = guard $engine->pcm() else ($e) {
    die("{$e}");
}

guard $out->start() else ($e) {
    die("{$e}");
}

// interleaved f32, channels() samples per frame
array<float32> $frames = render(512);
guard $out->write($frames) else ($e) {
    die("{$e}");
}
```

`pcm()` defaults to 4096 frames, about 85 ms at 48 kHz. `write` returns how many frames actually landed. Short writes are normal when the ring is wrapping or nearly full. Length must be a multiple of `channels()`.

`PcmStream` is not a `Sound`. A `Sound` is a file, encoded bytes, or a tone. This type is the pipe, plus `start` / `stop` / `volume` / `pan`. Spatialization is off. Frequency is whatever you put in the buffer.

`examples/synth` is a keyboard on thirteen tones. Hold a key to sustain. Release starts a 350 ms fade (`setFade` on the `Sound`). Terminals that report key-up (Kitty, Ghostty, WezTerm, iTerm2, Windows) gate the note on the real key. The rest fade shortly after the last character.

```
  w e   t y u         sharps
a s d f g h j k       c major (a = c)

1-4  sine square triangle saw
z x  octave     - =  volume     [ ]  pan
spc  all off    q    quit
```

```bash
cd examples
echoc run --target synth
```

## No device

Tests, and anything that wants frames without a speaker, set `noDevice`:

```echo
ma::Engine $engine = guard ma::Engine::create(
    ma::EngineConfig($noDevice: true, $channels: 2, $sampleRate: 48000)
) else ($e) {
    die("{$e}");
}

ma::Sound $s = guard $engine->sound('tests/resources/sine.wav') else ($e) {
    die("{$e}");
}

guard $s->start() else ($e) {
    die("{$e}");
}

array<float32> $frames = guard $engine->read(1024) else ($e) {
    die("{$e}");
}
```

`start` / `stop` on that engine return `invalidOperation`. Pull frames with `read` instead. Interleaved f32, `channels()` samples per frame. 1024 frames of stereo is 2048 floats.

If you omit `$channels` or `$sampleRate` with `noDevice`, the shim fills in 2 and 48000.

## Ownership

A `Sound` keeps the `Engine` it was created from (and the group, if any). A `PcmStream` does the same. Dropping your own engine handle does not uninit the C engine while a sound or stream still holds it. When the last handle goes, the engine goes.

Classes are `#[atomic]` because miniaudio has an audio thread. The reference count is safe to share. The fields are not a lock. Do not poke the same sound from two threads without your own discipline. `PcmStream::write` is one writer. The mixer is already the other side of that ring.

## Naming

The C prefix comes off. The rest stays Echo:

| C | Echo |
|---|---|
| `ma_engine` | `ma::Engine` |
| `ma_sound_start` | `$sound->start()` |
| `ma_engine_set_volume` | `$engine->setVolume` |
| `ma_pcm_rb` | `ma::PcmStream` |
| `ma_waveform` | `ma::Waveform` / `$engine->tone` |
| `ma_result` | `ma::Error` |

Queries have no `get` prefix: `volume()`, `sampleRate()`, `isPlaying()`.

## What this is not

This is the high-level engine. I am not wrapping the rest of miniaudio "just in case." Device callbacks, capture, the node graph, arbitrary data-source vtables, async load fences, and the vorbis/opus extras are not here. Tones are, because a sine should not require a WAV file. A push PCM ring is, because that is the hole you need to generate audio from Echo. Echo never runs in the mixer. You write from your thread.

A new getter or setter is an `extern` alias of the miniaudio function, not another C wrapper. Init, uninit, option packing, the PCM ring, the waveform box on a tone, and in-memory encoded bytes are the exceptions: Echo cannot say those types.

miniaudio itself has no ABI stability, even between patch releases. Echo never sees a miniaudio struct. That is the point of the shim. The copy in `third_party/miniaudio` is 0.11.22.

## License

MIT. miniaudio is MIT too, vendored under `third_party/miniaudio`.
