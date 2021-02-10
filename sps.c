#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const size_t init_rows = 1;
const size_t init_cols = 1;

const int min_argc = 3;
const int max_argc = 5;
const size_t string_init_capacity = 20;
const int range_max = -1;
const size_t max_increment = 500;


//const char *command_strings[] = {
//        "irow", "arow", "drown", "icol", "acol", ""
//};


const char *error_messages[] = {
    "",
    "",
    "ERROR: memory allocation failed!\n",
    "ERROR: invalid number of arguments!\n",
    "ERROR: invalid format of delimiters!\n",
    "ERROR: unable to open the input file!\n",
};


typedef enum {
    R_OK,
    R_EOF,
    R_ERR_MEM,
    R_ERR_ARGC,
    R_ERR_DELIM,
    R_ERR_FILE,
} RetCode;


typedef struct {
    char *content;
    size_t size;
    size_t capacity;
} String;


// ========================================
// ARGUMENTS STRUCTS
// ========================================


typedef enum {
    IROW,
    AROW,
    DROW,
    ICOL,
    ACOL,
    DCOL,
    SET,
    CLEAR,
    SWAP,
    SUM,
    AVG,
    COUNT,
    LEN,
    MIN,
    MAX,
    FIND,
    SELECT,
    UNKNOWN
} CommandType;


typedef struct {
    int row_from;
    int row_to;
    int col_from;
    int col_to;
} CommandRange;


typedef struct cmd {
    CommandType cmd_type;
    CommandRange cmd_range;
    String cmd_str;
    struct cmd *next;
} Command;


typedef struct {
    char *delimiters;
    char *file_name;
    Command *cmd_list;
} Arguments;


// ========================================
// TABLE STRUCTS
// ========================================


typedef enum {
    T_STRING,
    T_STRING_QUOTED,
    T_INT,
    T_FLOAT,
} CellContentType;


typedef struct {
    String content;
    CellContentType cell_type;
} TableCell;


typedef struct {
    TableCell **cells;
    size_t capacity;
} TableRow;


typedef struct {
    TableRow **rows;
    size_t row_count;
    size_t col_count;
    size_t capacity;
} Table;


// ==========================================
// FUNCTION PROTOTYPES
// ==========================================


String string_ctor(size_t initial_capacity, RetCode *code);
void string_dtor(String str);
void string_clear(String *str);

CommandRange range_ctor(int row_from, int row_to, int col_from, int col_to);

Command *command_ctor();
void command_dtor(Command *cmd);

Arguments arguments_ctor();
void arguments_dtor(Arguments arg);
RetCode parse_arguments(Arguments *arg, int argc, char *argv[]);

RetCode parse_file(Arguments *arg);
RetCode read_line(String *str, FILE *fp);

RetCode realloc_mem(void **mem_ptr, size_t *old_capacity, size_t elem_sizeof);


TableCell *cell_ctor();
void cell_dtor(TableCell *cell);

TableRow *row_ctor(size_t cell_count);
void row_dtor(TableRow *row);

Table *table_ctor();
void table_dtor(Table *table);
RetCode add_row(Table *table, size_t position);
RetCode add_col(Table *table, size_t position);
RetCode _add_cell(Table *table, size_t row, size_t col);


// ==========================================
// FUNCTION DEFINITIONS
// ==========================================


RetCode add_col(Table *table, size_t position) {
    RetCode code;

    // For each row, insert new cell at position
    for (size_t row = 0; row < table->row_count; row++) {
        code = _add_cell(table, row, position);
        if (code != R_OK) {
            return code;
        }
    }

    table->col_count++;
    return R_OK;
}


RetCode _add_cell(Table *table, size_t row, size_t col) {
    if (table->col_count + 1 > table->rows[row]->capacity) {
        // Realloc if needed
        RetCode code = realloc_mem((void **)&table->rows[row]->cells, &table->rows[row]->capacity, sizeof(TableCell *));
        if (code != R_OK) {
            return code;
        }

        // Initialize the new part of the array
        for (size_t i = table->col_count; i < table->rows[row]->capacity; i++) {
            table->rows[row]->cells[i] = NULL;
        }
    }

    // Move existing cells by one if needed
    for (size_t i = table->col_count; i >= col; i--) {
        table->rows[row]->cells[i] = table->rows[row]->cells[i - 1];
    }

    // Allocate new cell
    table->rows[row]->cells[col - 1] = cell_ctor();
    if (table->rows[row]->cells[col - 1] == NULL) {
        return R_ERR_MEM;
    }

    return R_OK;
}


RetCode add_row(Table *table, size_t position) {
    if (table->row_count + 1 > table->capacity) {
        // Realloc if needed
        RetCode code = realloc_mem((void **)&table->rows, &table->capacity, sizeof(TableRow *));
        if (code != R_OK) {
            return code;
        }

        // Initialize the new part of the array
        for (size_t row = table->row_count; row < table->capacity; row++) {
            table->rows[row] = NULL;
        }
    }

    // Move existing elements by one if needed
    for (size_t row = table->row_count; row >= position; row--) {
        table->rows[row] = table->rows[row - 1];
    }

    // Allocate new row
    table->rows[position - 1] = row_ctor(table->col_count);
    if (table->rows[position - 1] == NULL) {
        return R_ERR_MEM;
    }

    // Update row counter
    table->row_count++;
    return R_OK;
}


void table_dtor(Table *table) {
    // Initial check
    if (table == NULL) {
        return;
    }

    // Free array of row pointers
    if (table->rows != NULL) {
        for (size_t i = 0; i < table->row_count; i++) {
            row_dtor(table->rows[i]);
        }
        free(table->rows);
    }

    // Free the structure
    free(table);
}


Table *table_ctor() {
    // Allocate the structure
    Table *new_table = malloc(sizeof(Table));
    if (new_table == NULL) {
        return NULL;
    }

    // Set the initial number of rows and cols
    new_table->row_count = 0;
    new_table->col_count = 0;
    new_table->capacity = init_rows;

    // Allocate enough memory for the rows
    new_table->rows = malloc(sizeof(TableRow *) * new_table->capacity);
    if (new_table->rows == NULL) {
        table_dtor(new_table);
        return NULL;
    }

    // Initialize the array
    for (size_t row = 0; row < new_table->capacity; row++) {
        new_table->rows[row] = NULL;
    }

    return new_table;
}


TableRow *row_ctor(size_t cell_count) {
    // Allocate the structure itself
    TableRow *new_row = malloc(sizeof(TableRow));
    if (new_row == NULL) {
        return NULL;
    }

    // Allocate enough space for the amount of cells
    new_row->capacity = (cell_count > init_cols) ? cell_count : init_cols;
    new_row->cells = malloc(sizeof(TableCell *) * new_row->capacity);
    if (new_row->cells == NULL) {
        row_dtor(new_row);
        return NULL;
    }
    // Initialize the array to NULL
    for (size_t col = 0; col < new_row->capacity; col++) {
        new_row->cells[col] = NULL;
    }

    // Allocate cells
    for (size_t i = 0; i < cell_count; i++) {
        new_row->cells[i] = cell_ctor();
        if (new_row->cells[i] == NULL) {
            row_dtor(new_row);
            return NULL;
        }
    }
    return new_row;
}


void row_dtor(TableRow *row) {
    // Initial check
    if (row == NULL) {
        return;
    }

    if (row->cells) {
        // Free cells
        for (size_t i = 0; i < row->capacity; i++) {
            cell_dtor(row->cells[i]);
        }
        // Free array of pointers
        free(row->cells);
    }

    // Free the structure
    free(row);
}


void cell_dtor(TableCell *cell) {
    // Initial Check
    if (cell == NULL) {
        return;
    }

    string_dtor(cell->content);
    free(cell);
}


TableCell *cell_ctor() {
    // First allocate a new TableCell structure
    TableCell *new_cell = malloc(sizeof(TableCell));
    if (new_cell == NULL) {
        return NULL;
    }

    // Set the default values and allocate the cell content
    new_cell->cell_type = T_STRING;
    RetCode code;
    new_cell->content = string_ctor(string_init_capacity, &code);
    if (code != R_OK) {
        cell_dtor(new_cell);
        return NULL;
    }

    return new_cell;
}


RetCode realloc_mem(void **mem_ptr, size_t *old_capacity, size_t elem_sizeof) {
    // Make sure that we allocate some memory
    if (*old_capacity == 0) {
        *old_capacity = 1;
    }

    // Compute new size in bytes
    size_t new_capacity = (*old_capacity <= max_increment) ? *old_capacity * 2 : *old_capacity + max_increment;

    // Reallocate
    void *tmp_ptr = realloc(*mem_ptr, new_capacity * elem_sizeof);
    if (tmp_ptr == NULL) {
        return R_ERR_MEM;
    }

    // Assign new pointer and capacity
    *mem_ptr = tmp_ptr;
    *old_capacity = new_capacity;
    return R_OK;
}


RetCode read_line(String *str, FILE *fp) {
    if (feof(fp)) {
        return R_EOF;
    }

    string_clear(str);
    do {
        // Reallocate more memory if needed
        if (str->capacity == 0 || str->size == str->capacity - 1) {
            RetCode code = realloc_mem((void **)&str->content, &str->capacity, sizeof(char));
            if (code != R_OK) {
                return code;
            }
        }

        // Read line or a part of it
        if(fgets(str->content + str->size, (int)(str->capacity - str->size), fp) == NULL) {
            if (str->size == 0) {
                return R_EOF;
            }
            break;
        }
        str->size = strlen(str->content);
    } while(str->size == str->capacity - 1 && str->content[str->size - 1] != '\n');

    return R_OK;
}


RetCode parse_file(Arguments *arg) {
    // Attempt to open the input file
    FILE *input = fopen(arg->file_name, "r");
    if (input == NULL) {
        return R_ERR_FILE;
    }

    // Prepare the line buffer
    RetCode code;
    String buffer = string_ctor(string_init_capacity, &code);
    if (code != R_OK) {
        fclose(input);
        return code;
    }

    // Read the file line by line
    size_t row = 1;
    while(read_line(&buffer, input) != R_EOF) {
        printf("Line %lu: %s", row, buffer.content);
        row++;
    }

    string_dtor(buffer);
    fclose(input);
    return R_OK;
}


void print_error(RetCode code) {
    fprintf(stderr, "%s", error_messages[code]);
}

RetCode parse_arguments(Arguments *arg, int argc, char *argv[]) {
    // Initial ARGC check
    if (argc != min_argc && argc != max_argc) {
        return R_ERR_ARGC;
    }

    int arg_iter = 1;

    // Parse [-d DELIM]
    if (argc == max_argc) {
        if (strcmp(argv[arg_iter], "-d") != 0) {
            return R_ERR_DELIM;
        }
        arg_iter++;
        arg->delimiters = argv[arg_iter];
        arg_iter++;
    } else {
        // Default delimiters: " "
        arg->delimiters = " ";
    }

    // TODO: Parse CMD_SEQ
    arg_iter++;
    arg->file_name = argv[arg_iter];

    return R_OK;
}


Arguments arguments_ctor() {
    Arguments arg = {.delimiters = NULL, .cmd_list = NULL, .file_name = NULL};
    return arg;
}


void arguments_dtor(Arguments arg) {
    // Only cmd_list can be dynamically allocated
    Command *cmd_tmp;
    while(arg.cmd_list != NULL) {
        cmd_tmp = arg.cmd_list->next;
        command_dtor(arg.cmd_list);
        arg.cmd_list = cmd_tmp;
    }
}


CommandRange range_ctor(int row_from, int row_to, int col_from, int col_to) {
    CommandRange r = {
            .row_from = row_from, .row_to = row_to,
            .col_from = col_from, .col_to = col_to
    };
    return r;
}


Command *command_ctor() {
    // Allocate memory for the Command struct
    Command *new_cmd = malloc(sizeof(Command));
    if (new_cmd == NULL) {
        return NULL;
    }

    // Initialize Command values
    new_cmd->cmd_type = UNKNOWN;
    new_cmd->cmd_range = range_ctor(range_max, range_max, range_max, range_max);
    new_cmd->next = NULL;

    // Initialize Command String
    RetCode code;
    new_cmd->cmd_str = string_ctor(string_init_capacity, &code);
    if (code != R_OK) {
        free(new_cmd);
        return NULL;
    }

    return new_cmd;
}


void command_dtor(Command *cmd) {
    if (cmd == NULL) {
        return;
    }

    // Free the Command String
    string_dtor(cmd->cmd_str);
    // Free the Command structure
    free(cmd);
}



String string_ctor(size_t initial_capacity, RetCode *code) {
    // Create a new string
    String new_str = {.content = NULL, .capacity = initial_capacity, .size = 0};

    // Allocate memory for the content
    if (new_str.capacity > 0) {
        new_str.content = malloc(initial_capacity);
        if (new_str.content == NULL) {
            *code = R_ERR_MEM;
            return new_str;
        }
        new_str.content[0] = '\0';
    }

    *code = R_OK;
    return new_str;
}


void string_dtor(String str) {
    if (str.content != NULL) {
        free(str.content);
    }
}


void string_clear(String *str) {
    if (str == NULL) {
        return;
    }

    str->size = 0;
    if (str->content != NULL && str->capacity > 0) {
        str->content[0] = '\0';
    }
}


TableCell *get_cell(Table *table, size_t row, size_t col) {
    if (row >= table->row_count || col >= table->col_count) {
        return NULL;
    }

    return table->rows[row]->cells[col];
}


void print_table(Table *table, char delimiter) {
    for (size_t row = 0; row < table->row_count; row++) {
        for (size_t col = 0; col < table->col_count; col++) {
            printf("%s", get_cell(table, row, col)->content.content);
            if (col < table->col_count - 1) {
                putchar(delimiter);
            }
        }
        printf("\n");
    }
}


int main(int argc, char *argv[]) {
    Table *table = table_ctor();
    add_row(table, 1);
    add_row(table, 2);
    add_row(table, 3);
    add_col(table, 1);
    add_col(table, 2);
    add_col(table, 3);

    print_table(table, ':');

    table_dtor(table);

    // 1. Parse arguments
//    Arguments arg = arguments_ctor();
//    RetCode code = parse_arguments(&arg, argc, argv);
//    if (code != R_OK) {
//        print_error(code);
//        arguments_dtor(arg);
//        return code;
//    }

    // 2. Create Table

    // 3. Read file into the Table
//    code = parse_file(&arg);
//    if (code != R_OK) {
//        print_error(code);
//        arguments_dtor(arg);
//        return code;
//    }

    // 4. Perform the command sequence

    // 5. Cleanup
//    arguments_dtor(arg);
    return R_OK;
}