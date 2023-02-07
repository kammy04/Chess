#include <stdio.h>
<<<<<<< HEAD
<<<<<<< HEAD


// ♔ ♕ ♖ ♗ ♘ ♙
<<<<<<< HEAD
void showBoard (int board[8][9]) {

=======
#include <stdbool.h>
<<<<<<< HEAD
>>>>>>> origin/gridlines

// ♔ ♕ ♖ ♗ ♘ ♙

/* ┌ └ ┐ ┘

├ ┤ ┬ ┴
 
─ │ ┼
 
 might need these...
 
 ╟ ╢ ╧ ╤
 
*/
void showBoard (int board[8][8]) {
<<<<<<< HEAD
	printf("\n\n╔════════╗\n");
	for (int i = 0; i < 8; i++) {
		printf("║"); 
=======
void showBoard (int board[8][8]) {
	printf("\n\n  01234567\n");
	for (int i = 0; i < 8; i++) {
		printf("%d ", i); 
>>>>>>> coordinates
		for (int j = 0; j < 8; j++) {
			if (board[i][j] == 0) {
				if ((i + j) % 2 == 0) {
=======
	printf("\n\n┌─┬─┬─┬─┬─┬─┬─┬─┐\n");
	for (int i = 0; i < 8; i++) {
//		printf("│"); 
		bool flag = true;
		for (int j = 0; j < 8; j++) {
			if (!flag) {
				printf("│"); 
			}
			flag = false;
//			printf(""); 
			switch (board[i][j]) {
				case 0: 
>>>>>>> origin/gridlines
					printf(" ");
				} else {
					printf("▒");
				}
			} else if (board[i][j] == 1) {
				printf("♔");
 			} else if (board[i][j] == 2) {
				printf("♕");
			} else if (board[i][j] == 3) {
				printf("♗");
			} else if (board[i][j] == 4) {
				printf("♘");
			} else if (board[i][j] == 5) {
				printf("♖");
			} else if (board[i][j] == 6) {
				printf("♙");
			}
		}
<<<<<<< HEAD
		printf("\n");
	}
	printf("╚════════╝\n");
=======
		printf("│\n") ;
		if (i < 7) {
			printf("├─┼─┼─┼─┼─┼─┼─┼─┤\n") ;
		}
	}
	printf("└─┴─┴─┴─┴─┴─┴─┴─┘\n");
>>>>>>> origin/gridlines
}
=======
#include <stdbool.h>
#include <stdlib.h>

bool isValidMove (int x1, int x2, int y1, int y2, int board[8][8]) ;
>>>>>>> origin/Spennys_branch
=======
#include <stdlib.h>

bool isValidMove (int x1, int x2, int y1, int y2, int board[8][8]) ;

int main () {
	printf("Welcome to Terminal Chess!\n");
	printf("Initializing Board...\n")	;
	int board [8][8] = { {5, 4, 3, 1, 2, 3, 4, 5}
		           , {6, 6, 6, 6, 6, 6, 6, 6}
				 	     , {0, 0, 0, 0, 0, 0, 0, 0}
					     , {0, 0, 0, 0, 0, 0, 0, 0}
				             , {0, 0, 0, 0, 0, 0, 0, 0} 
				             , {0, 0, 0, 0, 0, 0, 0, 0}
				             , {6, 6, 6, 6, 6, 6, 6, 6}
				             , {5, 4, 3, 1, 2, 3, 4, 5}
				             } ;
>>>>>>> origin/king
  char buf[100];
  bool whitesMove = true;
	printf("Board Initialized!\n") ;
// 	showBoard(board);

<<<<<<< HEAD
  printf("Enter `q` to quit at any time.\n");
  printf("Moves are entered as co-ordinate pairs, such as \"13-33\" \n");
  do {
    if (whitesMove) {
      printf("White to move.\n");
    } else {
      printf("Black to move.\n");
    }
    printf("♔ >> ");
    scanf("%s", buf);
  } while (buf[0] != 'q' && buf[0] != 'Q') ;
	printf("Terminating...") ;
=======
int main () {
	printf("Welcome to Terminal Chess!\n");
	printf("Initializing Board...")	;
<<<<<<< HEAD
	int board [8][8]  = { {5, 4, 3, 1, 2, 3, 4, 5}
 				        , {6, 6, 6, 6, 6, 6, 6, 6}
 				 	    , {0, 0, 0, 0, 0, 0, 0, 0}
 					    , {0, 0, 0, 0, 0, 0, 0, 0}
 				        , {0, 0, 0, 0, 0, 0, 0, 0} 
 				        , {0, 0, 0, 0, 0, 0, 0, 0}
 				        , {6, 6, 6, 6, 6, 6, 6, 6}
				        , {5, 4, 3, 1, 2, 3, 4, 5}
				        } ;
=======
	int board [8][8] = { {5, 4, 3, 2, 1, 3, 4, 5}
				       , {6, 6, 6, 6, 6, 6, 6, 6}
				 	   , {0, 0, 0, 0, 0, 0, 0, 0}
					   , {0, 0, 0, 0, 0, 0, 0, 0}
				       , {0, 0, 0, 0, 0, 0, 0, 0} 
				       , {0, 0, 0, 0, 0, 0, 0, 0}
				       , {6, 6, 6, 6, 6, 6, 6, 6}
				       , {5, 4, 3, 2, 1, 3, 4, 5}
				       } ;
>>>>>>> origin/gridlines
	printf("Board Initialized!") ;
	
	showBoard(board);
	
	// rest of game...
	
	printf("Terminating...");
	
>>>>>>> coordinates
}

bool isValidMove (int x1, int x2, int y1, int y2, int board[8][8]) {
	int xcheck = x1;
	int ycheck = y1;
	int piecesInWay = 0;
	if (board[x1][x2] == 0) {
		return false;
<<<<<<< HEAD
	} else if (board[x1][y2] == 1) { // king
		if (abs(x1 - x2) <= 1 && abs(y1-y2) <= 1) {
			return true;
		}
	} else if (board[x1][y2] == 2) { 
=======
	} else if (board[x1][x2] == 1) { 
		if (abs(x1 - x2) <= 1 && abs(y1-y2) <= 1) {
			return true;
		}
		// king
	} else if (board[x1][x2] == 2) { 
>>>>>>> origin/king
		// queen
	} else if (board[x1][y2] == 3) {  // bishop
		while (true) {
			xcheck += (x1 > x2)? -1 : 1;
			ycheck += (y1 > y2)? -1 : 1;
		 	if (xcheck < 0 || ycheck < 0 || xcheck > 7 || ycheck > 7) {
				break;
			}
			if (board[xcheck][ycheck] != 0) {
				piecesInWay += 1;
			} 
			if (piecesInWay == 2) {
				break;
			}
			if (xcheck == x2 && ycheck == y2) {
				return true;
			}
		}
	} else if (board[x1][y2] == 4) { 
		// knight
	} else if (board[x1][y2] == 5) { // Rook
		while (true) {
			if (y1 == y2) {
				xcheck += (x1 > x2)? -1 : 1;
			} else if (x1 == x2) {
				ycheck += (y1 > y2)? -1 : 1;
			} else {
				return false;
			}
			
		 	if (xcheck < 0 || ycheck < 0 || xcheck > 7 || ycheck > 7) {
				break;
			}
			if (board[xcheck][ycheck] != 0) {
				piecesInWay += 1;
			} 
			if (piecesInWay == 2) {
				break;
			}
			if (xcheck == x2 && ycheck == y2) {
				return true;
			}
		}
	} else if (board[x1][y2] == 6) { // Pawn
		//printf("Pawn Selected\n");
		//printf("Moving in column %d \n", y1);
		//printf("y1-y2=%d\n", x1-x2);
		if (x1 - x2 == 1 && y1 == y2) { // forward one
			printf("Detected forward move by 1\n");
			if (board[x2][y2] == 0) {
				return true;
			}
		} else if (x1 - x2 == 2 && y1 == y2) { // forward two
//			printf("Detected forward move by 2\n");
			if (board[x2][y2] == 0 && board[x2+1][y2] == 0 && x1 == 6) {
				return true;
			}
		} else if (abs(y1 - y2) == 1 && x1-x2 == 1) { // capture left or right
			//printf("Detected Capture\n");
			if (board[x2][y2] != 0) {
				return true;
			}
		}
	}
	return false;
}

