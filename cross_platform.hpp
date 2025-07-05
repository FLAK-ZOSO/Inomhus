#include <cstdio>
#ifdef _WIN32
    #include <windows.h>
    #include <mmsystem.h>
    // #pragma comment(lib,"winmm.lib")
    #include <conio.h>

    void flushInput() {
        // Flush the input buffer (discard data not read yet)
        HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
        FlushConsoleInputBuffer(hInput);
    }
#elif __APPLE__
    #include <termios.h>
    #include <unistd.h>

    struct termios orig_termios;
    void term_echooff() {
        struct termios noecho;

        tcgetattr(0, &orig_termios);

        noecho = orig_termios;
        noecho.c_lflag &= ~(ECHO | ICANON);

        tcsetattr(0, TCSANOW, &noecho);
    }

    void flushInput() {
        // Flush stdin (discard data not read yet)
        tcflush(STDIN_FILENO, TCIFLUSH);
    }
#elif __linux__    
    #include <unistd.h>
    #include <termios.h>

    char getch(void) {
        char buf = 0;
        struct termios old = {0};
        fflush(stdout);
        if(tcgetattr(0, &old) < 0)
            perror("tcsetattr()");
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        if(tcsetattr(0, TCSANOW, &old) < 0)
            perror("tcsetattr ICANON");
        if(read(0, &buf, 1) < 0)
            perror("read()");
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        if(tcsetattr(0, TCSADRAIN, &old) < 0)
            perror("tcsetattr ~ICANON");
        // printf("%c\n", buf);
        return buf;
    }

    void flushInput() {
        // Flush stdin (discard data not read yet)
        tcflush(STDIN_FILENO, TCIFLUSH);
    }
#endif