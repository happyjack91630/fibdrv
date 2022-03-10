#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"

int main()
{
    long long sz;
    size_t mode_seq = 0;
    size_t mode_double = 1;
    FILE *fp = fopen("scripts/time_result", "a");

    char buf[1];
    char write_buf[] = "testing writing";
    int offset = 100; /* TODO: try test something bigger than the limit */

    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    for (int i = 0; i <= offset; i++) {
        long long sz_seq, sz_double;
        lseek(fd, i, SEEK_SET);
        sz_seq = write(fd, write_buf, mode_seq);
        sz_double = write(fd, write_buf, mode_double);
        fprintf(fp, "%d %lld %lld\n", i, sz_seq, sz_double);
    }

    // for (int i = 0; i <= offset; i++) {
    //     sz = write(fd, write_buf, strlen(write_buf));
    //     printf("Writing to " FIB_DEV ", returned the sequence %lld\n", sz);
    // }

    for (int i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, 1);
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%lld.\n",
               i, sz);
    }

    for (int i = offset; i >= 0; i--) {
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, 1);
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%lld.\n",
               i, sz);
    }
    fclose(fp);
    close(fd);
    return 0;
}
