## Snake It!

A command line Snake game.

### To build

- libraries 
  - chrono
  - ncurses
  - cmdline (pure headfile ,already included in this repo)
- build and run with git and gcc

```bash
mkdir snake && cd snake
git clone https://github.com/MicRev/Snake_it
mkdir build && cd build
cmake ..
make -j8
./Snake <Options>
```

### Supporting platform

Linux, Win32, Win64

### Command Options

`-l, --length` 

length of the snake (int [=3])

`-s, --speed`     

frame per second, the moving speed (int [=5])

`-v, --vim`       

vim mode

`-h, --help`      

print this message

### Enjoy :>