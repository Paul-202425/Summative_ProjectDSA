/* Q5: Embedded Firmware Log Compression Utility - Huffman coding (lossless)
 *
 * .huff format:
 *   4 bytes  magic "HUF1"
 *   4 bytes  original size (uint32, little-endian)
 *   2 bytes  number of distinct symbols (uint16)
 *   per symbol: 1 byte symbol + 4 bytes frequency
 *   remaining: packed code bits (MSB first), last byte zero-padded
 * The tree is rebuilt from the frequency table using the SAME deterministic
 * tie-breaking rule, so encoder and decoder get an identical tree.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define INPUT_FILE "device_log.txt"
#define HUFF_FILE "device_log.huff"
#define OUTPUT_FILE "device_log_restored.txt"

typedef struct HNode {
    unsigned char sym;
    uint32_t freq;
    int order;                      /* deterministic tie-breaker */
    struct HNode *left, *right;
} HNode;

static char *codes[256];

static HNode *newNode(unsigned char s, uint32_t f, int order, HNode *l, HNode *r) {
    HNode *n = malloc(sizeof(HNode));
    if (!n) { fprintf(stderr, "Out of memory.\n"); exit(1); }
    n->sym = s; n->freq = f; n->order = order; n->left = l; n->right = r;
    return n;
}

static void freeTree(HNode *n) {
    if (!n) return;
    freeTree(n->left); freeTree(n->right); free(n);
}

static void freeCodes(void) {
    for (int i = 0; i < 256; i++) { free(codes[i]); codes[i] = NULL; }
}

static int isLeaf(const HNode *n) { return !n->left && !n->right; }

/* Build tree from frequency table. Returns NULL for empty input. */
static HNode *buildTree(const uint32_t freq[256]) {
    HNode *pool[256];
    int cnt = 0, order = 0;
    for (int i = 0; i < 256; i++)
        if (freq[i]) pool[cnt++] = newNode((unsigned char)i, freq[i], i, NULL, NULL);
    if (cnt == 0) return NULL;
    order = 256;
    while (cnt > 1) {
        int a = 0, b = -1;                 /* indices of two smallest */
        #define LESS(x, y) (pool[x]->freq < pool[y]->freq || \
                           (pool[x]->freq == pool[y]->freq && pool[x]->order < pool[y]->order))
        for (int i = 1; i < cnt; i++) if (LESS(i, a)) a = i;
        for (int i = 0; i < cnt; i++) if (i != a && (b < 0 || LESS(i, b))) b = i;
        HNode *parent = newNode(0, pool[a]->freq + pool[b]->freq, order++, pool[a], pool[b]);
        int hi = a > b ? a : b, lo = a < b ? a : b;
        pool[lo] = parent;
        pool[hi] = pool[--cnt];
        #undef LESS
    }
    return pool[0];
}

static void genCodes(const HNode *n, char *buf, int depth) {
    if (!n) return;
    if (isLeaf(n)) {
        buf[depth ? depth : 1] = '\0';
        if (depth == 0) buf[0] = '0';      /* single distinct symbol: code "0" */
        codes[n->sym] = strdup(buf);
        return;
    }
    buf[depth] = '0'; genCodes(n->left, buf, depth + 1);
    buf[depth] = '1'; genCodes(n->right, buf, depth + 1);
}

static void printable(unsigned char c, char *out) {
    if (c == '\n') strcpy(out, "\\n");
    else if (c == '\r') strcpy(out, "\\r");
    else if (c == '\t') strcpy(out, "\\t");
    else if (c == ' ') strcpy(out, "' '");
    else if (c < 32 || c > 126) sprintf(out, "0x%02X", c);
    else sprintf(out, "%c", c);
}

static long fileSize(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long s = ftell(f);
    fclose(f);
    return s;
}

static void put32(FILE *f, uint32_t v) { for (int i = 0; i < 4; i++) fputc((v >> (8 * i)) & 0xFF, f); }
static int get32(FILE *f, uint32_t *v) {
    *v = 0;
    for (int i = 0; i < 4; i++) { int c = fgetc(f); if (c == EOF) return 0; *v |= (uint32_t)c << (8 * i); }
    return 1;
}

static void compress(void) {
    FILE *in = fopen(INPUT_FILE, "rb");
    if (!in) { printf("Error: cannot open %s.\n", INPUT_FILE); return; }
    fseek(in, 0, SEEK_END);
    long size = ftell(in);
    rewind(in);
    unsigned char *data = malloc(size > 0 ? (size_t)size : 1);
    if (!data || fread(data, 1, (size_t)size, in) != (size_t)size) {
        printf("Error: failed to read %s.\n", INPUT_FILE); fclose(in); free(data); return;
    }
    fclose(in);

    uint32_t freq[256] = {0};
    for (long i = 0; i < size; i++) freq[data[i]]++;
    int distinct = 0;
    for (int i = 0; i < 256; i++) if (freq[i]) distinct++;

    printf("\nCharacter frequencies:\n");
    for (int i = 0; i < 256; i++) if (freq[i]) {
        char p[8]; printable((unsigned char)i, p);
        printf("  %-5s %u\n", p, freq[i]);
    }

    HNode *root = buildTree(freq);
    char buf[300];
    if (root) genCodes(root, buf, 0);
    printf("\nHuffman codes:\n");
    for (int i = 0; i < 256; i++) if (codes[i]) {
        char p[8]; printable((unsigned char)i, p);
        printf("  %-5s -> %s\n", p, codes[i]);
    }

    FILE *out = fopen(HUFF_FILE, "wb");
    if (!out) { printf("Error: cannot write %s.\n", HUFF_FILE); freeTree(root); freeCodes(); free(data); return; }
    fwrite("HUF1", 1, 4, out);
    put32(out, (uint32_t)size);
    fputc(distinct & 0xFF, out); fputc((distinct >> 8) & 0xFF, out);
    for (int i = 0; i < 256; i++) if (freq[i]) { fputc(i, out); put32(out, freq[i]); }

    unsigned char cur = 0; int nbits = 0;
    for (long i = 0; i < size; i++)
        for (const char *c = codes[data[i]]; *c; c++) {
            cur = (unsigned char)((cur << 1) | (*c - '0'));
            if (++nbits == 8) { fputc(cur, out); cur = 0; nbits = 0; }
        }
    if (nbits) fputc((unsigned char)(cur << (8 - nbits)), out);
    fclose(out);

    long csize = fileSize(HUFF_FILE);
    printf("\nOriginal size:    %ld bytes\n", size);
    printf("Compressed size:  %ld bytes (includes header)\n", csize);
    if (size > 0)
        printf("Compression ratio: %.2f%% (compressed/original), space saved %.2f%%\n",
               100.0 * csize / size, 100.0 - 100.0 * csize / size);
    printf("Saved to %s\n", HUFF_FILE);

    freeTree(root); freeCodes(); free(data);
}

static int filesIdentical(const char *a, const char *b) {
    FILE *fa = fopen(a, "rb"), *fb = fopen(b, "rb");
    if (!fa || !fb) { if (fa) fclose(fa); if (fb) fclose(fb); return -1; }
    int same = 1, ca, cb;
    do { ca = fgetc(fa); cb = fgetc(fb); if (ca != cb) { same = 0; break; } } while (ca != EOF);
    fclose(fa); fclose(fb);
    return same;
}

static void verify(void) {
    int r = filesIdentical(INPUT_FILE, OUTPUT_FILE);
    if (r < 0) { printf("Error: %s or %s is missing. Compress and decompress first.\n", INPUT_FILE, OUTPUT_FILE); return; }
    if (r) printf("Verification: SUCCESS\nThe restored file is identical to the original.\n");
    else printf("Verification: FAILED\nThe restored file differs from the original.\n");
}

static void decompress(void) {
    FILE *in = fopen(HUFF_FILE, "rb");
    if (!in) { printf("Error: cannot open %s. Compress first.\n", HUFF_FILE); return; }
    char magic[4]; uint32_t size;
    if (fread(magic, 1, 4, in) != 4 || memcmp(magic, "HUF1", 4) != 0 || !get32(in, &size)) {
        printf("Error: %s is not a valid compressed file.\n", HUFF_FILE); fclose(in); return;
    }
    int lo = fgetc(in), hi = fgetc(in);
    if (lo == EOF || hi == EOF) { printf("Error: corrupt header.\n"); fclose(in); return; }
    int distinct = lo | (hi << 8);
    uint32_t freq[256] = {0};
    for (int i = 0; i < distinct; i++) {
        int s = fgetc(in); uint32_t f;
        if (s == EOF || !get32(in, &f)) { printf("Error: corrupt header.\n"); fclose(in); return; }
        freq[s] = f;
    }
    HNode *root = buildTree(freq);
    FILE *out = fopen(OUTPUT_FILE, "wb");
    if (!out) { printf("Error: cannot write %s.\n", OUTPUT_FILE); fclose(in); freeTree(root); return; }

    uint32_t written = 0; int ok = 1;
    if (root && isLeaf(root)) {                   /* only one distinct symbol */
        for (; written < size; written++) fputc(root->sym, out);
    } else if (root) {
        const HNode *cur = root;
        int c;
        while (written < size && (c = fgetc(in)) != EOF) {
            for (int b = 7; b >= 0 && written < size; b--) {
                cur = ((c >> b) & 1) ? cur->right : cur->left;
                if (!cur) { ok = 0; break; }
                if (isLeaf(cur)) { fputc(cur->sym, out); written++; cur = root; }
            }
            if (!ok) break;
        }
    }
    if (written != size) ok = 0;
    fclose(in); fclose(out); freeTree(root);
    if (ok) { printf("Decompressed to %s (%u bytes).\n", OUTPUT_FILE, written); verify(); }
    else printf("Error: compressed data is corrupt or truncated.\n");
}

int main(void) {
    char in[32];
    for (;;) {
        printf("\n1. Compress log\n2. Decompress log\n3. Verify restored log\n4. Exit\nChoice: ");
        if (!fgets(in, sizeof in, stdin)) break;
        if (in[1] != '\n') { printf("Invalid choice.\n"); continue; }
        switch (in[0]) {
            case '1': compress(); break;
            case '2': decompress(); break;
            case '3': verify(); break;
            case '4': return 0;
            default: printf("Invalid choice.\n");
        }
    }
    return 0;
}
