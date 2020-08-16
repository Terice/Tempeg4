#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"mpeg-4.h"
#include"box.h"


int main(int argc, char* argv[])
{
    FILE* fp, *fp_out;
    Box* mp4Box; 
#ifndef FILEPATH
    #define FILEPATH argv[0]
    if(argc != 2) {printf("Usage: %s filename\n", argv[0]); return -1;}
#endif
    if((fp = fopen(FILEPATH, "rb")) == NULL) 
    {
        printf("cant open file: %s\n", argv[1]);
        return -1;
    }
    else
    {
        printf("file: %s \n", argv[1]);
    }
    
    fp_out = fopen("trak.file", "w");
    mp4Box = InitBox(NULL, 0);
    ParserContainer_mpeg4(fp, mp4Box);
    DataWriter_mpeg4_h264(fp, mp4Box, 0, fp_out);
    DeleteBox(mp4Box);

    fclose(fp);

    return 0;
}