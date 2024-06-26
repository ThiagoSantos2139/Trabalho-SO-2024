#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#define MAX_THREADS 8

typedef struct {
    int *A;
    int *B;
    int *C;
    int *D;
    int *E;
    int n;
    int T;
} Matrices;

typedef struct {
    int thread_id;
    Matrices *matrices;
} ThreadData;

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

int* allocate_matrix(int n) {
    return (int *)malloc(n * n * sizeof(int));
}

void free_matrix(int *matrix) {
    free(matrix);
}

void read_matrix_from_file(const char *filename, int *matrix, int n) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Erro abrindo arquivo");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            fscanf(file, "%d", &matrix[i * n + j]);
        }
    }
    fclose(file);
}

void write_matrix_to_file(const char *filename, int *matrix, int n) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Erro abrindo arquivo");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            fprintf(file, "%d ", matrix[i * n + j]);
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

void sum_matrices(int *A, int *B, int *D, int n, int thread_id, int T) {
    int i, j;
    for (i = thread_id; i < n; i += T) {
        for (j = 0; j < n; j++) {
            D[i * n + j] = A[i * n + j] + B[i * n + j];
        }
    }
}

void multiply_matrices(int *C, int *D, int *E, int n, int thread_id, int T) {
    int i, j, k;
    for (i = thread_id; i < n; i += T) {
        for (j = 0; j < n; j++) {
            E[i * n + j] = 0;
            for (k = 0; k < n; k++) {
                E[i * n + j] += C[i * n + k] * D[k * n + j];
            }
        }
    }
}

void *thread_read_A(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    read_matrix_from_file("arqA.dat", data->matrices->A, data->matrices->n);
    return NULL;
}

void *thread_read_B(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    read_matrix_from_file("arqB.dat", data->matrices->B, data->matrices->n);
    return NULL;
}

void *thread_write_D(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    pthread_mutex_lock(&file_mutex);
    write_matrix_to_file("arqD.dat", data->matrices->D, data->matrices->n);
    pthread_mutex_unlock(&file_mutex);
    return NULL;
}

void *thread_read_C(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    read_matrix_from_file("arqC.dat", data->matrices->C, data->matrices->n);
    return NULL;
}

void *thread_sum_matrices(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    sum_matrices(data->matrices->A, data->matrices->B, data->matrices->D, data->matrices->n, data->thread_id, data->matrices->T);
    return NULL;
}

void *thread_multiply_matrices(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    multiply_matrices(data->matrices->C, data->matrices->D, data->matrices->E, data->matrices->n, data->thread_id, data->matrices->T);
    return NULL;
}

void *thread_reduce_matrix(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    long sum = 0;
    int i, j;
    for (i = data->thread_id; i < data->matrices->n; i += data->matrices->T) {
        for (j = 0; j < data->matrices->n; j++) {
            sum += data->matrices->E[i * data->matrices->n + j];
        }
    }
    return (void *)sum;
}

void *thread_write_E(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    pthread_mutex_lock(&file_mutex);
    write_matrix_to_file("arqE.dat", data->matrices->E, data->matrices->n);
    pthread_mutex_unlock(&file_mutex);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 8) {
        fprintf(stderr, "Erro de comando, utilize: %s T n arqA.dat arqB.dat arqC.dat arqD.dat arqE.dat\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int T = atoi(argv[1]);
    int n = atoi(argv[2]);
    const char *arqA = argv[3];
    const char *arqB = argv[4];
    const char *arqC = argv[5];
    const char *arqD = argv[6];
    const char *arqE = argv[7];

    Matrices matrices;
    matrices.A = allocate_matrix(n);
    matrices.B = allocate_matrix(n);
    matrices.C = allocate_matrix(n);
    matrices.D = allocate_matrix(n);
    matrices.E = allocate_matrix(n);
    matrices.n = n;
    matrices.T = T;

    pthread_t threads[MAX_THREADS];
    ThreadData thread_data[MAX_THREADS];

    thread_data[0].thread_id = 0;
    thread_data[0].matrices = &matrices;
    pthread_create(&threads[0], NULL, thread_read_A, &thread_data[0]);

    thread_data[1].thread_id = 1;
    thread_data[1].matrices = &matrices;
    pthread_create(&threads[1], NULL, thread_read_B, &thread_data[1]);

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);

    clock_t start_sum = clock();
    for (int i = 0; i < T; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].matrices = &matrices;
        pthread_create(&threads[i], NULL, thread_sum_matrices, &thread_data[i]);
    }
    for (int i = 0; i < T; i++) {
        pthread_join(threads[i], NULL);
    }
    clock_t end_sum = clock();

    thread_data[0].thread_id = 0;
    thread_data[0].matrices = &matrices;
    pthread_create(&threads[0], NULL, thread_write_D, &thread_data[0]);

    thread_data[1].thread_id = 1;
    thread_data[1].matrices = &matrices;
    pthread_create(&threads[1], NULL, thread_read_C, &thread_data[1]);

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);

    clock_t start_mult = clock();
    for (int i = 0; i < T; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].matrices = &matrices;
        pthread_create(&threads[i], NULL, thread_multiply_matrices, &thread_data[i]);
    }
    for (int i = 0; i < T; i++) {
        pthread_join(threads[i], NULL);
    }
    clock_t end_mult = clock();

    thread_data[0].thread_id = 0;
    thread_data[0].matrices = &matrices;
    pthread_create(&threads[0], NULL, thread_write_E, &thread_data[0]);

    long total_sum = 0;
    long partial_sum;
    clock_t start_reduce = clock();
    for (int i = 1; i <= T; i++) {
        thread_data[i].thread_id = i - 1;
        thread_data[i].matrices = &matrices;
        pthread_create(&threads[i], NULL, thread_reduce_matrix, &thread_data[i]);
    }
    for (int i = 1; i <= T; i++) {
        pthread_join(threads[i], (void **)&partial_sum);
        total_sum += partial_sum;
    }
    clock_t end_reduce = clock();

    double time_sum = (double)(end_sum - start_sum) / CLOCKS_PER_SEC;
    double time_mult = (double)(end_mult - start_mult) / CLOCKS_PER_SEC;
    double time_reduce = (double)(end_reduce - start_reduce) / CLOCKS_PER_SEC;
    double time_total = time_sum + time_mult + time_reduce;

    printf("Redução: %ld\n", total_sum);
    printf("Tempo soma: %lf segundos.\n", time_sum);
    printf("Tempo multiplicação: %lf segundos.\n", time_mult);
    printf("Tempo redução: %lf segundos.\n", time_reduce);
    printf("Tempo total: %lf segundos.\n", time_total);

    free_matrix(matrices.A);
    free_matrix(matrices.B);
    free_matrix(matrices.C);
    free_matrix(matrices.D);
    free_matrix(matrices.E);

    return 0;
}
