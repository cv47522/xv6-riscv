#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"


int
main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(2, "Usage: sixfive FILE\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY); /* 0,1,2 are taken, so open() takes 3 */
    if (fd < 0) {
        fprintf(2, "sixfive: cannot open %s\n", argv[1]);
        return 1;
    }

    const char separators[] = {"-", "\r", "\t", "\n", ".", ",", "/"};
    char buf[100];
    int bytes_read = 0;

    while (bytes_read = read(fd, buf, sizeof(buf)) > 0) {
        // 1. Find the first separator addr from buf
        char *p_sep_min = sizeof(buf); // Lowest addr of the first separator
        for (int i = 0; i < sizeof(separators); i++) {
            char sep = separators[i];
            char *p_sep = strchr(buf, sep);
            if (p_sep != 0 && p_sep < p_sep_min) {
                p_sep_min = p_sep;
            }
        }

        // 2. Extract a word w/ offset (p_sep - buf))
        for (int i = 0; i < p_sep_min; i++) { // Read a char from buf
            char c = buf[i];
            // 3. Check if the word is a number (atoi(w) > 0)
            if (atoi(c) == 0)
                break;
            printf("%c", c);
        }
    }
    return 0;
}