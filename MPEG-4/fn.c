#include "fn.h"

int f_WriteBytes(FILE* fp_in, long pos, long size, FILE* fp_out)
{
	unsigned char* ch;
	if(fp_in == NULL || fp_out == NULL) return -1;
	ch = (unsigned char*)malloc(size);
	fseek(fp_in, pos, SEEK_SET);
	fread(ch, sizeof(char), size, fp_in);
	fwrite(ch, sizeof(char), size, fp_out);
    free(ch);
	return 0;
}