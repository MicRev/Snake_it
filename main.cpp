#include "main.h"
#include "cmdline.h"

#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <termios.h>


Pos::Pos() {
    x = -1;
    y = -1;
}

Pos::Pos(int _x, int _y) {
    x = _x;
    y = _y;
}

bool Pos::operator== (Pos other) {
    return x == other.x && y == other.y;
}


Eatable::Eatable()
    : pos() {
        ;
}

Eatable::Eatable(int init_x, int init_y)
    : pos(init_x, init_y) {
        ;
}


Apple::Apple(winsize w, Snake & snake, std::vector<Apple> & apples, std::vector<Bomb> & bombs)
    :Eatable() {
    do {
        int x = random() % w.ws_col;
        int y = random() % w.ws_row;

        if (y == w.ws_row-1) {
            continue;
        }

        for (Pos p : snake.track) {
            if (x == p.x && y == p.y) {
                continue;
            }
        }

        if (!apples.empty()) {
            for (Apple a : apples) {
                if (x == a.pos.x && y == a.pos.y) {
                    continue;
                }
            }
        }

        if (!bombs.empty()) {
            for (Bomb b: bombs) {
                if (x == b.pos.x && y == b.pos.y) {
                    continue;
                }
            }
        }

        pos.x = x;
        pos.y = y;
        break;
    } while(1);
}

Snake::Snake(winsize w, int size = 3) {

    if (w.ws_col <= 15 || w.ws_row <= 11) {
        throw "Too small terminal window QAQ";
    }

    len = size;
    d = left;
    head = Pos(w.ws_col / 2, w.ws_row / 2);

    for (int i = len-1; i >=0; --i) {
        track.emplace_back(head.x+i, head.y);
    }
    
}

Pos Snake::nextpos() {
    switch (d) {
        case up:
            return Pos(head.x, head.y-1);
        case down:
            return Pos(head.x, head.y+1);
        case left:
            return Pos(head.x-1, head.y);
        case right:
            return Pos(head.x+1, head.y);
        default:
            return head;
    }
}

bool Snake::move(winsize w, std::vector<Apple> & apples, std::vector<Bomb> & bombs)  {
    Pos nexthead = nextpos();
    if (nexthead.x < 0 || nexthead.y < 0 ||
        nexthead.x >= w.ws_col || nexthead.y >= w.ws_row) {
            return false;
    }

    for (auto p: track) {
        if (nexthead.x == p.x && nexthead.y == p.y) {
            return false;
        }
    }

    for (auto b : bombs) {
        if (nexthead.x == b.pos.x && nexthead.y == b.pos.y) {
            return false;
        }
    }

    for (auto a = apples.begin(); a != apples.end(); ++a) {
        if (nexthead.x == a->pos.x && nexthead.y == a->pos.y) {
            head = nexthead;
            len++;
            apples.erase(a);
            apples.emplace_back(w, *this, apples, bombs);
            track.push_back(head);
            return true;
        }
    }

    head = nexthead;
    track.pop_front();
    track.push_back(head);
    return true;
}


void printscreen(winsize w, const Snake snake, const std::vector<Apple> apples, const std::vector<Bomb> bombs) {
    
    for (int y = 0; y < w.ws_row; ++y) {
        for (int x = 0; x < w.ws_col; ++x) {

            bool isSpecialBlock = false;

            for (Pos p : snake.track) {
                if (x == p.x && y == p.y) {
                    if (p == snake.head) {
                        switch (snake.d) {
                            case up:    putchar('^'); break;
                            case down:  putchar('v'); break;
                            case left:  putchar('<'); break;
                            case right: putchar('>'); break;
                        }
                    } else {
                        putchar('*');
                    }
                    isSpecialBlock = true;
                    continue;
                }
            }

            for (auto a : apples) {
                if (x == a.pos.x && y == a.pos.y) {
                    isSpecialBlock = true;
                    putchar('@');
                    continue;
                }
            }

            for (auto b: bombs) {
                if (x == b.pos.x && y == b.pos.y) {
                    isSpecialBlock = true;
                    putchar('X');
                    continue;
                }
            }
            if (!isSpecialBlock) {
                putchar(' ');
            }
        }
    }
}

void clearScreen() {
#ifdef __linux__
    system("clear");
#elif __WIN32
    system("cls");
#endif
}

char last_input;
bool isrunning = true;

void threadGetChar() {
    while(isrunning) {
        termios oldattr;
        tcgetattr(0, &oldattr);

        termios newattr = oldattr;
        newattr.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(0, TCSANOW, &newattr);

        char buffer = 0;
        buffer = getchar();  
        tcsetattr(0, TCSANOW, &oldattr);
        
        if (buffer != 0 && buffer != EOF) {
            last_input = buffer;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main(int argc, char *argv[]) {

    cmdline::parser parser;
    parser.add<int>("length", 'l', "length of the snake", false, 3, cmdline::range(3, 10));
    parser.add<int>("speed", 's', "frame per second, the moving speed", false, 5, cmdline::range(2, 50));
    parser.add("vim", 'v', "vim mode");

    parser.add("help", 'h', "print this message");

    parser.parse_check(argc, argv);

    int interval = 1000 / parser.get<int>("speed");
    bool isVimMode = parser.exist("vim");

    winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);  // get the current terminal size

    std::cout.flush();
    clearScreen();
    std::cout.flush();
    auto INIT_SCREEN = [w, isVimMode](){
        for (int i = 0; i < w.ws_row/4; ++i) {
            for (int j = 0; j < w.ws_col; ++j) {
                putchar(' ');
            }
        }

        for (int j = 0; j < (w.ws_col-8)/2; ++j) {
            putchar(' ');
        }

        printf("SNAKE IT");

        for (int j = 0; j < w.ws_col - (w.ws_col-8)/2 - 8; ++j) {
            putchar(' ');
        }

        for (int j = 0; j < w.ws_col; ++j) {
            putchar(' ');
        }

        if (isVimMode) {

            for (int j = 0; j < (w.ws_col-12)/2; ++j) {
                putchar(' ');
            }
            
            printf("HJKLto move");

            for (int j = 0; j < w.ws_col - (w.ws_col-12)/2 - 12; ++j) {
                putchar(' ');
            }

            for (int j = 0; j < w.ws_col; ++j) {
                putchar(' ');
            }

        } else {

            for (int j = 0; j < (w.ws_col-12)/2; ++j) {
                putchar(' ');
            }
            
            printf("WASD to move");

            for (int j = 0; j < w.ws_col - (w.ws_col-12)/2 - 12; ++j) {
                putchar(' ');
            }

            for (int j = 0; j < w.ws_col; ++j) {
                putchar(' ');
            }

        }

        for (int j = 0; j < (w.ws_col-7)/2; ++j) {
            putchar(' ');
        }

        printf("@ APPLE");

        for (int j = 0; j < w.ws_col - (w.ws_col-7)/2 - 7; ++j) {
            putchar(' ');
        }

        for (int j = 0; j < (w.ws_col-7)/2; ++j) {
            putchar(' ');
        }

        printf("X  BOMB");

        for (int j = 0; j < w.ws_col - (w.ws_col-7)/2 - 7; ++j) {
            putchar(' ');
        }

        for (int j = 0; j < (w.ws_col-9)/2; ++j) {
            putchar(' ');
        }

        printf("<**** YOU");

        for (int j = 0; j < w.ws_col - (w.ws_col-9)/2 - 9; ++j) {
            putchar(' ');
        }

        for (int j = 0; j < w.ws_col; ++j) {
            putchar(' ');
        }

        printf("PRESS ENTER");

        for (int j = 0; j < w.ws_col-11; ++j) {
            putchar(' ');
        }

        for (int i = 0; i < w.ws_row - w.ws_row/4 - 9; ++i) {
            for (int j = 0; j < w.ws_col; ++j) {
                putchar(' ');
            }
        }
    };
    INIT_SCREEN();
    getchar();

    std::cout.flush();
    clearScreen();
    std::cout.flush();

    std::vector<Apple> apples;
    std::vector<Bomb> bombs;
    Snake snake(w, parser.get<int>("length"));
    
    apples.emplace_back(w, snake, apples, bombs);

    bool res;
    bool islocked;
    std::thread inputThread(threadGetChar);
    printscreen(w, snake, apples, bombs);

    int round = 0;
    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        Direction curD = snake.d;
        if (isVimMode) {
            switch (last_input) {
                case 'k': curD = up;    break;
                case 'j': curD = down;  break;
                case 'h': curD = left;  break;
                case 'l': curD = right; break;
                default: break;
            }
        } else {
            switch (last_input) {
                case 'w': curD = up;    break;
                case 's': curD = down;  break;
                case 'a': curD = left;  break;
                case 'd': curD = right; break;
                default: break;
            }
        }
        if (!((curD == up && snake.d == down) || 
              (curD == down && snake.d == up) ||
              (curD == left && snake.d == right) ||
              (curD == right && snake.d == left))) {
                snake.d = curD;
        }
        std::cout.flush();
        clearScreen();
        std::cout.flush();
        res = snake.move(w, apples, bombs);
        printscreen(w, snake, apples, bombs);
        round++;
        
        if (!res) {
            std::cout.flush();
            clearScreen();
            std::cout.flush();

            isrunning = false;
            for (int i = 0; i < w.ws_row/2; ++i) {
                for (int j = 0; j < w.ws_col; ++j) {
                    putchar(' ');
                }
            }

            for (int i = 0; i < (w.ws_col-9)/2; ++i) {
                putchar(' ');
            }

            printf("GAME OVER!\n");

            for (int i = 0; i < w.ws_col - (w.ws_col-9)/2 - 9; ++i) {
                putchar(' ');
            }

            for (int i = 0; i < w.ws_row-w.ws_row/2-1; ++i) {
                for (int j = 0; j < w.ws_col; ++j) {
                    putchar(' ');
                }
            }
            inputThread.join();
            break;
        }
    }
}