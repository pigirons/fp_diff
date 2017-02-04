#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define REL_STAT_MAX 8

typedef enum
{
    READ_FAIL,
    READ_ONE,
    READ_END,
    FILE_LENGTH_NOT_EQUAL,
} file_status_t;

struct stat_norm_t
{
    int64_t count;

    float abs_sum;
    float abs_max;
    int64_t abs_max_pos;

    float rel_sum;
    float rel_max;
    int64_t rel_max_pos;

    int64_t rel_thres_stat[REL_STAT_MAX];
};

static inline int float_equal(float a, float b)
{
    return fabsf(a - b) < 1e-30;
}

static void stat_norm_init(struct stat_norm_t *stat)
{
    stat->count = 0;

    stat->abs_sum = 0.0f;
    stat->abs_max = 0.0f;
    stat->abs_max_pos = 0;

    stat->rel_sum = 0.0f;
    stat->rel_max = 0.0f;
    stat->rel_max_pos = 0;

    int i;
    for (i = 0; i < REL_STAT_MAX; i++)
    {
        stat->rel_thres_stat[i] = 0;
    }
}

static void stat_norm_proc_one(struct stat_norm_t *stat,
    float a,
    float b)
{
    int i;

    // computing some statistical value.
    float minus = fabsf(a - b);
    float fa = fabsf(a);
    float fb = fabsf(b);
    float maxab = fa > fb ? fa : fb;

    // computing absolute error.
    stat->abs_sum += minus;
    if (minus > stat->abs_max)
    {
        stat->abs_max = minus;
        stat->abs_max_pos = stat->count;
    }

    // computing relative error.
    float rel = 0.0f;
    if (!float_equal(fa, 0.0f) || !float_equal(fb, 0.0f))
    {
        rel = minus / maxab;
    }
    stat->rel_sum += rel;
    if (rel > stat->rel_max)
    {
        stat->rel_max = rel;
        stat->rel_max_pos = stat->count;
    }

    // relative error threshold.
    float thres = 0.1f;
    for (i = 0; i < 5; i++)
    {
        if (rel > thres)
        {
            stat->rel_thres_stat[i]++;
        }
        thres *= 0.1f;
    }

    stat->count++;
}

static void stat_norm_result(struct stat_norm_t *stat)
{
    int i;

    float abs_avg = stat->abs_sum / stat->count;
    float rel_avg = stat->rel_sum / stat->count;

    printf("************** Data Diff Results **************\n");
    printf("Array Count:                       %ld\n\n", stat->count);
    printf("Average Absolute Error:            %e\n", abs_avg);
    printf("Max Absolute Error Position:       %ld\n", stat->abs_max_pos);
    printf("Max Absolute Error:                %e\n\n", stat->abs_max);
    printf("Average Relative Error:            %e\n", rel_avg);
    printf("Max Relative Error Position:       %ld\n", stat->rel_max_pos);
    printf("Max Relative Error:                %e\n\n", stat->rel_max);

    for (i = 0; i < 5; i++)
    {
        printf("Number of Relative Error(>1e-%d):   %ld\n", i + 1,
            stat->rel_thres_stat[i]);
    }
    printf("***********************************************\n");
}

static file_status_t read_one(FILE *fp1,
    FILE *fp2,
    float *a,
    float *b)
{
    int asz = fread(a, sizeof(float), 1, fp1);
    int bsz = fread(b, sizeof(float), 1, fp2);

    if (asz != bsz)
    {
        return FILE_LENGTH_NOT_EQUAL;
    }
    else if (asz == 1 && bsz == 1)
    {
        return READ_ONE;
    }
    else if (asz == 0 && bsz == 0)
    {
        return READ_END;
    }
    else
    {
        fprintf(stderr, "Error: file read error(%d, %d)\n", asz, bsz);
        exit(0);
    }
    return READ_FAIL;
}

int main(int argc, char *argv[])
{
    FILE *fp1, *fp2;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s file1 file2\n", argv[0]);
        goto _end;
    }

    fp1 = fopen(argv[1], "rb");
    if (!fp1)
    {
        fprintf(stderr, "Error: file1 %s cannot be open.\n", argv[1]);
        goto _end;
    }

    fp2 = fopen(argv[2], "rb");
    if (!fp2)
    {
        fprintf(stderr, "Error: file2 %s cannot be open.\n", argv[2]);
        goto _fp1;
    }
    
    struct stat_norm_t stat;
    stat_norm_init(&stat);

    float a, b;
    file_status_t fs;

    while ((fs = read_one(fp1, fp2, &a, &b)) == READ_ONE)
    {
        stat_norm_proc_one(&stat, a, b);
    }

    if (fs == FILE_LENGTH_NOT_EQUAL)
    {
        fprintf(stderr, "Error: file length not equal.\n");
        goto _fp2;
    }

    stat_norm_result(&stat);

_fp2:
    fclose(fp2);
_fp1:
    fclose(fp1);
_end:
    return 0;
}

