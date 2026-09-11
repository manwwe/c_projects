#include <stdio.h>

int main(void) {
  printf("\x1b[2J"); // Clear the screen.
  while (1) {
    printf("#\n");
    printf("\x1b[H"); // Move to the top-left corner.
    printf("#\n");
    printf("\x1b[3D");
    printf("@\n");
  }

  return 0;
}
