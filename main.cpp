#include "main.h"
#include "cmdline.h"

#include <iostream>
#include <fstream>
#include <random>
#include <thread>
#include <chrono>

#include <curses.h>

std::vector<Pos> allPos;
bool debug_mode = false;
int init_len;
int speed;
char last_input;
bool isrunning = true;

std::ofstream logfile;

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


Apple::Apple(Pos pos)
    :Eatable(pos.x, pos.y) {
    ;
}

Snake::Snake(int size = 3) {

    if (COLS / 2 <= 15 || LINES <= 11) {
        throw "Too small terminal window QAQ";
    }

    len = size;
    d = left;
    head = Pos(COLS / 4, LINES / 2);

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

std::vector<Pos> Snake::getFreePos(const std::vector<Apple> apples, const std::vector<Bomb> bombs) {
    std::vector<Pos> freePos;
    for (Pos p: allPos) {

        bool isOccupied = false;
        for (Pos _p: track) {
            if (p == _p) {
                isOccupied = true;
                break;
            }
        }

        if (!isOccupied) {
            for (Apple a: apples) {
                Pos _p = a.pos;
                if (p == _p) {
                    isOccupied = true;
                    break;
                }
            }
        }

        if (!isOccupied) {
            for (Bomb b: bombs) {
                Pos _p = b.pos;
                if (p == _p) {
                    isOccupied = true;
                }
            }
        }

        if (!isOccupied) {
            freePos.emplace_back(p);
        }
    }

    return freePos;
}

bool Snake::move(std::vector<Apple> & apples, std::vector<Bomb> & bombs, Pos & tail)  {
    
    Pos nexthead = nextpos();
    if (nexthead.x < 0 || nexthead.y < 0 ||
        nexthead.x >= COLS/2 || nexthead.y >= LINES) {  // Out of boundary
            return false;
    }

    for (auto p: track) {
        if (nexthead.x == p.x && nexthead.y == p.y) {  // Crash into itself
            return false;
        }
    }

    for (auto b : bombs) {
        if (nexthead.x == b.pos.x && nexthead.y == b.pos.y) {  // Crash into a bomb
            return false;
        }
    }

    Pos newPos;
    for (auto a = apples.begin(); a != apples.end(); ++a) {
        if (nexthead.x == a->pos.x && nexthead.y == a->pos.y) {
            head = nexthead;
            len++;

            int newApples = 2;
            
            newPos = newFreePos(apples, bombs);
            
            track.push_back(head);

            apples.erase(a);
            apples.emplace_back(newPos);
            tail.x = -1; tail.y = -1;
            return true;
        }
    }

    head = nexthead;
    tail = track.at(0);
    track.pop_front();
    track.push_back(head);
    return true;
}

Pos Snake::newFreePos(std::vector<Apple> & apples, std::vector<Bomb> & bombs) {
    std::vector<Pos> freePos = getFreePos(apples, bombs);
    Pos newPos = freePos[random() % freePos.size()];

    if (debug_mode) {

        logfile << "Apple" << std::endl;
        for (Apple a: apples) {
            logfile << a.pos.x << "," << a.pos.y << " ";
        }

        logfile << "\nBomb" << std::endl;
        for (Bomb b: bombs) {
            logfile << b.pos.x << "," << b.pos.y << " ";
        }

        logfile << "\nSnake" << std::endl;
        for (Pos p: track) {
            logfile << p.x << "," << p.y << " ";
        }
        logfile << std::endl;

        logfile << "Free" << std::endl;
        for (Pos p: freePos) {
            logfile << p.x << "," << p.y << " ";
        }
        logfile << std::endl;

        logfile << "selected" << std::endl;
        logfile << newPos.x << "," << newPos.y << std::endl;
    }
    return newPos;
}

void initscreen(const Snake snake, const std::vector<Apple> apples, const std::vector<Bomb> bombs) {

    clear();

    mvprintw(0, 0, "SCORE: %d", (snake.len - init_len) * speed);

    for (Pos p: snake.track) {
        if (p == snake.head) {
            switch (snake.d) {
                case up:    mvprintw(p.y, p.x*2, "^"); break;
                case down:  mvprintw(p.y, p.x*2, "v"); break;
                case left:  mvprintw(p.y, p.x*2, "<"); break;
                case right: mvprintw(p.y, p.x*2, ">"); break;
            }
        } else {
            mvprintw(p.y, p.x*2, "*");
        }
    }

    for (Apple a : apples) {
        Pos p = a.pos;
        mvprintw(p.y, p.x*2, "@");
    }

    for (Bomb b : bombs) {
        Pos p = b.pos;
        mvprintw(p.y, p.x*2, "X");
    }

    refresh();
}

void updatescreen(const Snake snake, const std::vector<Apple> apples, const std::vector<Bomb> bombs, const Pos tail) {

    if (tail.x == -1 and tail.y == -1) {  // Apple eaten, tail not disappear
        mvprintw(0, 0, "SCORE: %d", (snake.len - init_len) * speed);
        for (auto a: apples) {
            Pos p = a.pos;
            mvaddch(p.y, 2*p.x, '@');
        }

        for (auto b: bombs) {
            Pos p = b.pos;
            mvaddch(p.y, 2*p.x, 'X');
        }

    } else {
        mvaddch(tail.y, 2*tail.x, ' ');
    }

    Pos p = snake.head;
    switch (snake.d) {
            case up:    
                mvaddch(p.y, p.x*2, '^'); 
                mvaddch(p.y+1, p.x*2, '*');
                break;
            case down:  
                mvaddch(p.y, p.x*2, 'v'); 
                mvaddch(p.y-1, p.x*2, '*');
                break;
            case left:  
                mvaddch(p.y, p.x*2, '<'); 
                mvaddch(p.y, p.x*2+2, '*');
                break;
            case right: 
                mvaddch(p.y, p.x*2, '>'); 
                mvaddch(p.y, p.x*2-2, '*');
                break;
    }
    
    refresh();

}

void printscreen(const Snake snake, const std::vector<Apple> apples, const std::vector<Bomb> bombs) {
    
    // clear();

    mvprintw(0, 0, "SCORE: %d", (snake.len - init_len) * speed);

    for (Pos p: snake.track) {
        if (p == snake.head) {
            switch (snake.d) {
                case up:    mvaddch(p.y, p.x*2, '^'); break;
                case down:  mvaddch(p.y, p.x*2, 'v'); break;
                case left:  mvaddch(p.y, p.x*2, '<'); break;
                case right: mvaddch(p.y, p.x*2, '>'); break;
            }
        } else {
            mvaddch(p.y, p.x*2, '*');
        }
    }

    for (Apple a : apples) {
        Pos p = a.pos;
        mvaddch(p.y, p.x*2, '@');
    }

    for (Bomb b : bombs) {
        Pos p = b.pos;
        mvaddch(p.y, p.x*2, 'X');
    }

    refresh();
}

void threadGetChar() {
    while(isrunning) {

        char buffer = 0;
        buffer = getch(); 

        if (buffer != 0 && buffer != EOF) {
            last_input = buffer;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main(int argc, char *argv[]) {

    // initialize parser
    cmdline::parser parser;
    parser.add<int>("length", 'l', "length of the snake", false, 3, cmdline::range(3, 10));
    parser.add<int>("speed", 's', "frame per second, the moving speed", false, 5, cmdline::range(2, 50));
    parser.add("vim", 'v', "vim mode");
    parser.add("debug", 'd', "debug mode");

    parser.add("help", 'h', "print this message");

    parser.parse_check(argc, argv);

    speed = parser.get<int>("speed");
    debug_mode = parser.exist("debug");

    int interval = 1000 / speed;
    bool isVimMode = parser.exist("vim");

    winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);  // get current windowsize 

    LINES = w.ws_row;
    COLS  = w.ws_col;

    if (debug_mode) {
        logfile.open("./log.log");
        logfile << LINES << " " << COLS << std::endl;
    }

    for (int y = 0; y < LINES; ++y) {
        for (int x = 0; x < COLS/2; ++x) {
            allPos.emplace_back(x, y);
        }
    }

Begin:

    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* local_time = std::localtime(&now_time);
    srand(local_time->tm_year * 365 * 24 * 60 * 60 +
          local_time->tm_yday * 24 * 60 * 60 +
          local_time->tm_hour * 60 * 60 +
          local_time->tm_min * 60 +
          local_time->tm_sec);
    
    initscr();
    noecho();  // do not show input chars
    curs_set(0);  // hide cursor
    // nodelay(stdscr, true);
    leaveok(stdscr, true);

    clear();
    refresh();

    mvprintw(LINES/2-4, (COLS-8)/2, "SNAKE IT");

    if (isVimMode) {
        mvprintw(LINES/2-2, (COLS-12)/2, "HJKL TO MOVE");
    } else {
        mvprintw(LINES/2-2, (COLS-12)/2, "WASD TO MOVE");
    }

    mvprintw(LINES/2, (COLS-7)/2, "@ APPLE"); 
    mvprintw(LINES/2+1, (COLS-7)/2, "X  BOMB");
    mvprintw(LINES/2+2, (COLS-9)/2, "<**** YOU"); 

    mvprintw(LINES/2+4, (COLS-11)/2, "PRESS ENTER");

    refresh(); 

    getch();
    cbreak();  // read chars immediately

    clear();
    refresh();

    std::vector<Apple> apples;
    std::vector<Bomb> bombs;

    init_len = parser.get<int>("length");

    Snake snake(init_len);
    
    Pos newPos = snake.newFreePos(apples, bombs);
    apples.emplace_back(newPos);

    bool res;
    std::thread inputThread(threadGetChar);
    initscreen(snake, apples, bombs);

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
        Pos tail;
        res = snake.move(apples, bombs, tail);
        updatescreen(snake, apples, bombs, tail);
        
        if (!res) {

            clear();
            isrunning = false;
            
            mvprintw(LINES/2-2, (COLS-9)/2, "GAME OVER");
            mvprintw(LINES/2-1, (COLS-5)/2, "SCORE");
            mvprintw(LINES/2, (COLS)/2, "%d", (snake.len-init_len) * speed);
            inputThread.join();

            refresh();

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            flushinp();  // clear the buffer
            
            mvprintw(LINES/2+1, (COLS-18)/2, "PRESS R TO RESTART");
            mvprintw(LINES/2+2, (COLS-19)/2, "PRESS ENTER TO QUIT");

            
            refresh();

            auto c = getch();
            if (c == 'r') {
                round = 0;
                isrunning = true;
                curD = left;
                last_input = 'a';
                goto Begin;
            }
            endwin();
            break;
        }
        round++;
    }
}