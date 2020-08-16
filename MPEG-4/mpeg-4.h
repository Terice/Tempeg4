#ifndef MPEG_4_H__
#define MPEG_4_H__
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"box.h"

 
typedef unsigned long int uint64;
typedef unsigned int uint32;
typedef unsigned short int uint16;

typedef struct stsd
{
    uint32 sample_count;
}stsd;
typedef struct stts
{
    uint32 sample_count;
    uint32 sample_delta;
}stts;
typedef struct stss
{
    uint32 sample_number;
}stss;
typedef struct ctts
{
    uint32 sample_count;
    uint32 sample_offset;
}ctts;
typedef struct stsc
{
    uint32 first_chunk;
    uint32 sample_perchunk;
    uint32 sample_description_index;
}stsc;
typedef struct stsz
{
    uint32 sample_size;
    uint32 sample_count;

}stsz;
typedef struct stco
{
    uint32 chunk_offset;
    
}stco;

typedef struct SPS__
{
    uint16 length;
    unsigned char* data;//length = 8*numberofSPSsets
}SPS;
typedef struct PPS__
{
    uint16 length;
    unsigned char* data;//length = 8*numberofPPSsets
}PPS;

typedef struct avcC
{
    unsigned char configuration;//1
    unsigned char avcProfileIndication;
    unsigned char profile_compatibility;
    unsigned char AVCLevelIndication;
    unsigned char lengthSizeMinusOne;
    unsigned char numOfSequenceParametersSets;
    SPS sPS;
    unsigned char numOfPictureParametersSets;
    PPS pPS;
}avcC;
typedef struct avc1
{
    unsigned char reserved[6];
    uint16 data_reference_index;

    uint16 pre_defined_0;
    uint16 reserved_2;
    uint32 pre_defined[3];

    uint16 width;
    uint16 height;

    uint32 horizons_res;
    uint32 vertical_res;

    uint32 reserved_3;

    uint16 frame_count;

    unsigned char compress_name[32];
    uint16 bit_depth;
    uint16 pre_defined_2;

    avcC* avcc;
}avc1;

int DataWriter_mpeg4_h264(FILE* fp_in, Box* data, int ID_trak, FILE* fp_out);
Box* ParserContainer_mpeg4(FILE* fp, Box* data);

void ReadInfoIntoBox(FILE* fp,  Box* rootBox);
void ReadDataIntoBox0(FILE* fp, Box* box);
void ReadDataIntoBox(FILE* fp, Box* box);

char IsFullBox(FILE* fp, size_t boxStartPos, size_t boxSize);
//this is to analyse whether it is a container box
char IsContainerBox(FILE* fp, size_t boxStartPos, size_t boxSize);
int ReadBoxInfo(FILE* fp, Box* box);

// delete the box's data by its name
void DataDeleter(Box* box);
// decode the box's data by its name
void DataPareser(FILE* fp, Box* box, Data* data);
size_t strlen_b(char* ch);

void RevertBigEndingChar(char* bytesToRevert, size_t length);
uint64 ChangeCharARToNumber(unsigned char* resource, size_t length);

// read data of length's char
void fread_m(void* data, FILE* fp, int isString, size_t length);
#endif //MPEG_4_H__