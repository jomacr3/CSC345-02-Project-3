/* CSC345 Virtual Memory Manager: demand paging, FIFO TLB, no page replacement. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define PAGE_SIZE 256
#define PAGE_COUNT 256
#define FRAME_COUNT 256
#define TLB_SIZE 16

typedef struct {
    int page;
    int frame;
    int valid;
} TLBEntry;

static signed char memory[FRAME_COUNT][PAGE_SIZE];
static int page_table[PAGE_COUNT];
static TLBEntry tlb[TLB_SIZE];
static int tlb_cursor = 0;
static int used_frames = 0;

static int tlb_lookup(int page) {
    for (int i = 0; i < TLB_SIZE; ++i) {
        if (tlb[i].valid && tlb[i].page == page)
            return tlb[i].frame;
    }
    return -1;
}

static void tlb_insert(int page, int frame) {
    /* Remove duplicates without changing the FIFO insertion order. */
    for (int i = 0; i < TLB_SIZE; ++i) {
        if (tlb[i].valid && tlb[i].page == page)
            tlb[i].valid = 0;
    }

    tlb[tlb_cursor] = (TLBEntry){page, frame, 1};
    tlb_cursor = (tlb_cursor + 1) % TLB_SIZE;
}

static int load_page(FILE *backing, int page) {
    if (used_frames >= FRAME_COUNT)
        return -1;

    int frame = used_frames;

    if (fseek(backing, (long)page * PAGE_SIZE, SEEK_SET) != 0 ||
        fread(memory[frame], 1, PAGE_SIZE, backing) != PAGE_SIZE)
        return -1;

    ++used_frames;
    page_table[page] = frame;
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
        perror("Could not open addresses file or BACKING_STORE.bin");
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
        fclose(addresses);
        fclose(backing);
        return 1;
    }

    for (int i = 0; i < PAGE_COUNT; ++i)
        page_table[i] = -1;

    long total = 0, faults = 0, hits = 0;
    unsigned int input;

    while (fscanf(addresses, "%u", &input) == 1) {
        unsigned int logical = input & 0xFFFFu;
        int page = (int)((logical >> 8) & 0xFFu);
        int offset = (int)(logical & 0xFFu);

        int frame = tlb_lookup(page);

        if (frame >= 0) {
            ++hits;
        } else {
            frame = page_table[page];

            if (frame < 0) {
                ++faults;
                frame = load_page(backing, page);

                if (frame < 0) {
                    fprintf(stderr, "Failed to load page %d\n", page);
                    fclose(out1);
                    fclose(out2);
                    fclose(out3);
                    fclose(addresses);
                    fclose(backing);
                    return 1;
                }
            }

            tlb_insert(page, frame);
        }

        int physical = frame * PAGE_SIZE + offset;
        int value = (int)memory[frame][offset];

        fprintf(out1, "%u\n", input);
        fprintf(out2, "%d\n", physical);
        fprintf(out3, "%d\n", value);
        ++total;
    }

    if (ferror(addresses)) {
        fprintf(stderr, "Error reading addresses file\n");
        fclose(out1);
        fclose(out2);
        fclose(out3);
        fclose(addresses);
        fclose(backing);
        return 1;
    }

    printf("Algorithm: No page replacement\n");
    printf("Addresses translated: %ld\n", total);
    printf("Page faults: %ld\n", faults);
    printf("TLB hits: %ld\n", hits);
    printf("Page-fault rate: %.6f\n",
           total ? (double)faults / total : 0.0);
    printf("TLB hit rate: %.6f\n",
           total ? (double)hits / total : 0.0);

    fclose(out1);
    fclose(out2);
    fclose(out3);
    fclose(addresses);
    fclose(backing);

    return 0;
}

