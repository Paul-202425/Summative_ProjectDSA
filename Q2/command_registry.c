/* Q2: Network Device Command Registry - Binary Search Tree */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_CMDS 50
#define MAX_LEN 64
#define CMD_FILE "commands.txt"
#define AUDIT_FILE "audit.log"
#define SESSION_FILE "registry_session.txt"
#define SIMILAR_DISTANCE 2     /* edit-distance threshold for suggestions */

typedef struct Node {
    char cmd[MAX_LEN];
    struct Node *left, *right;
} Node;

static Node *root = NULL;
static int count = 0;

static void trim(char *s) {
    size_t len = strlen(s);
    while (len && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
}

/* returns 1 inserted, 0 duplicate, -1 full/error */
static int insert(const char *cmd) {
    if (count >= MAX_CMDS) return -1;
    Node **link = &root;
    while (*link) {
        int c = strcmp(cmd, (*link)->cmd);
        if (c == 0) return 0;
        link = (c < 0) ? &(*link)->left : &(*link)->right;
    }
    Node *n = malloc(sizeof(Node));
    if (!n) return -1;
    strncpy(n->cmd, cmd, MAX_LEN - 1);
    n->cmd[MAX_LEN - 1] = '\0';
    n->left = n->right = NULL;
    *link = n;
    count++;
    return 1;
}

static int search(const char *cmd) {
    Node *p = root;
    while (p) {
        int c = strcmp(cmd, p->cmd);
        if (c == 0) return 1;
        p = (c < 0) ? p->left : p->right;
    }
    return 0;
}

static void inorder(const Node *n, FILE *out) {
    if (!n) return;
    inorder(n->left, out);
    fprintf(out, "%s\n", n->cmd);
    inorder(n->right, out);
}

static void freeTree(Node *n) {
    if (!n) return;
    freeTree(n->left);
    freeTree(n->right);
    free(n);
}

/* Levenshtein distance */
static int editDistance(const char *a, const char *b) {
    int la = (int)strlen(a), lb = (int)strlen(b);
    int prev[MAX_LEN + 1], cur[MAX_LEN + 1];
    for (int j = 0; j <= lb; j++) prev[j] = j;
    for (int i = 1; i <= la; i++) {
        cur[0] = i;
        for (int j = 1; j <= lb; j++) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            int del = prev[j] + 1, ins = cur[j - 1] + 1, sub = prev[j - 1] + cost;
            int m = del < ins ? del : ins;
            cur[j] = m < sub ? m : sub;
        }
        memcpy(prev, cur, sizeof(int) * (lb + 1));
    }
    return prev[lb];
}

static void findClosest(const Node *n, const char *input, const char **best, int *bestDist) {
    if (!n) return;
    findClosest(n->left, input, best, bestDist);
    int d = editDistance(input, n->cmd);
    if (d < *bestDist) { *bestDist = d; *best = n->cmd; }
    findClosest(n->right, input, best, bestDist);
}

static void logUnknown(const char *cmd) {
    FILE *f = fopen(AUDIT_FILE, "a");
    if (!f) { printf("Warning: cannot write %s\n", AUDIT_FILE); return; }
    time_t t = time(NULL);
    char ts[24];
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", localtime(&t));
    fprintf(f, "%s | UNKNOWN | %s\n", ts, cmd);
    fclose(f);
}

static void loadCommands(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { printf("Warning: cannot open %s. Registry is empty.\n", path); return; }
    char line[256];
    int dup = 0, skipped = 0;
    while (fgets(line, sizeof line, f)) {
        trim(line);
        if (!line[0]) continue;                       /* skip blank lines */
        if (strlen(line) >= MAX_LEN) { skipped++; continue; }
        int r = insert(line);
        if (r == 0) dup++;
        else if (r < 0) { printf("Limit of %d commands reached; remaining ignored.\n", MAX_CMDS); break; }
    }
    fclose(f);
    printf("Loaded %d commands (%d duplicates ignored, %d too long).\n", count, dup, skipped);
    if (count == 0) printf("Note: registry is empty.\n");
}

static void verify(void) {
    char in[256];
    printf("Enter command: ");
    if (!fgets(in, sizeof in, stdin)) return;
    trim(in);
    if (!in[0]) { printf("Invalid input: empty command.\n"); return; }
    if (strlen(in) >= MAX_LEN) { printf("Invalid input: too long.\n"); return; }
    if (search(in)) { printf("COMMAND APPROVED\n"); return; }
    const char *best = NULL;
    int dist = SIMILAR_DISTANCE + 1;
    findClosest(root, in, &best, &dist);
    if (best) printf("Did you mean: %s?\n", best);
    else printf("UNKNOWN COMMAND\n");
    logUnknown(in);     /* not an exact match, so it is an invalid attempt */
}

int main(void) {
    loadCommands(CMD_FILE);
    char in[32];
    for (;;) {
        printf("\n[v]erify [p]rint [s]count [x]clear [q]uit > ");
        if (!fgets(in, sizeof in, stdin)) break;
        if (in[0] == '\n' || in[1] != '\n') { if (in[0] != '\n') printf("Invalid input.\n"); continue; }
        switch (in[0]) {
            case 'v': verify(); break;
            case 'p':
                if (!root) printf("Registry is empty.\n"); else inorder(root, stdout);
                break;
            case 's': printf("Stored commands: %d\n", count); break;
            case 'x': freeTree(root); root = NULL; count = 0; printf("Registry cleared.\n"); break;
            case 'q': goto done;
            default: printf("Invalid input.\n");
        }
    }
done:;
    FILE *f = fopen(SESSION_FILE, "w");
    if (f) { fprintf(f, "Commands at exit: %d\n", count); inorder(root, f); fclose(f); printf("Session saved to %s.\n", SESSION_FILE); }
    freeTree(root);
    return 0;
}
