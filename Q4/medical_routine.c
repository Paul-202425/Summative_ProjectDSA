/* Q4: Emergency Medical Supply Routing - weighted graph + Dijkstra */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>

#define V 7
#define INF INT_MAX

static const char *loc[V] = {
    "Central Warehouse", "Distribution Center", "Health Center A",
    "Health Center B", "Health Center C", "District Hospital", "" };
static int nLoc = 6;
static int w[V][V];                 /* 0 = no road (weights must be > 0) */

/* last computed route, for menu option 3 */
static int lastPath[V], lastLen = 0, lastDist = -1, lastSrc = -1, lastDst = -1;

static int addRoad(int a, int b, int d) {
    if (a < 0 || b < 0 || a >= nLoc || b >= nLoc) { printf("Invalid location.\n"); return 0; }
    if (a == b) { printf("A road cannot connect a location to itself.\n"); return 0; }
    if (d <= 0) { printf("Invalid distance %d (must be positive).\n", d); return 0; }
    w[a][b] = w[b][a] = d;
    return 1;
}

static int sameName(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

/* accepts a name or a list number (1..nLoc) */
static int lookup(const char *s) {
    char *end;
    long v = strtol(s, &end, 10);
    if (end != s && *end == '\0') return (v >= 1 && v <= nLoc) ? (int)v - 1 : -1;
    for (int i = 0; i < nLoc; i++) if (sameName(loc[i], s)) return i;
    return -1;
}

static void readLine(const char *prompt, char *buf, size_t size) {
    printf("%s", prompt);
    if (!fgets(buf, (int)size, stdin)) { buf[0] = '\0'; return; }
    buf[strcspn(buf, "\r\n")] = '\0';
    size_t len = strlen(buf);
    while (len && isspace((unsigned char)buf[len - 1])) buf[--len] = '\0';
}

static void showLocations(void) {
    printf("\nLocations:\n");
    for (int i = 0; i < nLoc; i++) printf("  %d. %s\n", i + 1, loc[i]);
    printf("Roads (km):\n");
    for (int i = 0; i < nLoc; i++)
        for (int j = i + 1; j < nLoc; j++)
            if (w[i][j]) printf("  %s -- %d -- %s\n", loc[i], w[i][j], loc[j]);
}

static int dijkstra(int src, int dst, int path[], int *len) {
    int dist[V], prev[V], done[V];
    for (int i = 0; i < nLoc; i++) { dist[i] = INF; prev[i] = -1; done[i] = 0; }
    dist[src] = 0;
    for (int k = 0; k < nLoc; k++) {
        int u = -1;
        for (int i = 0; i < nLoc; i++)
            if (!done[i] && dist[i] != INF && (u < 0 || dist[i] < dist[u])) u = i;
        if (u < 0) break;                 /* remaining vertices unreachable */
        done[u] = 1;
        if (u == dst) break;
        for (int v = 0; v < nLoc; v++)
            if (w[u][v] && !done[v] && dist[u] + w[u][v] < dist[v]) {
                dist[v] = dist[u] + w[u][v];
                prev[v] = u;              /* store predecessor to rebuild route */
            }
    }
    if (dist[dst] == INF) return -1;
    int tmp[V], c = 0;
    for (int at = dst; at != -1; at = prev[at]) tmp[c++] = at;
    *len = c;
    for (int i = 0; i < c; i++) path[i] = tmp[c - 1 - i];
    return dist[dst];
}

static void printRoute(void) {
    printf("Shortest Route:\n");
    for (int i = 0; i < lastLen; i++) printf("%s%s", i ? " -> " : "", loc[lastPath[i]]);
    printf("\nTotal Distance: %d km\n", lastDist);
}

static void findRoute(void) {
    char a[64], b[64];
    readLine("Start (name or number): ", a, sizeof a);
    readLine("Destination (name or number): ", b, sizeof b);
    int s = lookup(a), d = lookup(b);
    if (s < 0) { printf("Start location '%s' does not exist.\n", a); return; }
    if (d < 0) { printf("Destination '%s' does not exist.\n", b); return; }
    if (s == d) { printf("Source and destination are the same location. Distance: 0 km.\n"); return; }
    int dist = dijkstra(s, d, lastPath, &lastLen);
    if (dist < 0) { printf("No route: %s cannot be reached from %s.\n", loc[d], loc[s]); lastDist = -1; return; }
    lastDist = dist; lastSrc = s; lastDst = d;
    printRoute();
}

static void addRoadInteractive(void) {
    char a[64], b[64], dbuf[32];
    readLine("From: ", a, sizeof a);
    readLine("To: ", b, sizeof b);
    readLine("Distance (km): ", dbuf, sizeof dbuf);
    char *end;
    long d = strtol(dbuf, &end, 10);
    if (end == dbuf || *end != '\0') { printf("Invalid distance value.\n"); return; }
    if (addRoad(lookup(a), lookup(b), (int)d)) printf("Road added.\n");
}

int main(void) {
    memset(w, 0, sizeof w);
    addRoad(0, 1, 8);  addRoad(0, 5, 14); addRoad(1, 2, 5);
    addRoad(1, 3, 11); addRoad(2, 5, 7);  addRoad(3, 4, 3);
    addRoad(4, 5, 6);  addRoad(3, 5, 9);  addRoad(2, 4, 4);

    char in[32];
    for (;;) {
        printf("\n1. Display locations\n2. Find shortest route\n3. Display route distance\n"
               "4. Exit\n5. Add road (extra)\nChoice: ");
        if (!fgets(in, sizeof in, stdin)) break;
        if (in[1] != '\n') { printf("Invalid choice.\n"); continue; }
        switch (in[0]) {
            case '1': showLocations(); break;
            case '2': findRoute(); break;
            case '3':
                if (lastDist < 0) printf("No route calculated yet.\n");
                else printf("Last route: %s to %s = %d km\n", loc[lastSrc], loc[lastDst], lastDist);
                break;
            case '4': return 0;
            case '5': addRoadInteractive(); break;
            default: printf("Invalid choice.\n");
        }
    }
    return 0;
}
