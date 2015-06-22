# Zen Puzzle Garden Solver

There is a video game called Zen Puzzle Garden. This program generates solutions for this game. 

This program is not finished. With grids larger than even 6x6, it uses enough
memory to fail. I need to make this more efficient, but it works for smaller
grids. It uses a brute force method, recursively investigating every possible
path. But if a solution is encountered during this search, it prints the
solution and ends. Improvements necessary: Backtracking could be improved. Right
now it will backtrack from every position on a path, even when they all lead to
the same dead end. This is obviously stupid. If I could backtrack groups of
blocks at a time, I'd save a ton of memory and time.
