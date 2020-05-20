# funcubeLib
decodes/encodes data from/for the FUNcube family of satellites, detects and tracks them too.

## pre-requisites
Current (2020) stable Raspbian or Debian (10.4).

## building
On target (RPi, x86 Debian), you will need GNU/make, GCC & G++ plus local dependencies. Note
that there _may_ be a collision with an existing `libjack-jackd2-0`, which can be avoided by
swapping it out for the earlier `libjack0` package first, typically:

```bash
apt-get remove libportaudio2 libjack-jackd2-0
apt-get install libjack0
apt-get install build-essential libfftw3-dev libusb-1.0-0-dev portaudio19-dev libasound2-dev
```

This done, your should be able to build directly using the supplied `Makefile` with a simple
`make` or `make clean`. This will produce the static library `bin/libfuncube.a`. Other targets
will build a dynamic library and install or uninstall to `/usr/local/[include|bin]` if you
execute as root.

## using the library
Start with the header `funcubeLib.h`, scoll down to the exported functions list and you will
see they are in 4 sections:

 * Library specific:
   * version checks of the library and the FFTW dependency.
   * registering a callback for log messages, so you can direct them whereever you need to.

 * FUNcube Dongle specific:
   * existence check, can be called regularly to detect hotplugging..
   * initialize and shutdown, wrap around multiple calls to establish a 'session' with a
     connected dongle.
   * get current tuning frequency in Hz.
   * set tuning frequency in Hz.
   * enable/disable bias T (DC output) signal on RF connector.

 * FUNcube decoding specific:
   * initialize and shutdown, wrap around a decode session, manages resources such as
     memory buffers and threads.
   * enumerate audio devices through a callback function.
   * start a decode session with named audio devices, in mono/stereo (IQ input) and
     indicate if the input device is a dongle.
   * start a decode session with indexed audio devices as above.
   * stop the decode session (disconnects audio devices).
   * set a manual override tuning centre frequency, within the audio passband, in Hz.
   * set the auto-tuning frequency range, within the audio passband, in Hz.
   * collect current FFT power spectral density (PSD) values. This will scale the whole
     tuning range to the supplied buffer length.
   * collect a section of the current FFT PSD values. This will scale requested section
     to the supplied buffer length.
   * register a callback to receive decoded data blocks.
   * enable/disable pass-through audio from tuner to output device for monitoring.
   * enable/disable DC offset removal from tuner output.
   * reload list of available audio devices, use before enumerating again.
   * check if decode session is started.
   * set auto-tuning frequency tracking flags (see below).
   * set signal peak detection parameters for tracking multiple satellites (see below).
   * set the number of decoders (workers), thus maximum number of tracked satellites.
   * get the current decoder tuning (peak) frequencies (<= number of workers).
   * get the current decoder availability (recency) (<= number of workers).
   * exclude specific frequencies from tracking (noisy neighbour rejection).
   * register a callback to be notified when data is available.
   * get the last decoded data block, with received frequency and error count.
   * get a text description of the last recorded error from decoding.

 * AO40 encoding and baseband modulation:
   * initialize and shutdown, wrap around an encode session, manages resources.
   * check if audio samples are available.
   * check if no more audio samples are available (done).
   * collect a block of audio samples (mono).
   * queue a block of data (256 bytes) for encoding.

## auto-tuning / peak tracking
Decoding operates a number of decoders simultaneously, tracking multiple satellites.
Behaviour is controlled through the auto-tuning flags, peak detection parameters and
frequency range settings. A manual override can also be applied which disables the
auto-tuning / tracking in favour of a single decoder under manual control.

An FFT is calculated across the baseband range (typically 96kHz or 192kHz) to provide
callers with real-time spectral power density. This is used by the standard FUNcube
dashboard to display a tuning window. The auto-tuning uses the FFT to find peaks in
signal strength that are a likely FUNcube satellite 'shape', and to assign decoders
to these frequencies. As decoders successfully receive data, they are marked as
'recent', and track their peak until they fail to decode for a number of cycles, at
which time their 'recency' is lowered and they are re-usable for another frequency.

Auto-tuning frequency range constrains the system within a user-defined baseband window.

Flags are a combination of `FFTRangeLimit` and `FFTTunerState` enumerations that are
intended to control the step size of auto-tuning, and to turn auto-tuning on/off or
to lock the current frequency. At present step size is not used, and locking is not
possible, hence flags may only turn auto-tuning on/off.

Peak detection parameters are `maxCalc` which limits the number of decoders that are
auto-tuned per detection cycle (default 2) and `delaySecs` which sets the detection
cycle time (default 1.0). This can be used to limit the CPU load on smaller devices.

Finally, the frequency of each decoder is readable, as is the 'recency' (actually
the inverse, availability) score, allowing visual indication (perhaps through colour
changes) of auto-tuning / peak tracking behaviour.

