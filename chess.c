#include <stdio.h>


// ♔ ♕ ♖ ♗ ♘ ♙
void showBoard (int board[8][9]) {


// ♔ ♕ ♖ ♗ ♘ ♙

/* ┌ └ ┐ ┘

├ ┤ ┬ ┴
 
─ │ ┼
 
 might need these...
 
 ╟ ╢ ╧ ╤
 
*/
void showBoard (int board[8][8]) {
	printf("\n\n╔════════╗\n");
	for (int i = 0; i < 8; i++) {
		printf("║"); 
		for (int j = 0; j < 8; j++) {
			if (board[i][j] == 0) {
				if ((i + j) % 2 == 0) {
					printf(" ");
				} else {
					printf("▒");
				}
			} else if (board[i][j] == 1) {
				printf("♔");
 			} else if (board[i][j] == 2) {
				printf("♕");
			} else if (board[i][j] == 3) {
				printf("♖");
			} else if (board[i][j] == 4) {
				printf("♗");
			} else if (board[i][j] == 5) {
				printf("♘");
			} else if (board[i][j] == 6) {
				printf("♙");
			}
		}
		printf("\n");
	}
	printf("╚════════╝\n");
}

int main () {
	printf("Welcome to Terminal Chess!\n");
	printf("Initializing Board...")	;
	int board [8][8] = { {5, 4, 3, 1, 2, 3, 4, 5}
			       , {6, 6, 6, 6, 6, 6, 6, 6}
				 	   , {0, 0, 0, 0, 0, 0, 0, 0}
					   , {0, 0, 0, 0, 0, 0, 0, 0}
				       , {0, 0, 0, 0, 0, 0, 0, 0} 
				       , {0, 0, 0, 0, 0, 0, 0, 0}
				       , {6, 6, 6, 6, 6, 6, 6, 6}
				       , {5, 4, 3, 1, 2, 3, 4, 5}
				       } ;
	printf("Board Initialized!");
	printf("Terminating...");
  char buf[100];
  bool whitesMove = true;
	printf("Board Initialized!\n") ;
// 	showBoard(board);

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
}
