#include <stdlib.h>
#include <stdio.h>
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
struct Node* init_buffer(void);
void free_buffer(struct Node*);
void process_input(struct Node*);
void process_escape(int*, int*);

int main(int argc, char **argv)
{
    struct Node* buffer;
    if (argc == 1)
        buffer = init_buffer();
    else
        ; // TODO: Open file and load text into the linked list, or if the file does not exist, initialise empty linked list

    struct termios orig_term;
    if (enable_raw_mode(&orig_term) == -1)
        return 1;

    process_input(buffer);

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

struct Node* init_buffer(void)
{
    struct Node* node = malloc(sizeof(struct Node));
    if (node == NULL) {
        perror("Malloc failed.");
        return NULL;
    }
    node->buffer.buff = malloc(sizeof(char) * INITIAL_LINE_SIZE);
    if (node->buffer.buff == NULL) {
        perror("Malloc failed.");
        free(node);
        return NULL;
    }
    node->buffer.len = 0;
    node->buffer.capacity = INITIAL_LINE_SIZE;
    node->next = NULL;
    return node;
}

void free_buffer(struct Node* buffer)
{
    struct Node* temp;

    while (buffer != NULL) {
        temp = buffer;
        buffer = buffer->next;
        free(temp->buffer.buff);
        free(temp);
    }
}

void process_input(struct Node* buffer)
{
    int cur_col = 0, cur_row = 0;
    for (int running = 1; running;) {
        char c = getc(stdin);
        switch (c) {
            case CTRL_Q:
                running = 0;
                break;
            case CTRL_K:
                // TODO: Add buffer logic to remove a line from the linked list
                printf(ERASE_LINE);
                break;
            case '\r':
                // TODO: Add buffer logic to split line on a newline
                printf("\r\n");
                cur_row++;
                cur_col = 0;
                break;
            case ESC:
                process_escape(&cur_col, &cur_row);
                break;
            case BACK_SPACE:
                // TODO: Add buffer logic to synchronise pointer location
                printf("\b \b");
                cur_col -= (cur_col > 0) ? 1 : 0;
                break;
            default:
                // TODO: Add buffer logic to store characters
                printf("%c", c);
                break;
        }
    }
}

void process_escape(int* cur_col, int* cur_row)
{
    char c = getc(stdin);
    if (c != '[') {
        ungetc(c, stdin);
        return ;
    }

    // TODO: Add buffer logic to synchronise pointer location
    switch (getc(stdin)) {
        case 'A':
            printf(CURSOR_UP);
            *cur_row -= (*cur_row > 0) ? 1 : 0;
            break;
        case 'B':
            printf(CURSOR_DOWN);
            (*cur_row)++;
            break;
        case 'C':
            printf(CURSOR_RIGHT);
            (*cur_col)++;
            break;
        case 'D':
            printf(CURSOR_LEFT);
            *cur_col -= (*cur_col > 0) ? 1 : 0;
            break;
    }
}
