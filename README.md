# Kroki/glxoffload

Kroki/glxoffload is a wrapper around system libGL that redirects
OpenGL/GLX rendering to discrete video accelerator driven by
[Bumblebee] (https://github.com/Bumblebee-Project/Bumblebee) found in
integrated/discrete video card setups like NVIDIA Optimus.  It is
similar to [Primus] (https://github.com/amonakov/primus), but has an
independent code base with a number of improvements:

* Completely transparent to OpenGL calls.  No commit to a particular
  function list is required, all extensions exported by the
  accelerated libGL are immediately available.
* Thread-safe: no additional threads are created, rendering is done
  synchronously in the calling thread.
* Slightly faster (higher FPS rate) than Primus (while also consuming
  less CPU), which itself is a lot faster than [VirtualGL]
  (http://www.virtualgl.org/) (to be fair it should be noted that
  VirtualGL has different design goals; neither Primus nor
  Kroki/glxoffload support connection to X server over the network).
* Significantly faster (FPS-wise) than Primus when drawing to several
  windows/screens from a single thread (several calls to
  `glXMakeCurrent()` per frame do not cause re-init and pipeline
  reset).
* Supports `glXUseXFont()` out of the box.


## Installation

Assuming Bumblebee is already [installed and operational]
(http://duxyng.wordpress.com/2012/01/26/finally-working-nvidia-optimus-on-fedora/)
the following packages have to be installed:

* Kroki/error
  1. `git clone git://github.com/kroki/error.git`
  2. `cd error`
  3. `cmake .`
  4. `sudo make install`
  5. `cd ..`
* Kroki/likely
  1. `git clone git://github.com/kroki/likely.git`
  2. `cd likely`
  3. `cmake .`
  4. `sudo make install`
  5. `cd ..`
* Cuckoo-hash:
  1. `git clone git://github.com/kroki/Cuckoo-hash.git`
  2. `cd Cuckoo-hash`
  3. `./bootstrap.sh`
  4. `./configure`
  5. `make`
  6. `sudo make install`
  7. `cd ..`
* Kroki/glxoffload
  1. `git clone git://github.com/kroki/glxoffload.git`
  2. `cd glxoffload`
  3. `./bootstrap.sh`
  4. `./configure`
  5. `make`
  6. `sudo make install`
  7. `cd ..`


## Running

    $ kroki-glxoffload PROGRAM

(requires `socat` utility and a running `bumblebeed` instance).


## Selecting pixel copy method

Prior to version 0.7 kroki/glxoffload copied pixel buffers in RGB
format instead of conventional BGRA format because this produced a
slightly higher FPS rate with high resolution on my hardware (Intel
Core i7-2670QM @2.2GHz, NVIDIA GeForce GT 555M/PCIe/SSE2, screen
1920x1080; apparently copying 25% less of data with high resolution
beats the advantage of SIMD-enabled `memcpy()`).  However with small
resolutions (like default 300x300 for glxgears) performance was poor
(about 40% lower FPS rate than with BGRA).  Starting with version 0.7
`kroki-glxoffload` wrapper implements `--copy-method` command line
option.  You may select between `RGB` and `BGRA`, or use `AUTO` (which
is the default).  In the latter case the library will periodically
re-evaluate performance of both methods and dynamically switch to a
faster one.  Note however that automatic method selection is not
perfect: if CPU uses on-demand frequency scaling than a less efficient
method may trigger higher CPU speed and thus will appear to perform
better during such re-evaluation, though frequency switching latencies
and other CPU stalls will still result in a lower FPS rate on a long
run.  On the other hand, unless you are doing benchmarking and disable
sync-to-vblank (with `vblank_mode=0` environment variable) the default
`--copy-method=AUTO` should be an optimal setting in all cases.
`--verbose` will output a message on every copy method switch.  For
benchmarking you may disable CPU frequency scaling with

    $ sudo sh -c '
        for f in echo /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
          echo performance > "$f";
        done'


## Still frames and resize

Kroki/glxoffload checks for window resize every 16 frames.  Resize is
smooth when FPS rate is sufficiently high.  However when application
shows a single frame for a prolonged time (for instance [FlightGear
Flight Simulator] (http://www.flightgear.org/) shows a still
introduction image frame during startup for several seconds) enlarging
a window during that time will result in a resized image still being
showed through an original small viewport.  Viewport geometry will be
updated on the next 16th frame.  This can be fixed but probably not
worth it as kroki/glxoffload targets high FPS rate applications.
