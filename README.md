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
