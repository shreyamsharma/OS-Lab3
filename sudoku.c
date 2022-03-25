
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

typedef struct {
	int row;
	int col;
	int(*sudoku_grid)[9];
} parameters;

void *validateSubgrid(void*);
void *validateRows(void*);
void *validateCols(void*);
int readSudokuGrid(int(*grid)[9], FILE *);

#define NTHREADS 11

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Usage: executable-name [input-file]\n");
		exit(EXIT_FAILURE);
	}

	int sudoku_grid[9][9];
	FILE *fp = fopen(argv[1], "r");

	// Initialize parameters for subgrid evaluation threads
	parameters *data[9];
	int row, col, i = 0;

	for (row = 0; row < 9; row += 3) {
		for (col = 0; col < 9; col += 3, ++i) {
			data[i] = (parameters*) malloc(sizeof(parameters));
			if (data[i] == NULL) {
				int err = errno;
				puts("malloc failed");
				puts(strerror(err));
				exit(EXIT_FAILURE);
			}

			data[i]->row = row;
			data[i]->col = col;
			data[i]->sudoku_grid = sudoku_grid;
		}
	}

	// Validate sudoku grid
	pthread_t tid[NTHREADS];
	int p, j, h, retCode, check_flag = 0, t_status[NTHREADS];

    if (readSudokuGrid(sudoku_grid, fp)) {
        puts("Unable to read the grid from the file.");
        exit(EXIT_FAILURE);
    }

    // Create threads for subgrid validation
    for (p = 0; p < 9; ++p) {
        if ((retCode = pthread_create(&tid[p], NULL, validateSubgrid, (void*) data[p]))) {
            fprintf(stderr, "Error - pthread_create() return code: %d\n", retCode);
            exit(EXIT_FAILURE);
        }
    }

    // Create thread for rows validation
    if ((retCode = pthread_create(&tid[9], NULL, validateRows, (void*) data[0]))) {
        fprintf(stderr, "Error - pthread_create() return code: %d\n", retCode);
        exit(EXIT_FAILURE);
    }

    // Create thread for columns validation
    if ((retCode = pthread_create(&tid[10], NULL, validateCols, (void*) data[0]))) {
        fprintf(stderr, "Error - pthread_create() return code: %d\n", retCode);
        exit(EXIT_FAILURE);
    }

    // Join all threads so main waits until the threads finish
    for (j = 0; j < NTHREADS; ++j) {
        if ((retCode = pthread_join(tid[j], (void*) &t_status[j]))) {
            fprintf(stderr, "Error - pthread_join() return code: %d\n", retCode);
            exit(EXIT_FAILURE);
        }
    }

    // Check the status returned by each thread
    for (h = 0; h < NTHREADS; ++h) {
        if (t_status[h]) {
            check_flag = 1;
            break;
        }
    }

    if (check_flag) {
        printf("Sudoku Puzzle is Invalid\n");
    } else {
        printf("Sudoku Puzzle is Valid\n");
    }

	// Free memory and close the file
	int k;
	for (k = 0; k < 9; ++k) {
		free(data[k]);
	}

	fclose(fp);

	return 0;
}

void *validateSubgrid(void *data) {
	int digit_check[10] = { 0 };
	parameters *params = (parameters*) data;
	int i, j;

	for (i = params->row; i < params->row + 3; ++i) {
		for (j = params->col; j < params->col + 3; ++j) {
			if (digit_check[params->sudoku_grid[i][j]] == 1) {
                // Invalid sudoku subgrid
				return (void*) - 1;
			}

			digit_check[params->sudoku_grid[i][j]] = 1;
		}
	}

    // Valid sudoku subgrid
	return (void*) 0;
}

void *validateRows(void *data) {
	int digit_check[10] = { 0 };
	parameters *params = (parameters*) data;
	int i, j;

	for (i = 0; i < 9; ++i) {
		for (j = 0; j < 9; ++j) {
			if (digit_check[params->sudoku_grid[i][j]] == 1) {
				return (void*) - 1; // Invalid sudoku rows
			}

			digit_check[params->sudoku_grid[i][j]] = 1;
		}

		// Reinitialize check array for next row
		memset(digit_check, 0, sizeof(int) *10);
	}

    // Valid sudoku rows
	return (void*) 0;
}


void *validateCols(void *data) {
	int digit_check[10] = { 0 };
	parameters *params = (parameters*) data;
	int i, j;

	for (i = 0; i < 9; ++i) {
		for (j = 0; j < 9; ++j) {
			if (digit_check[params->sudoku_grid[j][i]] == 1) {
                // Invalid sudoku columns
				return (void*) - 1;
			}

			digit_check[params->sudoku_grid[j][i]] = 1;
		}

		// Reinitialize check array for next column
		memset(digit_check, 0, sizeof(int) *10);
	}

    // Valid sudoku columns
	return (void*) 0;
}

int readSudokuGrid(int(*grid)[9], FILE *fp) {
    // Seek beginning of file
    fseek(fp, 0, SEEK_SET);

    char entry;
	int i = 0, j = 0;

    // Loop through digits in file
	while ((fread(&entry, 1, 1, fp)) > 0) {
        if (entry == '\n') continue;
        if (entry == ' ') continue;

        if (isdigit(entry)) {
            // Store integer representation
            grid[i][j] = entry - '0';
            ++j;
            if (j == 9) {
                j = 0;
                ++i;
            }
        } else {
            // A non-digit character was read
            return -1;
		}
	}

    // Successfully read sudoku grid from file
	return 0;
}
