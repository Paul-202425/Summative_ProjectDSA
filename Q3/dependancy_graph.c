/* Q3: Server Dependency Network Analyzer - directed graph (adjacency matrix) */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_SERVERS 30
#define NAME_LEN 40

static char names[MAX_SERVERS][NAME_LEN];
static int adj[MAX_SERVERS][MAX_SERVERS];   /* adj[a][b]=1 : a depends on b (a -> b) */
static int n = 0;

static void readLine(const char *prompt, char *buf, size_t size) {
    printf("%s", prompt);
    if (!fgets(buf, (int)size, stdin)) { buf[0] = '\0'; return; }
    buf[strcspn(buf, "\r\n")] = '\0';
    size_t len = strlen(buf);
    while (len && isspace((unsigned char)buf[len - 1])) buf[--len] = '\0';
    char *p = buf;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != buf) memmove(buf, p, strlen(p) + 1);
}

static int sameName(const char *a, const char *b) {   /* case-insensitive */
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

static int find(const char *name) {
    for (int i = 0; i < n; i++) if (sameName(names[i], name)) return i;
    return -1;
}

static int addServer(const char *name) {
    if (!name[0]) { printf("Server name cannot be empty.\n"); return 0; }
    if (find(name) >= 0) { printf("Server '%s' already exists.\n", name); return 0; }
    if (n >= MAX_SERVERS) { printf("Graph is full (%d servers).\n", MAX_SERVERS); return 0; }
    strncpy(names[n], name, NAME_LEN - 1);
    names[n][NAME_LEN - 1] = '\0';
    for (int i = 0; i <= n; i++) adj[n][i] = adj[i][n] = 0;
    n++;
    return 1;
}

static int removeServer(const char *name) {
    int k = find(name);
    if (k < 0) { printf("Server '%s' does not exist.\n", name); return 0; }
    /* shift rows/columns so the matrix stays contiguous */
    for (int i = k; i < n - 1; i++) strcpy(names[i], names[i + 1]);
    for (int i = k; i < n - 1; i++)
        for (int j = 0; j < n; j++) adj[i][j] = adj[i + 1][j];
    for (int j = k; j < n - 1; j++)
        for (int i = 0; i < n - 1; i++) adj[i][j] = adj[i][j + 1];
    n--;
    for (int i = 0; i < n; i++) adj[n][i] = adj[i][n] = 0;   /* tidy freed slot */
    adj[n][n] = 0;
    return 1;
}

static int addDep(const char *from, const char *to) {
    int a = find(from), b = find(to);
    if (a < 0 || b < 0) { printf("Both servers must exist (%s%s%s).\n",
        a < 0 ? from : "", (a < 0 && b < 0) ? ", " : "", b < 0 ? to : ""); return 0; }
    if (a == b) { printf("A server cannot depend on itself.\n"); return 0; }
    if (adj[a][b]) { printf("Dependency already exists.\n"); return 0; }
    adj[a][b] = 1;
    return 1;
}

static int removeDep(const char *from, const char *to) {
    int a = find(from), b = find(to);
    if (a < 0 || b < 0) { printf("Both servers must exist.\n"); return 0; }
    if (!adj[a][b]) { printf("That dependency does not exist.\n"); return 0; }
    adj[a][b] = 0;
    return 1;
}

static void query(const char *name) {
    if (n == 0) { printf("The graph is empty.\n"); return; }
    int k = find(name);
    if (k < 0) { printf("Server '%s' does not exist.\n", name); return; }
    int found = 0;
    printf("Server: %s\nDirect dependencies:\n", names[k]);
    for (int j = 0; j < n; j++) if (adj[k][j]) { printf("  %s\n", names[j]); found = 1; }
    if (!found) printf("  (none)\n");
    found = 0;
    printf("Services depending on %s:\n", names[k]);
    for (int i = 0; i < n; i++) if (adj[i][k]) { printf("  %s\n", names[i]); found = 1; }
    if (!found) printf("  (none)\n");
}

static void showMatrix(void) {
    if (n == 0) { printf("The graph is empty.\n"); return; }
    printf("\nIndex legend:\n");
    for (int i = 0; i < n; i++) printf("  [%d] %s\n", i, names[i]);
    printf("\n     ");
    for (int j = 0; j < n; j++) printf("%3d", j);
    printf("\n");
    for (int i = 0; i < n; i++) {
        printf("  %2d ", i);
        for (int j = 0; j < n; j++) printf("%3d", adj[i][j]);
        printf("\n");
    }
}

static void initialGraph(void) {
    const char *s[] = {"Web Server", "API Server", "Authentication Server",
                       "Cache Server", "Database Server", "Backup Server"};
    for (int i = 0; i < 6; i++) addServer(s[i]);
    const char *e[][2] = {
        {"Web Server", "API Server"}, {"Web Server", "Authentication Server"},
        {"API Server", "Database Server"}, {"API Server", "Cache Server"},
        {"Authentication Server", "Database Server"}, {"Cache Server", "Database Server"},
        {"Database Server", "Backup Server"}};
    for (int i = 0; i < 7; i++) addDep(e[i][0], e[i][1]);
}

int main(void) {
    char cmd[16], a[NAME_LEN], b[NAME_LEN];
    initialGraph();
    for (;;) {
        printf("\n[a]dd server [r]emove server [c]reate dep [d]elete dep [q]uery [m]atrix e[x]it > ");
        if (!fgets(cmd, sizeof cmd, stdin)) break;
        if (cmd[0] == '\n') continue;
        if (cmd[1] != '\n') { printf("Invalid command.\n"); continue; }
        switch (cmd[0]) {
            case 'a': readLine("Server name: ", a, sizeof a);
                      if (addServer(a)) { printf("Server added.\n"); }
                      break;
            case 'r': readLine("Server name: ", a, sizeof a);
                      if (removeServer(a)) { printf("Server and its dependencies removed.\n"); }
                      break;
            case 'c': readLine("From (dependent): ", a, sizeof a);
                      readLine("To (dependency): ", b, sizeof b);
                      if (addDep(a, b)) { printf("Dependency created.\n"); }
                      break;
            case 'd': readLine("From: ", a, sizeof a);
                      readLine("To: ", b, sizeof b);
                      if (removeDep(a, b)) { printf("Dependency deleted.\n"); }
                      break;
            case 'q': readLine("Server name: ", a, sizeof a); query(a); break;
            case 'm': showMatrix(); break;
            case 'x': return 0;
            default: printf("Invalid command.\n");
        }
    }
    return 0;
}
