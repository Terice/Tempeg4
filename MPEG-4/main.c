#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"mpeg-4.h"


#define FILEPATH_IN   "fox.mp4"
#define FILEPATH_OUT  "trak"

int main(int argc, char* argv[])
{
    FILE* fp, *fp_out;
    mpeg4* fmpeg4; 
    int option;
    
    if((fp = fopen(FILEPATH_IN, "r")) == NULL)     {printf("cant open file: %s\n", FILEPATH_IN );return -1;}
    if((fp_out = fopen(FILEPATH_OUT, "w")) == NULL){printf("cant open file: %s\n", FILEPATH_OUT);return -1;}
    
    fmpeg4 = InitMpeg4();
    ParserContainer_mpeg4(fp, fmpeg4);
    DataWriter_mpeg4_h264(fp, fmpeg4, 0, fp_out);
    DeleteMpeg4(fmpeg4);
    

    fclose(fp);

    return 0;
}
