#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"mpeg-4.h"
#include"box.h"

#define FILEPATH "../resource/fox.mp4"

int main(int argc, char* argv[])
{
    FILE* fp;
    Box* mp4Box; 

    if(argc != 2) {printf("Usage: %s filename\n", argv[0]); return -1;}
    if((fp = fopen(argv[1], "rb")) == NULL) 
    {
        printf("cant open file: %s\n", argv[1]);
        return -1;
    }
    else
    {
        printf("file: %s \n", argv[1]);
    }
    
    
    fseek(fp, 0L, SEEK_END);
    mp4Box = InitBox("mp4f", ftell(fp));
    rewind(fp);

    ReadInfoIntoBox(fp, mp4Box);
    ReadDataIntoBox(fp, mp4Box);

    PrintBox(mp4Box);

    DeleteBox(mp4Box);
    fclose(fp);

    return 0;
}