#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MIN_LENGTH 20
#define DEFAULT_LENGTH 80

typedef struct ch_count {
    unsigned char ch;
    size_t count;
} ch_count_t;

void get_counts (unsigned char *data, size_t size, ch_count_t count[static 0x100]) {
    size_t i;
    // initialize data
    for (i = 0; i < 0x100; i++) {
        count[i].ch = (char)i;
        count[i].count = 0;
    }
    // get frequency distribution
    for (i = 0; i < size; i++) {
        count[data[i]].count++;
    }
}

void print_graph (int length, size_t max_count, size_t size, ch_count_t count[static 0x100]) {
    int i;
    double topPercentage = (double)max_count / size;
    fprintf (stdout, "   +0%%");
    for (i = 6; i < length-6; i++) { // skip until end of line, making room for top percentage
        fputc ('-', stdout);
    }
    fprintf (stdout, "%03.2f%%\n", topPercentage*100);
    for (i = 0; i < 0x100; i++) {
        int numStars = (int)((((double)count[i].count / size) / topPercentage) * (length-4));
        fprintf (stdout, "%02X |", count[i].ch);
        while (numStars --> 0) {
            fputc ('*', stdout);
        }
        fputc ('\n', stdout);
    }
    fprintf (stdout, "   +");
    for (i = 4; i < length; i++) { // skip until end of line, making room for top percentage
        fputc ('-', stdout);
    }
    fputc ('\n', stdout);
}

void print_table (size_t size, ch_count_t count[static 0x100]) {
    int i;
    for (i = 0; i < 0x100; i++) {
        double frequency = ((double)count[i].count / size) * 100;
        fprintf (stdout, "%02X (%c): Frequency: %06.3f%%, Count: %lu\n", count[i].ch, isprint (count[i].ch) ? count[i].ch : '.', frequency, count[i].count);
    }
    fprintf (stdout, "Total Size: %lu\n", size);
}

static inline void show_help () {
    fprintf (stderr, "usage: histogram [options] input\n");
    fprintf (stderr, "  options:\n");
    fprintf (stderr, "    -l num     Limit graph line length to num, minimum of %d\n", MIN_LENGTH);
    fprintf (stderr, "    -s         Sort the data descending\n");
    fprintf (stderr, "    -t         Show table view only\n");
    fprintf (stderr, "    -g         Show graph view only\n");
}

int count_cmp (const void *p1, const void *p2) {
    return ((ch_count_t*)p2)->count - ((ch_count_t*)p1)->count;
}

int main (int argc, char **argv) {
    argc--; argv++; // skip first argument
    int length = DEFAULT_LENGTH;
    int sort = 0;
    int graph = 1;
    int table = 1;
    while (argc) {
        size_t arglen;
        if ((arglen = strlen (argv[0])) > 1 && argv[0][0] == '-') {
            switch (argv[0][1]) {
                case 'l':
                    if (arglen < 4) break; // invalid argument
                    length = atoi (&argv[0][3]);
                    break;
                case 's':
                    sort = 1;
                    break;
                case 't':
                    graph = 0;
                    break;
                case 'g':
                    table = 0;
                    break;
                default:
                    fprintf (stderr, "Invalid option: %c\n", argv[0][1]);
                case 'h':
                case '?':
                    show_help ();
                    return 2;
            }
        } else {
            break; // out of options
        }
        argc--; argv++; // next argument
    }
    if (argc < 1) {
        fprintf (stderr, "Too few arguments.\n");
        show_help ();
        return 1;
    }
    if (length < MIN_LENGTH) {
        length = MIN_LENGTH;
    }
    int fd;
    struct stat statbuf;
    void *data;
    ch_count_t count[0x100];
    size_t max_count = 0;
    if ((fd = open (argv[0], O_RDONLY, 0)) < 0) {
        fprintf (stderr, "Cannot open %s for reading.\n", argv[0]);
        return 3;
    }
    if (fstat (fd, &statbuf) < 0) {
        fprintf (stderr, "Cannot stat %s.\n", argv[0]);
        return 4;
    }
    if ((data = mmap (NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) {
        fprintf (stderr, "Cannot read %s.\n", argv[0]);
        return 5;
    }

    get_counts ((unsigned char*)data, statbuf.st_size, count);
    if (sort) {
        qsort (count, 0x100, sizeof (ch_count_t), count_cmp);
        max_count = count[0].count;
    } else {
        int i;
        for (i = 0; i < 0x100; i++) {
            if (count[i].count > max_count) {
                max_count = count[i].count;
            }
        }
    }
    if (graph) {
        print_graph (length, max_count, statbuf.st_size, count);
    }
    if (table) {
        print_table (statbuf.st_size, count);
    }

    munmap (data, statbuf.st_size);
    close (fd);
}
