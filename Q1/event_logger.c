/* Q1: Embedded Device Event Logger - doubly linked list */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_EVENTS 25
#define SESSION_FILE "event_session.txt"

typedef struct Event {
    int id;
    char type[32];
    char device[32];
    char timestamp[24];
    int severity;               /* 1 (low) .. 5 (critical) */
    struct Event *prev, *next;
} Event;

typedef struct {
    Event *head, *tail, *current;
    int count, nextId;
} Logger;

/* ---------- helpers ---------- */
static void readLine(const char *prompt, char *buf, size_t size) {
    printf("%s", prompt);
    if (!fgets(buf, (int)size, stdin)) { buf[0] = '\0'; return; }
    buf[strcspn(buf, "\r\n")] = '\0';
}

static void now(char *out, size_t size) {
    time_t t = time(NULL);
    strftime(out, size, "%Y-%m-%d %H:%M:%S", localtime(&t));
}

static void printEvent(const Event *e, int pos, int total) {
    printf("\n--- Event %d of %d ---\n", pos, total);
    printf("ID: %d\nType: %s\nDevice: %s\nTime: %s\nSeverity: %d\n",
           e->id, e->type, e->device, e->timestamp, e->severity);
}

static int positionOf(const Logger *l, const Event *e) {
    int i = 1;
    for (const Event *p = l->head; p; p = p->next, i++)
        if (p == e) return i;
    return 0;
}

/* ---------- core operations ---------- */
static void initLogger(Logger *l) {
    l->head = l->tail = l->current = NULL;
    l->count = 0;
    l->nextId = 1;
}

static Event *createEvent(Logger *l, const char *type, const char *dev, int sev) {
    Event *e = malloc(sizeof(Event));
    if (!e) return NULL;
    e->id = l->nextId++;
    strncpy(e->type, type, sizeof e->type - 1);   e->type[sizeof e->type - 1] = '\0';
    strncpy(e->device, dev, sizeof e->device - 1); e->device[sizeof e->device - 1] = '\0';
    now(e->timestamp, sizeof e->timestamp);
    e->severity = sev;
    e->prev = e->next = NULL;
    return e;
}

/* Remove oldest (head) event; fixes head, tail and current. */
static void removeOldest(Logger *l) {
    if (!l->head) return;
    Event *old = l->head;
    l->head = old->next;
    if (l->head) l->head->prev = NULL; else l->tail = NULL;
    if (l->current == old) l->current = l->head;   /* never leave dangling pointer */
    free(old);
    l->count--;
}

static int addEvent(Logger *l, const char *type, const char *dev, int sev) {
    if (l->count == MAX_EVENTS) {
        removeOldest(l);
        printf("[Memory full] Oldest event removed.\n");
    }
    Event *e = createEvent(l, type, dev, sev);
    if (!e) { printf("Error: out of memory.\n"); return 0; }
    if (!l->tail) { l->head = l->tail = e; l->current = e; }  /* first event */
    else { e->prev = l->tail; l->tail->next = e; l->tail = e; }
    l->count++;
    return 1;
}

static void clearAll(Logger *l) {
    Event *p = l->head;
    while (p) { Event *n = p->next; free(p); p = n; }
    l->head = l->tail = l->current = NULL;
    l->count = 0;
}

static void showCurrent(const Logger *l) {
    if (!l->current) { printf("No events logged.\n"); return; }
    printEvent(l->current, positionOf(l, l->current), l->count);
}

static void moveNext(Logger *l) {
    if (!l->current) { printf("No events logged.\n"); return; }
    if (!l->current->next) { printf("Already at the newest event.\n"); return; }
    l->current = l->current->next;
    showCurrent(l);
}

static void movePrev(Logger *l) {
    if (!l->current) { printf("No events logged.\n"); return; }
    if (!l->current->prev) { printf("Already at the oldest event.\n"); return; }
    l->current = l->current->prev;
    showCurrent(l);
}

static void saveSession(const Logger *l) {
    FILE *f = fopen(SESSION_FILE, "w");
    if (!f) { printf("Warning: could not save session.\n"); return; }
    fprintf(f, "ID | TYPE | DEVICE | TIMESTAMP | SEVERITY\n");
    for (const Event *e = l->head; e; e = e->next)
        fprintf(f, "%d | %s | %s | %s | %d\n", e->id, e->type, e->device, e->timestamp, e->severity);
    fclose(f);
    printf("Session saved to %s (%d events).\n", SESSION_FILE, l->count);
}

static void addInteractive(Logger *l) {
    char type[64], dev[64], buf[32];
    int sev;
    readLine("Event type (e.g. TEMP_WARNING): ", type, sizeof type);
    readLine("Sensor/device ID: ", dev, sizeof dev);
    if (!type[0] || !dev[0]) { printf("Type and device cannot be empty.\n"); return; }
    readLine("Severity (1-5): ", buf, sizeof buf);
    char *end;
    sev = (int)strtol(buf, &end, 10);
    if (end == buf || *end != '\0' || sev < 1 || sev > 5) { printf("Invalid severity.\n"); return; }
    if (addEvent(l, type, dev, sev)) printf("Event added (%d/%d).\n", l->count, MAX_EVENTS);
}

int main(void) {
    Logger log;
    initLogger(&log);
    char in[64];
    printf("Embedded Event Logger\n");
    for (;;) {
        printf("\n[n]ext [p]rev [a]dd [c]urrent [x]clear [q]uit > ");
        if (!fgets(in, sizeof in, stdin)) break;
        char cmd = in[0];
        if (cmd == '\n' || in[1] != '\n') { if (cmd != '\n') printf("Invalid command.\n"); continue; }
        switch (cmd) {
            case 'n': moveNext(&log); break;
            case 'p': movePrev(&log); break;
            case 'a': addInteractive(&log); break;
            case 'c': showCurrent(&log); break;
            case 'x': clearAll(&log); printf("All events cleared.\n"); break;
            case 'q': saveSession(&log); clearAll(&log); return 0;
            default: printf("Invalid command.\n");
        }
    }
    saveSession(&log);  /* EOF: still save and free */
    clearAll(&log);
    return 0;
}
