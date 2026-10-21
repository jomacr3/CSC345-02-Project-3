/*
 * CSC345 Virtual Memory Manager - main2.c
 * Second version: FIFO page replacement with a 128-frame default.
 * Accepts optional frame count: ./main2 addresses.txt 32
 * TODO: run experiments for 32, 64, 128, 256, 512, 768, 1024
 * (Only 256 distinct virtual pages can be resident at once.)
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define PAGE_SIZE 256
#define PAGE_COUNT 256
#define MAX_FRAMES 1024
#define TLB_SIZE 16

typedef struct {
    int page;
    int frame;
    int valid;
} TLBEntry;

static signed char memory[MAX_FRAMES][PAGE_SIZE];
static int page_table[PAGE_COUNT];
static int frame_page[MAX_FRAMES];
static TLBEntry tlb[TLB_SIZE];
static int tlb_next = 0;
static int next_free = 0;
static int fifo_victim = 0;

static int tlb_lookup(int page) {
    for (int i = 0; i < TLB_SIZE; i++)
        if (tlb[i].valid && tlb[i].page == page) return tlb[i].frame;
    return -1;
}

static void tlb_insert(int page, int frame) {
    tlb[tlb_next] = (TLBEntry){page, frame, 1};
    tlb_next = (tlb_next + 1) % TLB_SIZE;
}

static void tlb_remove(int page) {
    /* Evicted pages must never remain accessible through the TLB. */
    for (int i = 0; i < TLB_SIZE; i++)
        if (tlb[i].valid && tlb[i].page == page) tlb[i].valid = 0;
}

static int page_fault(FILE *backing, int page, int frame_count) {
    int frame;
    if (next_free < frame_count) {
        frame = next_free++;
    } else {
        /* FIFO picks the frame containing the oldest loaded page. */
        frame = fifo_victim;
        fifo_victim = (fifo_victim + 1) % frame_count;
        int old_page = frame_page[frame];
        if (old_page >= 0) {
            page_table[old_page] = -1;
            tlb_remove(old_page);
        }
    }
    if (fseek(backing, (long)page * PAGE_SIZE, SEEK_SET) != 0 ||
        fread(memory[frame], 1, PAGE_SIZE, backing) != PAGE_SIZE)
        return -1;
    frame_page[frame] = page;
    page_table[page] = frame;
    return frame;
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s addresses.txt [frame_count]\n", argv[0]);
        return 1;
    }
    int frame_count = 128;
    if (argc == 3) {
        char *end;
        long n = strtol(argv[2], &end, 10);
        if (*end != '\0' || n < 1 || n > MAX_FRAMES) {
            fprintf(stderr, "frame_count must be between 1 and %d\n", MAX_FRAMES);
            return 1;
        }
        frame_count = (int)n;
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
        fprintf(stderr, "Could not open output files\n");
        if (out1) fclose(out1);
        if (out2) fclose(out2);
        if (out3) fclose(out3);
        fclose(addresses); fclose(backing);
        return 1;
    }
    for (int i = 0; i < PAGE_COUNT; i++) page_table[i] = -1;
    for (int i = 0; i < MAX_FRAMES; i++) frame_page[i] = -1;

    long total = 0, faults = 0, hits = 0;
    unsigned int input;
    while (fscanf(addresses, "%u", &input) == 1) {
        unsigned int logical = input & 0xFFFFu;
        int page = (int)((logical >> 8) & 0xFFu);
        int offset = (int)(logical & 0xFFu);
        int frame = tlb_lookup(page);
        if (frame >= 0) {
            hits++;
        } else {
            frame = page_table[page];
            if (frame < 0) {
                faults++;
                frame = page_fault(backing, page, frame_count);
                if (frame < 0) {
                    fprintf(stderr, "Failed to load page %d\n", page);
                    fclose(out1); fclose(out2); fclose(out3);
                    fclose(addresses); fclose(backing);
                    return 1;
                }
            }
            tlb_insert(page, frame);
        }
        fprintf(out1, "%u\n", input);
        fprintf(out2, "%d\n", frame * PAGE_SIZE + offset);
        fprintf(out3, "%d\n", (int)memory[frame][offset]);
        total++;
    }
    printf("Frames: %d\nAddresses translated: %ld\nPage faults: %ld\nTLB hits: %ld\n",
           frame_count, total, faults, hits);
    printf("Page-fault rate: %.4f\nTLB hit rate: %.4f\n",
           total ? (double)faults / total : 0.0,
           total ? (double)hits / total : 0.0);
    fclose(out1); fclose(out2); fclose(out3);
    fclose(addresses); fclose(backing);
    return 0;
}
