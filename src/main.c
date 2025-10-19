#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

// Cursor controls
#define RESET_CURSOR "\033[H"
#define CURSOR_UP "\033[1A"
#define CURSOR_DOWN "\033[1B"
#define CURSOR_RIGHT "\033[1C"
#define CURSOR_LEFT "\033[1D"

// Erasure codes
#define ERASE_LINE "\033[2K"
#define ERASE_SCREEN "\033[2J"

// ASCII codes
#define CTRL_K 11
#define CTRL_Q 17
#define ESC 27
#define BACK_SPACE 127

// Test codes (to be removed)
#define CTRL_T 20
#define CTRL_U 21

#define INITIAL_LINE_SIZE 25

struct Line {
    char* buff;
    int len;
    int capacity;
};

struct Node {
    struct Line buffer;
    struct Node* next;
};

int enable_raw_mode(struct termios*);
void disable_raw_mode(struct termios*);
struct Node* init_node(char*);
void free_node(struct Node*);
struct Node* add_node(struct Node*, char*, int);
struct Node* remove_node(struct Node*, int);
void free_buffer(struct Node*);
int count_lines(struct Node*);
struct Node* get_line(struct Node*, int);
int get_line_len(struct Node*, int);
void increase_line_capacity(struct Node*);
void shift_right(struct Node*, int);
void add_char(struct Node*, int*, char);
struct Node* process_input(struct Node*);
void process_escape(struct Node*, int*, int*, int, int, struct Node**);

int main(int argc, char **argv)
{
    struct Node* buffer;
    if (argc == 1)
        buffer = init_node("\0");
    else
        ; // TODO: Open file and load text into the linked list, or if the file does not exist, initialise empty linked list

    struct termios orig_term;
    if (enable_raw_mode(&orig_term) == -1)
        return 1;

    buffer = process_input(buffer);

    disable_raw_mode(&orig_term);

    free_buffer(buffer);

    return 0;
}

int enable_raw_mode(struct termios* orig_term)
{
    // Turns off the STDOUT buffer
    setbuf(stdout, NULL);
    printf(RESET_CURSOR ERASE_SCREEN);

    // Return an error if tcgetattr fails
    if (tcgetattr(STDIN_FILENO, orig_term) != 0) {
        perror("tcgetattr");
        return -1;
    }

    /*
    struct termios {
        tcflag_t        c_iflag;        input flags
        tcflag_t        c_oflag;        output flags
        tcflag_t        c_cflag;        control flags
        tcflag_t        c_lflag;        local flags
        cc_t            c_cc[NCCS];     control chars
        speed_t         c_ispeed;       input speed
        speed_t         c_ospeed;       output speed
    };
    */

    // Make a separate copy so the original settings are saved
    struct termios raw = *orig_term;
    
    // Enter raw mode
    cfmakeraw(&raw);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    return 0;
}

void disable_raw_mode(struct termios* orig_term)
{
    // Reset terminal to default settings
    tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_term);
    printf(ERASE_SCREEN RESET_CURSOR);
}

struct Node* init_node(char* s)
{
    struct Node* node = malloc(sizeof(struct Node));
    if (node == NULL) {
        perror("Malloc failed.");
        return NULL;
    }

    node->buffer.len = strlen(s);
    node->buffer.capacity = INITIAL_LINE_SIZE;
    while (node->buffer.capacity <= node->buffer.len) {
        node->buffer.capacity *= 2;
    }
    node->buffer.buff = malloc(sizeof(char) * node->buffer.capacity);
    if (node->buffer.buff == NULL) {
        perror("Malloc failed.");
        free(node);
        return NULL;
    }
    strcpy(node->buffer.buff, s);

    node->next = NULL;
    return node;
}

void free_node(struct Node* node)
{
    free(node->buffer.buff);
    free(node);
}

struct Node* add_node(struct Node* node, char* s, int location)
{
    int i = 0;
    while (node->next != NULL && i < location) {
        node = node->next;
        i++;
    }
    if (i == 0 && node->next != NULL) {
        struct Node* temp = node;
        node = init_node(s);
        node->next = temp;
    } else if (i == location && node->next != NULL) {
        struct Node* temp = node->next;
        node->next = init_node(s);
        node = node->next;
        node->next = temp;
    } else if (node->next == NULL) {
        node->next = init_node(s);
        node = node->next;
    }
    return node;
}

struct Node* remove_node(struct Node* node, int location)
{
    int i = 0;
    struct Node* prev = node;
    while (node->next != NULL && i < location) {
        prev = node;
        node = node->next;
        i++;
    }
    if (node->next == NULL && i == 0) {
        node->buffer.buff[0] = '\0';
        node->buffer.len = 0;
    } else if (location == 0 && node->next != NULL) {
        node = node->next;
        free_node(prev);
    } else if (node->next == NULL && i > 0) {
        prev->next = NULL;
        free_node(node);
        node = prev;
    } else if (i > 0) {
        prev->next = node->next;
        free_node(node);
        node = prev;
    }
    return node;
}

void free_buffer(struct Node* buffer)
{
    struct Node* temp;

    while (buffer != NULL) {
        temp = buffer;
        buffer = buffer->next;
        free_node(temp);
    }
}

int count_lines(struct Node* buffer)
{
    int count = 0;

    while (buffer != NULL) {
        buffer = buffer->next;
        count++;
    }
    return count;
}

struct Node* get_line(struct Node* buffer, int line_number)
{
    for (int i = 0; i < line_number; i++)
        buffer = buffer->next;
    return buffer;
}

int get_line_len(struct Node* buffer, int cur_row)
{
    for (int i = 0; i < cur_row; i++)
        buffer = buffer->next;
    return buffer->buffer.len;
}

void increase_line_capacity(struct Node* line)
{
    line->buffer.capacity *= 2;
    line->buffer.buff = realloc(line->buffer.buff, sizeof(char) * line->buffer.capacity);
}

void shift_right(struct Node* line, int cur_col)
{
    for (int i = line->buffer.len; i >= cur_col; i--) {
        line->buffer.buff[i+1] = line->buffer.buff[i];
    }
}

void add_char(struct Node* line, int* cur_col, char c)
{
    if ((*cur_col + 1) >= line->buffer.capacity)
        increase_line_capacity(line);

    if (*cur_col == line->buffer.len) {
        printf("test");
        line->buffer.buff[(*cur_col)++] = c;
        line->buffer.buff[*cur_col] = '\0';
    } else {
        shift_right(line, *cur_col);
        line->buffer.buff[(*cur_col)++] = c;
    }

    line->buffer.len++;
}

struct Node* process_input(struct Node* buffer)
{
    int cur_col = 0, cur_row = 0;
    struct Node* cur_line = get_line(buffer, cur_row);
    // Test variable (to be removed)
    struct Node* temp;

    for (int running = 1; running;) {
        char c = getc(stdin);
        int line_count = count_lines(buffer);
        int line_len = get_line_len(buffer, cur_row);
        switch (c) {
            case CTRL_Q:
                running = 0;
                break;
            case CTRL_K:
                // TODO: Add buffer logic to remove a line from the linked list
                printf(ERASE_LINE);
                break;
            /*case CTRL_T:
                add_node(buffer, "Test is really long in order to see if the capacity changes or not.", 1);
                temp = buffer;
                while (temp) {
                    printf("%s\r\n%i\r\n", temp->buffer.buff, temp->buffer.capacity);
                    temp = temp->next;
                }
                printf("\r\n");
                break;
            case CTRL_U:
                remove_node(buffer, 1);
                temp = buffer;
                while (temp) {
                    printf("%s\r\n%i\r\n", temp->buffer.buff, temp->buffer.capacity);
                    temp = temp->next;
                }
                printf("\r\n");
                break;*/
            case '\r':
                // TODO: Add buffer logic to split line on a newline
                printf("\r\n");
                cur_line = add_node(buffer, "\0", ++cur_row);
                cur_col = 0;
                break;
            case ESC:
                process_escape(buffer, &cur_col, &cur_row, line_count, line_len, &cur_line);
                break;
            case BACK_SPACE:
                // TODO: Add buffer logic to synchronise pointer location
                printf("\b \b");
                cur_col -= (cur_col > 0) ? 1 : 0;
                break;
            default:
                // TODO: Add buffer logic to store characters
                add_char(cur_line, &cur_col, c);
                printf(ERASE_SCREEN RESET_CURSOR);
                printf("%s", cur_line->buffer.buff);
                break;
        }
    }
    return buffer;
}

void process_escape(struct Node* buffer, int* cur_col, int* cur_row, int line_count, int line_len, struct Node** cur_line)
{
    char c = getc(stdin);
    if (c != '[') {
        ungetc(c, stdin);
        return ;
    }

    // TODO: Add buffer logic to synchronise pointer location
    switch (getc(stdin)) {
        case 'A':
            if (*cur_row > 0) {
                printf(CURSOR_UP);
                (*cur_row)--;
                *cur_line = get_line(buffer, *cur_row);
            }
            break;
        case 'B':
            if (*cur_row < line_count-1) {
                printf(CURSOR_DOWN);
                (*cur_row)++;
                *cur_line = get_line(buffer, *cur_row);
            }
            break;
        case 'C':
            if (*cur_col < line_len-1) {
                printf(CURSOR_RIGHT);
                (*cur_col)++;
            }
            break;
        case 'D':
            printf(CURSOR_LEFT);
            *cur_col -= (*cur_col > 0) ? 1 : 0;
            break;
    }
}
