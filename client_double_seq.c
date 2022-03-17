#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"
#define sample_size 1000
#define mode 2


int main()
{
    FILE *fp = fopen("time_result", "a");
    char write_buf[] = "testing writing";
    int offset = 100; /* TODO: try test something bigger than the limit */

    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    for (int i = 0; i <= offset; i++) {
        double time[mode][sample_size] = {0};
        double mean[mode] = {0};
        double std[mode] = {0};
        double result[mode] = {0};

        for (int s = 0; s < sample_size; s++) {
            double seq_time, doubling_time;
            lseek(fd, i, SEEK_SET);
            seq_time = (double) write(fd, write_buf, 0);
            doubling_time = (double) write(fd, write_buf, 1);
            time[0][s] = seq_time;
            mean[0] += time[0][s];
            time[1][s] = doubling_time;
            mean[1] += time[1][s];
        }
        for (int m = 0; m < mode; m++) {
            int count = 0;
            mean[m] /= sample_size;
            for (int n = 0; n < sample_size; n++) {
                std[m] += (time[m][n] - mean[m]) * (time[m][n] - mean[m]);
            }
            std[m] = sqrt(std[m] / (sample_size - 1));
            for (int n = 0; n < sample_size; n++) {
                if (time[m][n] <= (mean[m] + 2 * std[m]) &&
                    time[m][n] >= (mean[m] - 2 * std[m])) {
                    result[m] += time[m][n];
                    count++;
                }
            }
            result[m] /= count;
        }
        fprintf(fp, "%d %f %f\n", i, result[0], result[1]);
    }

    fclose(fp);  // close result_txt
    close(fd);   // close fibdrv
    return 0;
}
