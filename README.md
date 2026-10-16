HEAD
Data Structures Final Project (C)

Five independent C programs. Each one is compiled and run on its own from its own folder.

Folder layout
Q1/  q1_event_logger.c
Q2/  q2_command_registry.c   commands.txt
Q3/  q3_dependency_graph.c
Q4/  q4_medical_routing.c
Q5/  q5_huffman.c            device_log.txt
Requirements
GCC (Linux, macOS, or MinGW/MSYS2 on Windows)
Run each program from inside its own folder so it can find its input file.
Q1: Embedded Device Event Logger (doubly linked list)
bash
cd Q1
gcc -Wall -Wextra -std=c99 -D_DEFAULT_SOURCE -o q1 q1_event_logger.c
./q1

Commands: n next, p previous, a add, c current, x clear all, q save and quit. Output file: event_session.txt. Capacity is 25 events (the oldest is freed when the 26th arrives).

Q2: Network Device Command Registry (binary search tree)
bash
cd Q2
gcc -Wall -Wextra -std=c99 -D_DEFAULT_SOURCE -o q2 q2_command_registry.c
./q2

Reads commands.txt at startup (one command per line, max 50). Commands: v verify, p print alphabetically, s count, x clear, q save and quit. Output files: audit.log (unknown attempts), registry_session.txt.

Q3: Server Dependency Network Analyzer (directed graph)
bash
cd Q3
gcc -Wall -Wextra -std=c99 -D_DEFAULT_SOURCE -o q3 q3_dependency_graph.c
./q3

Commands: a add server, r remove server, c create dependency, d delete dependency, q query a server, m adjacency matrix, x exit. Server names are case-insensitive and may contain spaces (e.g. API Server).

Q4: Emergency Medical Supply Routing (Dijkstra)
bash
cd Q4
gcc -Wall -Wextra -std=c99 -D_DEFAULT_SOURCE -o q4 q4_medical_routing.c
./q4

Menu: 1 display locations, 2 find shortest route, 3 display last route distance, 4 exit, 5 add road (extra, used to demonstrate negative-distance validation). Locations can be typed by name or by list number.

Q5: Firmware Log Compression (Huffman coding)
bash
cd Q5
gcc -Wall -Wextra -std=c99 -D_DEFAULT_SOURCE -o q5 q5_huffman.c
./q5

Needs device_log.txt in the same folder. Menu: 1 compress, 2 decompress (auto-verifies), 3 verify, 4 exit. Output files: device_log.huff, device_log_restored.txt.

Suggested order: choose 1, then 2, then 4.

Windows notes
Run with .\q1.exe (PowerShell/CMD) instead of ./q1 after compiling with -o q1.exe.
To compile all five at once (Linux/macOS/Git Bash):
bash
for n in 1 2 3 4 5; do (cd Q$n && gcc -Wall -Wextra -std=c99 -D_DEFAULT_SOURCE -o q$n q$n*.c); done
Suggested test cases
Q	Test
1	Press c, n, p on an empty list; add 27 events and check that only 25 remain; p at the oldest and n at the newest; x then c
2	Verify an exact command, a near match (SHOW_STATU) and an unknown command (FOO); empty commands.txt; duplicates; x then p; check audit.log
3	Query API Server; add a duplicate server; add a dependency with a missing server; remove Database Server and check the matrix
4	Central Warehouse to District Hospital (14 km direct); unknown location; same source and destination; add a road with distance -4
5	Normal log; missing file; empty file; log with one repeated character; confirm Verification: SUCCESS
=======
# Summative_ProjectDSA
a743a43a4e9cfd964ddbc0d5b1706f02aeb4ae07
