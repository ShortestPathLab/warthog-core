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

Warthog dependencies are setup using git-submodules, use commands below are used to initialise and update respectively:
    git submodule init
    git submodule update

All dependencies should be placed in `/extern`.
If not project forked, we suggest setting the project up as below:

1. Copy `/cmake/submodule.cmake` to your repo
2. Submodule or subtree all your warthog dependencies and their dependencies into `/extern`
3. In `/CMakeLists.txt`, setup as below:
```
include(cmake/submodule.cmake)
warthog_submodule(warthog-[module]) # repeat for each [module]
```

With (2) we flatten dependencies to top-level project.
With (3) `warthog_submodule` will add `add_subdirectoy` if your CMake is the top level config.

An informal project not designed to be a dependency does not require this, and can just `add_subdirectory` to all dependencies.

# Resources

- [Moving AI Lab](https://movingai.com/): pathfinding benchmark and tools
- [Posthoc](https://posthoc-app.pathfinding.ai/): visualiser and debugger for pathfinding
- [Pathfinding Benchmarks](https://benchmarks.pathfinding.ai/): git repo for benchmarks
