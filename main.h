#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <deque>

struct Pos { 
    int x, y;
    Pos();

    Pos(int _x, int _y);

    bool operator== (Pos other);
};

class Eatable {

    public:
        Pos pos;

        Eatable();

        Eatable(int init_x, int init_y);
};

class Apple;
class Bomb;
class Snake;

class Apple : public Eatable {

    public:
        Apple(winsize w, Snake & snake, std::vector<Apple> & apples, std::vector<Bomb> & bombs);

};

class Bomb : public Eatable {
    ;
};

typedef enum{up, down, left, right} Direction;

class Snake {

    public:
        size_t len;
        Direction d;
        Pos head;
        std::deque<Pos> track;

        Snake(winsize w, int size);

        Pos nextpos();

        bool move(winsize w, std::vector<Apple> & apples, std::vector<Bomb> & bombs);

};
