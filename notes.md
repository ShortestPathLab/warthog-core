# Search

- Merge ostream operators into single file for faster compile.
- Update util::timer to std::chrono
- scenario-manager.cpp: why global variables?
- is some mess with domain::xy_graph and euclidean_heuristic, euclidean_heuristic takes xy_graph but xy_graph is typedef of a template class, and xy_graph has a function that uses euclidean_heuristic, basically breaking the template class
- overall, xy_graph is a mess. Also, don't like euclidean_heuristic explicit attached to a graph, I do not associate it with graph nodes, but Cartesian coordinates in Euclidean environment
- xy_graph needs some heavy thought, is not general enough for a domain
- removed graph and xy_graph and euclidean_heuristic and road_heuristic for now
- removed generate experiments
