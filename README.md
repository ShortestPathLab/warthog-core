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
This can be setup with either a submodule (can't be modified unless forked submodule)
or subtree.

Helper functions from `/cmake/warthog.cmake`, calling `warthog_submodule(warthog-core)` will
add `warthog-core` using the command `add_subdirectory(extern/warthog-core EXCLUDE_FROM_ALL)`,
but only if called by the root project (can be overridden with `WARTHOG_SUBMODULE_ROOT_ONLY`).
This means that all warthog dependencies should be added by the root project using the
`warthog_submodule`, and placed in `/extern/` using either submodules or subtrees.

## CMake

Basic repo example from creation is below:

    git remote add warthog-core https://github.com/ShortestPathLab/warthog-core.git
    git fetch warthog-core
    git checkout warthog-core/main cmake/warthog.cmake

Use this base CMake file `CMakeLists.txt`

```
cmake_minimum_required(VERSION 3.13)

project(App
	VERSION 0.0.1
	LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# warthog submodules
warthog_submodule(warthog-core)

add_executable(app main.cpp)
target_link_libraries(app PUBLIC warthog::core)
```

## Submodule

Submodule dependencies are useful for avoiding diamond dependencies,
i.e. ensuring there is only a single copy of warthog-core in use.

Commands for adding submodules for each repo are found below:

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

# Resources

- [Moving AI Lab](https://movingai.com/): pathfinding benchmark and tools
- [Posthoc](https://posthoc-app.pathfinding.ai/): visualiser and debugger for pathfinding
- [Pathfinding Benchmarks](https://benchmarks.pathfinding.ai/): git repo for benchmarks
