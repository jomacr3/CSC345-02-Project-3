/*
 * CSC345 Virtual Memory Manager - main1.c
 * First version: 256 physical frames, demand paging, FIFO TLB.
 * TODO for project: test against the instructor's addresses.txt,
 * BACKING_STORE.bin, and correct.txt before submission.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define PAGESIZE 256
#define PAGECOUNT 256
#define FRAMECOUNT 256
#define TLBSIZE 16

typedef struct {
    int page;
    int frame;
    int valid;
} TLBEntry;

static signed char memory[FRAMECOUNT][PAGESIZE];
static int pageTable[PAGECOUNT];
static TLBEntry tlb[TLBSIZE];
static int tlbCursor = 0;
static int usedFrames = 0;

static int getTLB(int page) {
    for (int i = 0; i < TLBSIZE; i++) {
        if (tlb[i].valid && tlb[i].page == page)
            return tlb[i].frame;
    }
    return -1;
}

static void setTLB(int page, int frame) {
    /* FIFO: overwrite the entry that has been in the TLB longest. */
    tlb[tlbCursor] = (TLBEntry){page, frame, 1};
    tlbCursor = (tlbCursor + 1) % TLBSIZE;
}

static int loadPage(FILE *backing, int page) {
    if (usedFrames >= FRAMECOUNT) return -1;
    int frame = usedFrames;
    if (fseek(backing, (long)page * PAGESIZE, SEEK_SET) != 0 ||
        fread(memory[frame], 1, PAGESIZE, backing) != PAGESIZE)
        return -1;
    usedFrames++;
    pageTable[page] = frame;
    return frame;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s addresses.txt\n", argv[0]);
        return 1;
    }
    FILE *addresses = fopen(argv[1], "r");
    FILE *backing = fopen("BACKING_STORE.bin", "rb");
    if (!addresses || !backing) {
        perror("Could not open addresses or BACKING_STORE.bin");
        if (addresses) fclose(addresses);
        if (backing) fclose(backing);
        return 1;
    }
    FILE *out1 = fopen("out1.txt", "w");
    FILE *out2 = fopen("out2.txt", "w");
    FILE *out3 = fopen("out3.txt", "w");
    if (!out1 || !out2 || !out3) {
        fprintf(stderr, "Could not open an output file\n");
        if (out1) fclose(out1);
        if (out2) fclose(out2);
        if (out3) fclose(out3);
        fclose(addresses);
        fclose(backing);
        return 1;
    }
    for (int i = 0; i < PAGECOUNT; i++) pageTable[i] = -1;

    long total = 0, faults = 0, hits = 0;
    unsigned int input;
    while (fscanf(addresses, "%u", &input) == 1) {
        /* Only the low 16 bits matter: high 8 = page, low 8 = offset. */
        unsigned int logical = input & 0xFFFFu;
        int page = (int)((logical >> 8) & 0xFFu);
        int offset = (int)(logical & 0xFFu);
        int frame = getTLB(page);
        if (frame >= 0) {
            hits++;
        } else {
            frame = pageTable[page];
            if (frame < 0) {
                faults++;
                frame = loadPage(backing, page);
                if (frame < 0) {
                    fprintf(stderr, "Failed to load page %d\n", page);
                    fclose(out1); fclose(out2); fclose(out3);
                    fclose(addresses); fclose(backing);
                    return 1;
                }
            }
            setTLB(page, frame);
        }
        int physical = frame * PAGESIZE + offset;
        int value = (int)memory[frame][offset];
        fprintf(out1, "%u\n", input);
        fprintf(out2, "%d\n", physical);
        fprintf(out3, "%d\n", value);
        total++;
    }
    printf("Addresses translated: %ld\nPage faults: %ld\nTLB hits: %ld\n", total, faults, hits);
    printf("Page-fault rate: %.4f\nTLB hit rate: %.4f\n",
           total ? (double)faults / total : 0.0,
           total ? (double)hits / total : 0.0);
    fclose(out1); fclose(out2); fclose(out3);
    fclose(addresses); fclose(backing);
    return 0;
}
