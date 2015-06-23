/*
    Zen Puzzle Garden solver.

    2015.6.21~11am.         I think it works but malloc() fails with larger grids. I wonder if there's a way
                            to only pass the current position and surrounding positions, instead of the
                            entire grid. Or something else, I dunno. Save memory somehow. I changed stack
                            from unsigned to char, which quadrupled potential, but still wasn't big enough.

    Ok there's a problem:   one of the rules of Zen Puzzle Garden is that once you start moving in a
                            direction across the sand, you have to keep moving until you either hit
                            something, or you are on the path again. This is good because it could save me a
                            ton of memory, but it also means I have to rewrite a bunch of stuff, and I've
                            been doing this for about 12 hours now. O_O

    2015.6.22.12:58         Okay it works now, but it's hella slow. It's working on a grid
                            right now that's 5x5 and I've been waiting about 10 minutes. I incorporated that
                            rule that you have to keep travelling in the same direction if you're on sand
                            and not blocked, so that's cool. Also fixed a memory leak which was causing
                            those crashes I mentioned. Always use free before a return, kids!

    2015.6.22.15:01         Still crashes with grids as small as 4x4.  Fix later.

    2015.6.22.11:03         Been messing with this all day.  So it turns out I had a + where I should have had a - on one of my
                            monk movements.  Now my program works fine, even for large grids, in less than 5 seconds. :)
                            So that was fucking annoying.  Done!
*/

#include <stdio.h>
#include <stdlib.h>

#define SANDROWS 10      //  These two values are the only ones you need to set.  If there are objects in the sand, do that in main().
#define SANDCOLS 10

#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4

#define UNKNOWN 0
#define SAND 1
#define PATH 2

#define DEBUG 0                                                             //  if set to 1, debug stuff will be shown.

#define pathrows (SANDROWS+2)                                               //  height of grid with the path.
#define pathcols (SANDCOLS+2)                                               //  width of grid with the path.
#define bigrows  (SANDROWS+4)                                               //  height of entire grid.
#define bigcols (SANDCOLS+4)                                                //  width of entire grid.
#define bigsize (bigrows*bigcols)                                           //  number of cells in entire grid.
#define stacksize (SANDROWS*SANDROWS*SANDROWS+2*SANDROWS*SANDCOLS)          //  This is max size of stack.
#define mylimit (bigsize-bigcols-1)                                         //  last grid index that is sand or path.  Used for loops.

char *stack;                                                                //  used to store moves.  Replace later with nifty bit tricks.

void func(unsigned x, unsigned y, char *grid, unsigned stacknext);          //  recursive function for grid searching. Does all the work.
char celltype(unsigned i);                                                  //  tells if a cell is sand or path.
void resetpath(unsigned stacknext, unsigned x, unsigned y, char *grid);     //  checks if path needs to be reset, does it if so.
void endprogram(unsigned condition);                                        //  exits the program but also beeps if I want it to.

unsigned malloccallcount=0;                                                 //  tells me number of malloc calls have been made and freed.  Used to check for memory leaks.
unsigned mallocedbytes=0;                                                   //  Use for debug, tells me how much mem has been allocated. Used to check for memory leaks.

int main(void){
    unsigned i,j;
    char grid[bigrows][bigcols];

    //  initialise grid. ones on the edge, zeroes elsewhere.
    for(i=0; i<bigrows*bigcols; i++) grid[i/bigcols][i%bigcols] = !(i%bigcols) || (i%bigcols==bigcols-1) || !(i/bigcols) || (i/bigcols==bigrows-1);

    //  malloc space for stack.  Need bigrows*bigcols chars.
    if((stack = malloc(stacksize)) == NULL){ puts("\nmain(): malloc failed."); endprogram(1); }
    malloccallcount++;
    mallocedbytes+=stacksize;

    //  initialise the stack with zeroes.
    for(i=0; i<stacksize; i++) *(stack+i) = 0;

    //  put objects in place, if any;
    //grid[2][3] = 1;
    //grid[2][4] = 1;
    //grid[3][4] = 1;
    //grid[4][3] = 1;
    grid[2][2] = 1;

    //  display entire grid.
    puts("Here is the grid:");
    for(i=0; i<bigrows*bigcols; i++) {
        if(i/bigcols==bigrows/2 && i%bigcols==1) printf("x");
        else if(!(i%bigcols)) printf("%c", 178);
        else if(i%bigcols == bigcols-1) printf("%c", 178);
        else if(i/bigcols == bigrows-1) printf("%c", 178);
        else if(!(i/bigcols)) printf("%c", 178);
        else if(grid[i/bigcols][i%bigcols]==1) printf("1");
        else if(grid[i/bigcols][i%bigcols]==0) printf("0");
        else printf("\nfailfailfail");
        if(i%bigcols==bigcols-1) putchar('\n');
    }

    //  character's starting position is about half-way down left edge.
    printf("\nstarting position: %u, 0\nworking...\n", bigrows/2-1);
    func(bigrows/2,1, &grid[0][0], 0);
    free(stack);
    puts("\nno solution :(");
}


void func(unsigned x, unsigned y, char *grid, unsigned stacknext){
    unsigned onpath, sanddone, i, lastposition, lastpositiontype;
    char *mygrid;

    //  for lazy debug, finding memory leaks. Cheap graph of allocations. displays number of calls.
    //for(i=0; i<malloccallcount/10; i++) putchar('|');
    //putchar('\n');

    //  for debug, finding memory leaks. Cheap graph of allocations. displays number of bytes.
    //if(DEBUG){
    //    for(i=0; i<mallocedbytes/1024; i++) putchar('|');
    //    putchar('\n');
    // }

    if(DEBUG){
        printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
        if(mallocedbytes/1024>=1024) printf("%.2f mb used", (float)mallocedbytes/1024.0/1024.0);
        else printf("%u kb used", mallocedbytes/1024);
    }

    //  Check if monk is on the path or not.
    if(celltype(x*bigcols+y)==PATH) onpath=1;
    else onpath=0;

    //check if all sand blocks have been visited.
    sanddone=1;
    for(i=bigcols+1; i<mylimit; i++){                                                           //  loop from first path element to last path element, sequentially.
        if(!(i%bigcols)) continue;                                                              //  if in column zero, skip it. This is a dead edge.
        if(i%bigcols == bigcols-1) continue;                                                    //  if on far right side, skip it.  This is a dead edge.
        if(celltype(i)==SAND && !(*(grid+i))){ sanddone=0; break; }                             //  if cell is sand and it hasn't been visited, sanddone=0 and break.
    }

    //  if you're on sand, you have to keep moving in that direction until you hit something or are on path again.
    //  The following block of code does that.

    //  if on sand and I can still go in last direction.
    if(!onpath &&  (*(stack+stacknext-1)==RIGHT && !(*(grid+x*bigcols+y+1)))       || \
                   (*(stack+stacknext-1)==LEFT  && !(*(grid+x*bigcols+y-1)))       || \
                   (*(stack+stacknext-1)==UP    && !(*(grid+x*bigcols+y-bigcols))) || \
                   (*(stack+stacknext-1)==DOWN  && !(*(grid+x*bigcols+y+bigcols)))) {
            //  malloc space for copy of grid.
            if((mygrid = malloc(bigsize)) == NULL){ puts("\nfunc():mygrid:malloc failed."); endprogram(1); }
            malloccallcount++;
            mallocedbytes+=bigsize;

            //  copy the grid.
            for(i=0; i<bigsize; i++) *(mygrid+i) = *(grid+i);

            //  reset the path if necessary.
            if(stacknext>=3 && !onpath) resetpath(stacknext, x,y, mygrid);

            //  mark current position as visited.
            *(mygrid+x*bigcols+y) = 1;


            //  if last move was right, move right.
            if(*(stack+stacknext-1)==RIGHT){
                *(stack+stacknext) = RIGHT;
                if(stacknext==stacksize){ puts("\nstack overflow"); endprogram(1); }
                func(x, y+1, mygrid, stacknext+1);
            }
            //  if last move was left, move left.
            else if(*(stack+stacknext-1)==LEFT){
                *(stack+stacknext) = LEFT;
                if(stacknext==stacksize){ puts("\nstack overflow"); endprogram(1); }
                func(x, y-1, mygrid, stacknext+1);
            }
            //  if last move was up, move up.
            else if(*(stack+stacknext-1)==UP){
                *(stack+stacknext) = UP;
                if(stacknext==stacksize){ puts("\nstack overflow"); endprogram(1); }
                func(x-1, y, mygrid, stacknext+1);
            }
            //  if last move was down, move down.
            else if(*(stack+stacknext-1)==DOWN){
                *(stack+stacknext) = DOWN;
                if(stacknext==stacksize){ puts("\nstack overflow"); endprogram(1); }
                func(x+1, y, mygrid, stacknext+1);
            }
    }

    //  check if puzzle is solved.
    if(onpath && sanddone){
        //  puzzle is solved.  print solution.
        puts("\npuzzle is complete.  printing solution.");
        printf("\nmoves: %u\n", stacknext);
        for(i=0; i<stacknext; i++){
            if(*(stack+i) == UP) printf("UP\t");
            else if(*(stack+i) == DOWN) printf("DOWN\t");
            else if(*(stack+i) == LEFT) printf("LEFT\t");
            else if(*(stack+i) == RIGHT) printf("RIGHT\t");

            //  try lazily to print solution in a grid.
            if(!(stacknext%7)){       if(!((i+1)%7)) putchar('\n'); }
            else if(!(stacknext%5)){  if(!((i+1)%5)) putchar('\n'); }
            else if(!(stacknext%3)){  if(!((i+1)%3)) putchar('\n'); }
            else if(!(stacknext%2)){  if(!((i+1)%2)) putchar('\n'); }
            else if(!((i+1)%10)) putchar('\n');
        }
        endprogram(0);
    }

    //  malloc space for copy of grid.
    if((mygrid = malloc(bigsize)) == NULL){ puts("\nfunc():mygrid:malloc failed."); endprogram(1); }
    malloccallcount++;
    mallocedbytes+=bigsize;

    //  copy the grid.
    for(i=0; i<bigsize; i++) *(mygrid+i) = *(grid+i);

    //  reset path if necessary
    if(stacknext>=3 && !onpath) resetpath(stacknext, x,y, mygrid);

    //  mark current position as visited.
    *(mygrid+x*bigcols+y) = 1;

    //  check if at deadend. if so, return.
    if(*(mygrid+x*bigcols+y-bigcols) && *(mygrid+x*bigcols+y+bigcols) && *(mygrid+x*bigcols+y+1) && *(mygrid+x*bigcols+y-1)){
        free(mygrid);
        malloccallcount--;
        mallocedbytes-=bigsize;
        return;
    }

    //  if monk can move right, do it until you hit something or are back on path.
    if(!(*(mygrid+x*bigcols+y+1))){
        *(stack+stacknext) = RIGHT;
        if(stacknext==stacksize){ puts("\nstack overflow"); endprogram(1); }
        func(x, y+1, mygrid, stacknext+1);
    }
    //  if monk can move left, do it.
    if(!(*(mygrid+x*bigcols+y-1))){
        *(stack+stacknext) = LEFT;
        if(stacknext==stacksize){ puts("\nstack overflow"); endprogram(1); }
        func(x, y-1, mygrid, stacknext+1);
    }
    //  if monk can move up, do it.
    if(!(*(mygrid+x*bigcols+y-bigcols))){
        *(stack+stacknext) = UP;
        if(stacknext==stacksize){ puts("\nstack overflow"); endprogram(1); }
        func(x-1, y, mygrid, stacknext+1);
    }
    //  if monk can move down, do it.
    if(!(*(mygrid+x*bigcols+y+bigcols))){
        *(stack+stacknext) = DOWN;
        if(stacknext==stacksize){ puts("\nstack overflow"); endprogram(1); }
        func(x+1, y, mygrid, stacknext+1);
    }

    free(mygrid);
    malloccallcount--;
    mallocedbytes-=bigsize;
}

//  tells if cell i is sand or path.
char celltype(unsigned i){
    if((i%bigcols==1 || i%bigcols==bigcols-2) || (i/bigcols==1 || i/bigcols==bigrows-2)) return PATH;
    else return SAND;
}


void resetpath(unsigned stacknext, unsigned x, unsigned y, char *grid){
    char lastpositiontype=UNKNOWN;
    unsigned lastposition, i;

    if(*(stack+stacknext-1)==UP) lastposition = x*bigcols+y+bigcols;
    else if(*(stack+stacknext-1)==DOWN) lastposition = x*bigcols+y-bigcols;
    else if(*(stack+stacknext-1)==LEFT) lastposition = x*bigcols+y+1;
    else if(*(stack+stacknext-1)==RIGHT) lastposition = x*bigcols+y-1;
    else printf("\nlastposition is fucked.");

    if(celltype(lastposition)==SAND){
        for(i=bigcols+1; i<mylimit; i++){
            if(!(i%bigcols)) continue;
            if(i%bigcols == bigcols-1) continue;
            if(celltype(i)==PATH) *(grid+i) = 0;
        }
    }
}


void endprogram(unsigned condition){
    putchar('\a');      //  bell.
    exit(condition);
}
