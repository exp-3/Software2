#include <stdio.h>
#include <string.h>

#define BUFSIZE 1000

int main()
{
  char buf[BUFSIZE];
  FILE *fp;
  const char *filename = "input.txt";
  
  if ((fp = fopen(filename, "r")) == NULL) {
    printf("error: can't open %s\n", filename);
    return 1;
  }

  for(int i = 0; i < 100; i++) {
      fscanf(fp, "%s", buf);
      printf("%s\n", buf);
  }
  fclose(fp);
}
