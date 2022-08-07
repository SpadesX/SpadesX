<img src="docs/SpadesX-Logo.png">

# An Ace of Spades server
SpadesX is a C implementation for an Ace of Spades server that supports the protocol 0.75. C was chosen because of it's speed and the need for low latency in FPS games when modding the server as well as when scaling up.

## Why "SpadesX"?
We just one day decided to brainstorm the name and ended up on something that sounded cool yet still had the original naming.

## Installation

##### Libraries
Be sure to install the development versions of those:
* [zlib](https://github.com/madler/zlib)
* [json-c](https://github.com/json-c/json-c)
* [enet](https://github.com/lsalzman/enet)

##### Unix based systems
Ubuntu 20.04 doesnt have the latest version of json-c and since SpadesX uses some functions that are in later releases you either have to build it on your own or update to 21.04+

> git clone https://github.com/SpadesX/SpadesX

> cd SpadesX

> git submodule init && git submodule update

> mkdir build && cd build
 
> cmake ..
 
> make -jX (Replace X with number of threads you have on your system)

##### Windows
Users can use mingw. But you still have to install the libraries.

## Contributing
For a bug you must open an issue in this repository. Fully explaining your bug.
Beware tho that it may take days or even weeks before we fix the issue.
While we would love to fix issues all day (OK maybe not that actually sounds horrible) we are people too with our lives too.
Just like you we have jobs or other things we have to take care of. Please remember this before you start yelling at us why we cant fix your bug fast enough.

## Statistics
![Alt](https://repobeats.axiom.co/api/embed/e5cb9ca93a389a430b40229b39f01cfbab8b57ab.svg "Repobeats analytics image")

## Credits
Main Developer: Haxk20

Contributors: Check CONTRIBUTORS.md

Big thanks goes to amisometimes for the SpadesX logo.
