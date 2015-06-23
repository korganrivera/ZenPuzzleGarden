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

    2015.6.22.11:03         Been messing with this all day.  So it turns out I had a +
                            where I should have had a - on one of my monk movements.
                            Now my program works fine, even for large grids, in less
                            than 5 seconds. :) So that was fucking annoying.  Done!

    2015.6.22.11:49         Well....malloc still fails when the grid is big enough.  I don't know how to fix that.
                            Maybe it's possible to get into an infinite loop with no backtracking.  I should check for that.
                            Also later, print the grid in func and use cls, watch what happens.

    2015.6.23.14:34         I added an update. I noticed that when backtracking from a dead-end
                            down a one-way path, that it would still head in the direction toward the
                            deadend. I put in flags that tell the monk whether or not the only available
                            direction is valid or not. If it isn't, just keep heading back until there's
                            more than one option. Also made sure the 'gliding over sand' condition sets
                            another flag to prevent the second movement check from repeating the same
                            direction. Malloc still fails with some maps, but it's getting better.
                            ALso, it needs a way to find shorter solutions. He runs in circles a lot. -_-
*/

#include <stdio.h>
#include <stdlib.h>

#define SANDROWS 9      //  These two values are the only ones you need to set.  If there are objects in the sand, do that in main().
#define SANDCOLS 7

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

unsigned lreverse=0;                                                         //  trying to flag when the monk can backtrack all the way up a pointless path.
unsigned rreverse=0;
unsigned ureverse=0;
unsigned dreverse=0;

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
    grid[5][6] = 1;
    grid[5][7] = 1;
    //grid[][3] = 1;
    //grid[6][4] = 1;
    //grid[8][10] = 1;
    //grid[8][11] = 1;

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
    char open;
    char upflag, downflag, leftflag, rightflag;

    upflag=downflag=rightflag=leftflag=0;

    //  for lazy debug, finding memory leaks. Cheap graph of allocations. shows number of grid copies.
    if(DEBUG){
        for(i=0; i<malloccallcount/100; i++) putchar('|');
        putchar('\n');
    }
    //  for debug, finding memory leaks. Cheap graph of allocations. displays number of bytes.
    //if(DEBUG){
    //    for(i=0; i<mallocedbytes/1024/1024; i++) putchar('|');
    //    putchar('\n');
    //}

    //if(DEBUG){
    //    printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
    //    if(mallocedbytes/1024>=1024) printf("%.2f mb used", (float)mallocedbytes/1024.0/1024.0);
    //    else printf("%u kb used", mallocedbytes/1024);
    //}

if(DEBUG){
    system("cls");
    //  display entire grid.
    puts("Here is the grid:");
    for(i=0; i<bigrows*bigcols; i++) {
        if(i/bigcols==x && i%bigcols==y) printf("x");
        else if(!(i%bigcols)) printf("%c", 178);
        else if(i%bigcols == bigcols-1) printf("%c", 178);
        else if(i/bigcols == bigrows-1) printf("%c", 178);
        else if(!(i/bigcols)) printf("%c", 178);

        else if(*(grid+i)==1) printf("1");
        else if(*(grid+i)==0) printf("0");
        else printf("\nfailfailfail");
        if(i%bigcols==bigcols-1) putchar('\n');
    }
    getchar();
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


    /*  After hitting a deadend with a straight path to it, there's no point in the monk backtracking
        to any position on that straight path, because he'll just keep heading toward the deadend.
        So, when he hits a deadend, it sets the reverse flag.  While the reverse flag is on and there's
        only one direction available, just return/backtrack.
        There is a reverse flag for each direction.
    */

    //  check if current position is deadend or open.
    open=0;
    if(!*(grid+x*bigcols+y-1) || !*(grid+x*bigcols+y+1) || !*(grid+x*bigcols+y-bigcols) || !*(grid+x*bigcols+y+bigcols)) open=1;

    //  if leftreverse is set and any other direction is open, set leftreverse to 0. do for all directions.
    if(lreverse && ( !*(grid+x*bigcols+y+1) || !*(grid+x*bigcols+y-bigcols) || !*(grid+x*bigcols+y+bigcols)))   lreverse=0;
    if(rreverse && ( !*(grid+x*bigcols+y-1) || !*(grid+x*bigcols+y-bigcols) || !*(grid+x*bigcols+y+bigcols)))   rreverse=0;
    if(ureverse && ( !*(grid+x*bigcols+y+1) || !*(grid+x*bigcols+y-1)       || !*(grid+x*bigcols+y+bigcols)))   ureverse=0;
    if(dreverse && ( !*(grid+x*bigcols+y+1) || !*(grid+x*bigcols+y-bigcols) || !*(grid+x*bigcols+y-1)))         dreverse=0;

    //  if only up is available, but upreverse flag is set, don't go up, just return.
    if(*(grid+x*bigcols+y-1)==0 && lreverse) return;
    if(*(grid+x*bigcols+y+1)==0 && rreverse) return;
    if(*(grid+x*bigcols+y-bigcols)==0 && ureverse) return;
    if(*(grid+x*bigcols+y+bigcols)==0 && dreverse) return;

    //  if at deadend, set the reverse flag associated with the last direction made. (don't go in this direction again.)
    if(!open) {
            if(*(stack+stacknext-1)==UP){ ureverse=1; return; }
            if(*(stack+stacknext-1)==DOWN){ dreverse=1; return; }
            if(*(stack+stacknext-1)==LEFT){ lreverse=1; return; }
            if(*(stack+stacknext-1)==RIGHT){ rreverse=1; return; }
    }


    /*  if you're on sand, you have to keep moving in that direction until you hit something or are on path again.
        The following block of code does that.
    */

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
                rightflag=1;
                if(stacknext==stacksize){ puts("\nstack overflow"); endprogram(1); }
                func(x, y+1, mygrid, stacknext+1);
            }
            //  if last move was left, move left.
            else if(*(stack+stacknext-1)==LEFT){
                *(stack+stacknext) = LEFT;
                leftflag=1;
                if(stacknext==stacksize){ puts("\nstack overflow"); endprogram(1); }
                func(x, y-1, mygrid, stacknext+1);
            }
            //  if last move was up, move up.
            else if(*(stack+stacknext-1)==UP){
                *(stack+stacknext) = UP;
                upflag=1;
                if(stacknext==stacksize){ puts("\nstack overflow"); endprogram(1); }
                func(x-1, y, mygrid, stacknext+1);
            }
            //  if last move was down, move down.
            else if(*(stack+stacknext-1)==DOWN){
                *(stack+stacknext) = DOWN;
                downflag=1;
                if(stacknext==stacksize){ puts("\nstack overflow"); endprogram(1); }
                func(x+1, y, mygrid, stacknext+1);
            }
    }

    //  check if puzzle is solved.
    if(onpath && sanddone){
        //  puzzle is solved.  print solution.
        puts("\npuzzle is complete.  printing solution.");
        for(i=0; i<stacknext; i++){
            if(*(stack+i) == UP) printf("UP\t");
            else if(*(stack+i) == DOWN) printf("DOWN\t");
            else if(*(stack+i) == LEFT) printf("LEFT\t");
            else if(*(stack+i) == RIGHT) printf("RIGHT\t");

            //  try lazily to print solution in a grid.
            if(!(stacknext%7)){       if(!((i+1)%7)) putchar('\n'); }
            else if(!(stacknext%5)){  if(!((i+1)%5)) putchar('\n'); }
            else if(!(stacknext%3)){  if(!((i+1)%3)) putchar('\n'); }
            //else if(!(stacknext%2)){  if(!((i+1)%2)) putchar('\n'); }
            else if(!((i+1)%10)) putchar('\n');
        }
        printf("\nmoves: %u\n", stacknext);
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

    //  if monk can move right, do it.
    if(!(*(mygrid+x*bigcols+y+1)) && !rightflag){
        *(stack+stacknext) = RIGHT;
        if(stacknext==stacksize){ puts("\nstack overflow"); endprogram(1); }
        func(x, y+1, mygrid, stacknext+1);
    }
    //  if monk can move left, do it.
    if(!(*(mygrid+x*bigcols+y-1)) && !leftflag){
        *(stack+stacknext) = LEFT;
        if(stacknext==stacksize){ puts("\nstack overflow"); endprogram(1); }
        func(x, y-1, mygrid, stacknext+1);
    }
    //  if monk can move up, do it.
    if(!(*(mygrid+x*bigcols+y-bigcols)) && !upflag){
        *(stack+stacknext) = UP;
        if(stacknext==stacksize){ puts("\nstack overflow"); endprogram(1); }
        func(x-1, y, mygrid, stacknext+1);
    }
    //  if monk can move down, do it.
    if(!(*(mygrid+x*bigcols+y+bigcols)) && !downflag){
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
