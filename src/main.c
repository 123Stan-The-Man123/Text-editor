#include <stdio.h>
#include <termios.h>
#include <unistd.h>

// Cursor controls
#define RESET_CURSOR "\033[H"
#define CURSOR_UP "\033[1A"
#define CURSOR_DOWN "\033[1B"
#define CURSOR_RIGHT "\033[1C"
#define CURSOR_LEFT "\033[1D"

// Erasures
#define ERASE_LINE "\033[2K"
#define ERASE_SCREEN "\033[2J"

// ASCII codes
#define CTRL_K 11
#define CTRL_Q 17
#define ESC 27
#define BACK_SPACE 127

// Handle arrow keys
void handle_arrows(void);

int main(void)
{
    // Turns off the STDOUT buffer
    setbuf(stdout, NULL);
    printf(RESET_CURSOR ERASE_SCREEN);
    
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

    // Return an error if tcgetattr fails
    if (tcgetattr(STDIN_FILENO, &orig_term) != 0) {
        perror("tcgetattr");
        return -1;
    }

    // Make a separate copy so the original settings are saved
    raw = orig_term;
    
    // Enter raw mode
    cfmakeraw(&raw);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    
    // Event loop
    for (;;) {
        char c = getchar();
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
                handle_arrows();
                break;
            case BACK_SPACE:
                printf("\b \b");
                break;
            default:
                printf("%c", c);
                break;
        }
    }
    
    // Reset terminal to default settings
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
    return 0;
}

void handle_arrows(void)
{
    getchar();
    switch (getchar()) {
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
