#include <stdio.h>
#include<string.h>
#include <stdlib.h>
#include <unistd.h>
#include <error.h>
#include <sys/ioctl.h>
#include <dirent.h> 

int main()
{

    FILE *file = fopen("form.html", "r");
	char output;
	while(fscanf(file,"%c", &output)==1)
		printf("%c", output);

	fclose(file);


	DIR *d;
    struct dirent *dir;
	char list[200]="\n";
    d = opendir("./ServerFile");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if(strcmp(dir->d_name,"..")!=0 && strcmp(dir->d_name,".")!=0){
                strcat(list, dir->d_name);
                strcat(list, " ");
            } 
        }
		printf("%s", list);
        closedir(d);
    }
}