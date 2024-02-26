# SpadesX
[![Build Status](
https://github.com/SpadesX/SpadesX/actions/workflows/cmake.yml/badge.svg)
](https://github.com/SpadesX/SpadesX/actions/workflows/cmake.yml)

[![SpadesX Logo](docs/SpadesX-Logo-small.png)](https://spadesx.org)

SpadesX is a C implementation for an Ace of Spades server that supports the protocol 0.75.

C was chosen because of its speed and the need for low latency in FPS games when modding the server as well as when scaling up.

## Why "SpadesX"?
We just one day decided to brainstorm the name and ended up on something that sounded cool yet still had the original naming.

## Installation

##### Libraries
If you are using the pre-supplied Dockerfile (details below) please skip this step.

Otherwise, be sure to install the development versions of those:
* [zlib](https://github.com/madler/zlib)
* [json-c](https://github.com/json-c/json-c)
* [libreadline](https://tracker.debian.org/pkg/readline)
* [libbsd](https://tracker.debian.org/pkg/libbsd) (Only on some systems)

#### Docker

This method allows you to run SpadesX in a container which might be a more convenient and secure option for you. For your convenience, `Dockerfile` and `docker-compose.yml` are supplied for you in this repository.

Initial setup commands:

```bash
git clone https://github.com/SpadesX/SpadesX

cd SpadesX

git submodule update --init 

docker-compose build
```

Docker provides multiple methods to startup and shutdown the container, however the following may be useful for you.

> Note that some commands assume you are already in the folder which the Dockerfile is in (i.e., `SpadesX`).

- Startup container, attached: `docker-compose run spadesx`.
- Startup container, detached: `docker-compose up -d`
- Shutdown container (whilst in game or in console): `/shutdown` command
- Shutdown container (whilst detached): `docker-compose down`

To update your server to the latest commit (please shutdown your server first):
```bash
git pull
git submodule update --init  
docker-compose build
```

##### Unix based systems
Ubuntu 20.04 doesnt have the latest version of json-c and since SpadesX uses some functions that are in later releases you either have to build it on your own or update to 21.04+

```bash
git clone https://github.com/SpadesX/SpadesX

cd SpadesX

mkdir build && cd build

cmake ..

make -jX #(Replace X with number of threads you have on your system)
```

##### Windows
You can use mingw, but you'll still have to install the libraries first.

## Contribute
If you would like to contribute bug fixes, improvements, and new features please take a look at our [Contributor Guide](CONTRIBUTING.md) to see how you can participate in this open source project.

You can also contact us via the official [Discord server][discord].

## Statistics
![Alt](https://repobeats.axiom.co/api/embed/e5cb9ca93a389a430b40229b39f01cfbab8b57ab.svg "Repobeats analytics image")

## Credits
Main Developer: Haxk20

Special thanks to [amisometimes](https://amisometimes.com/) for the SpadesX logo.

Check the [Contributor List](CONTRIBUTORS.md) for a list of the people that helped make this project.

## License
[GNU General Public License v3.0](LICENSE)

[discord]: https://discord.gg/dsRjTzJpZC
[build]: https://github.com/SpadesX/SpadesX/actions/workflows/cmake.yml
