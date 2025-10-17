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

int process_input(void);
void process_escape(void);
int enable_raw_mode(struct termios* orig_term, struct termios* raw);
void disable_raw_mode(struct termios* orig_term);

int main(void)
{
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
    struct termios orig_term, raw;
    if (enable_raw_mode(&orig_term, &raw) == -1)
        return -1;

    for (int running = 1; running;)
        running = process_input();

    disable_raw_mode(&orig_term);

    return 0;
}

int process_input(void)
{
    char c = getc(stdin);
    switch (c) {
        case CTRL_Q:
            return 0;
        case CTRL_K:
            printf(ERASE_LINE);
            break;
        case '\r':
            printf("\r\n");
            break;
        case ESC:
            process_escape();
            break;
        case BACK_SPACE:
            printf("\b \b");
            break;
        default:
            printf("%c", c);
            break;
    }

    return 1;
}

void process_escape(void)
{
    char c = getc(stdin);
    if (c != '[') {
        ungetc(c, stdin);
        return ;
    }

    switch (getc(stdin)) {
        case 'A':
            printf(CURSOR_UP);
            break;
        case 'B':
            printf(CURSOR_DOWN);
            break;
        case 'C':
            printf(CURSOR_RIGHT);
            break;
        case 'D':
            printf(CURSOR_LEFT);
            break;
    }
}

int enable_raw_mode(struct termios* orig_term, struct termios* raw)
{
    // Turns off the STDOUT buffer
    setbuf(stdout, NULL);
    printf(RESET_CURSOR ERASE_SCREEN);

    // Return an error if tcgetattr fails
    if (tcgetattr(STDIN_FILENO, orig_term) != 0) {
        perror("tcgetattr");
        return -1;
    }
    // Make a separate copy so the original settings are saved
    *raw = *orig_term;
    
    // Enter raw mode
    cfmakeraw(raw);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, raw);

    return 0;
}

void disable_raw_mode(struct termios* orig_term)
{
    // Reset terminal to default settings
    tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_term);
    printf(ERASE_SCREEN RESET_CURSOR);
}
