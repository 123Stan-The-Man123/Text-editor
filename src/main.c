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

struct Line {
    char* buff;
    int len;
};

struct Node {
    struct Node* prev;
    struct Line buffer;
    struct Node* next;
};

void process_input(void);
void process_escape(int*, int*);
int enable_raw_mode(struct termios*);
void disable_raw_mode(struct termios*);

int main(void)
{
    struct termios orig_term;
    if (enable_raw_mode(&orig_term) == -1)
        return 1;

    process_input();

    disable_raw_mode(&orig_term);

    return 0;
}

void process_input(void)
{
    int cur_col = 0, cur_row = 0;
    for (int running = 1; running;) {
        char c = getc(stdin);
        switch (c) {
            case CTRL_Q:
                running = 0;
                break;
            case CTRL_K:
                printf(ERASE_LINE);
                break;
            case '\r':
                printf("\r\n");
                cur_row++;
                cur_col = 0;
                break;
            case ESC:
                process_escape(&cur_col, &cur_row);
                break;
            case BACK_SPACE:
                printf("\b \b");
                break;
            default:
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
