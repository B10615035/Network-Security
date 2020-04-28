#include <dirent.h> 
#include <stdio.h> 

int main(void) {
  DIR *d;
  struct dirent *dir;
  char list[200]="list:\n";
  d = opendir("./ServerFile");
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if(strcmp(dir->d_name,"..")!=0&&strcmp(dir->d_name,".")!=0){
        strcat(list,dir->d_name);
        strcat(list,"\n");
      } 
    }
    closedir(d);
    printf("%s",list);
  }
  return(0);
}