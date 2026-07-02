# Warthog Library

Warthog is an optimised C++ library for pathfinding search.
It is developed and maintained by Daniel Harabor and contributors in the Shortest Path Lab.
Github houses the newer version, the legacy version is available at: https://bitbucket.org/dharabor/pathfinding/

## Layout

Warthog is split in several repos, all official repos will be located within [Shortest Path Lab](https://github.com/ShortestPathLab).

### Core

Repo [warthog-core](https://github.com/ShortestPathLab/warthog-core) houses the core library of the project.
Any warthog library will require this dependency.
It contains domain, search, heuristic, scenario loaders and other core utilities.
See `/apps` for pre-setup application that can run standard scenarios.

### JPS

Repo [warthog-jps](https://github.com/ShortestPathLab/warthog-jps) houses jps implementation.
Requires `warthog-core` as a dependency.

# Using warthog

It is recommended to use warthog not as a fork, but included in an external repo.
This setup support FetchContent, git submodule and git subtree.

## CMake

Setup a basic project using the following the commands:

    git init
    git remote add warthog-core https://github.com/ShortestPathLab/warthog-core.git
    git fetch warthog-core
    git checkout warthog-core/main cmake/warthog.cmake

Example `CMakeLists.txt`:

```
cmake_minimum_required(VERSION 3.13)

project(App
	VERSION 0.0.1
	LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# warthog modules
include(cmake/warthog.cmake)

warthog_module_declare(warthog-core v0.5.0) # is optional, remove for default of main
warthog_module(warthog-core)

add_executable(app main.cpp)
target_link_libraries(app PUBLIC warthog::core)
```

## Submodule

Commands for adding a module as a submodules for each repo are found below:

    git submodule add https://github.com/ShortestPathLab/warthog-core.git extern/warthog-core
    git submodule add https://github.com/ShortestPathLab/warthog-jps.git extern/warthog-jps

To update the version of warthog, for warthog module `$module`:

    cd extern/$module
    git fetch
    git checkout|git switch
    cd ..
    git add $module

This will update the submodule to the checkout commit.
Initialise or update the submodule on other clones with
the following commands:

    git submodule init # after clone
    git submodule update # after pull

## Subtree

Subtree will make the module a part of your repo, allowing
for local editing without forking.

The setup for each module:

    git subtree -P extern/warthog-core add https://github.com/ShortestPathLab/warthog-core.git main|branch|commit --squash
    git subtree -P extern/warthog-jps add https://github.com/ShortestPathLab/warthog-jps.git main|branch|commit --squash

The update commands:

    git subtree -P extern/warthog-core pull https://github.com/ShortestPathLab/warthog-core.git main|branch|commit --squash
    git subtree -P extern/warthog-jps pull https://github.com/ShortestPathLab/warthog-jps.git main|branch|commit --squash

## Advance Module Details

File `/cmake/warthog.cmake` from warthog core should be copied to user repo and `include` in CMake.
Calling `warthog_submodule(warthog-core)` will then add `warthog-core` to your CMake in the following order:
1. `add_subdirectory(/extern/warthog-core)` if `/extern/warthog-core/CMakeLists.txt` exists (submodule/subtree)
2. `FetchContent_Declare` then `FetchContent_MakeAvailable(warthog-core)` otherwise
3. Error if cannot find `warthog-core` content

The `warthog_module` call only adds a module once, the following calls will be ignored.
The submodule/subtree version only works if called in the top level project by default;
if this method is preferred, then it should be added to the top level `/extern/`, can be overridden
with code `warthog_module(warthog-core ON)`.

Declare of warthog-core can be done using the following code:
```
warthog_module_declare(warthog-core [main|branch|tag|commit])
```
or:
```
FetchContent_Declare(warthog-core
	GIT_REPOSITORY https://github.com/ShortestPathLab/warthog-core.git
	GIT_TAG [main|branch|tag|commit])
```
This will declare what warthog-core version to fetched.
The `warthog_module_declare` version makes it simple, although it only supports known warthog libraries.
The optional second parameter sets the version to pull, by default is `main` branch.
This system only support warthog 0.5 or greater.

# The Warthog Application

## Compile

Invoke CMake build to compile.

Example release build commands, from project root:

```
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

To enable posthoc trace generation, it must be enabled through CMake (adds small overhead).
Example setting (and then rebuild):

```
cmake build -DWARTHOG_POSTHOC=On
cmake --build build -j
```

Many parameters are configurable (most in built library not application).
They all prefixed `WARTHOTG_`.

### Advanced Features

Library includes support for x86 intrinsics instruction support, which may
improve performance of some algorithms.
To enable, compiler must have these instructions enabled, and they must be
enabled in CMake, either `WARTHOG_INTRIN_ALL` for all or a specific
supported instruction set, more to be added when required.

Even if BMI2 may be supported by a CPU, the instructions may be implemented
by microcode which may reduce performance instead; e.g. Zen 3 arch has some
microcode, Zen 4 supports full BMI2.
User must determine manually if enabling is applicable.
Example below (works with gcc or clang, other systems may differ):
```
cmake build -DWARTHOG_INTRIN_ALL=On -DCMAKE_CXX_FLAGS="-march=native"
cmake --build build -j
```

## Run

Run application `build/warthog` from build commands.
See overview of commands with `build/warthog --help`.

### Command line options overview

`--help`
Set this parameter to print all available program options.

`--alg [name]`
Used to specify a named search algorithm.
Current supported: astar, astar_wgm, astar4c, dijkstra

`--scen [file]`
Used to specify a scenario file for experiments.
Support for v1 & v2 scenario format, see benchmark in resources.

`--map [file]`
Overrides map filename used in scenario.

`--cost [type]`
Scenario v2 files holds several costs for instances, specify these costs here.
If given `-`, then remove all costs (works for both v1 and v2).
Otherwise requires v2 scenario and cost to exist or error.
If `--cost` is not supplied, uses the first cost (if present) for v2.

`--grid-weight [file]`
For use with weighted terrain algorithm (e.g. astar_wgm).
Provide grid weights, provided `examples/grid.weight` for scenario grid
weights and `examples/terrain.weight` for MovingAI terrain maps, as
used in the jpsw paper.

`--checkopt`
Set this parameter to compare the length of each computed path against an
optimal length value specified by the scenario file at hand.

`--verbose`
Set this parameter to print debugging information (must be in debug config).

`--snapshot [id]`
Only run instances from snapshot `id`.
Scenario will be considered a static scenario.

`--filter [id|num]`
Only run instance `id` from scenario.
If used with `--snapshot`, then run instance number `num` in specified snapshot.
e.g. `--snapshot=10 --filter=0` will run the first instance on snapshot 10.
Scenario will be considered a static scenario.

`--trace [.trace.yaml file]`
Requires `WARTHOG_POSTHOC=On`.
Generate a trace file output for use with posthoc (see resources).
The trace is only for first instance, use with `--filter` to choose instance.

`--dump-map [file]`
Dump the gridmap at the beginning of search to `file`.
Use with `--filter` or `--snapshot` to choose the map to dump.
Useful in dynamic scenario to see current state of grid, especially if used with `--trace`.

## Examples

```
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DWARTHOG_POSTHOC=On && cmake --build build -j
./build/warthog --alg dijkstra --scen examples/arena2.map.scen --checkopt
./build/warthog --alg astar --scen examples/NovaStation_Berlin.scen --checkopt
./build/warthog --alg astar4c --scen examples/NovaStation_Berlin.scen --cost 4c --checkopt
./build/warthog --alg astar --scen examples/NovaStation_Berlin.scen --filter=201 --dump-map=test.map --trace=test.trace.yaml
./build/warthog --alg astar --scen examples/NovaStation_Berlin.scen --snapshot=40 --filter=1
./build/warthog --alg astar_wgm --scen examples/arena2.map.scen --grid-weight examples/grid.weight
```

# Resources

- [Moving AI Lab](https://movingai.com/): pathfinding benchmark and tools
- [Posthoc](https://posthoc-app.pathfinding.ai/): visualiser and debugger for pathfinding
- [Pathfinding Benchmarks](https://benchmarks.pathfinding.ai/): git repo for benchmarks
- [Dynamic Benchmarks](https://github.com/gppc-dev/benchmarks/): git repo for v2 scenarios
