#ifndef MPEG_4_H__
#define MPEG_4_H__
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"box.h"

#include "tree.h"

 
typedef unsigned long int uint64;
typedef unsigned int uint32;
typedef unsigned short int uint16;



// box praser environment
typedef struct box_parser_environment__
{
    tree_node* tn_trak[10];
    int        info_trak[10];
    int index;
    int index_raw;
}box_parser_environment;

typedef struct mpeg4__
{
    tree* structure;
    box_parser_environment bpe;
}mpeg4;


mpeg4* InitMpeg4();
void DeleteMpeg4(mpeg4* m);

// write the ID_trak from fp_in's box to fp_out
int DataWriter_mpeg4_h264(FILE* fp_in, mpeg4* m, int trak, FILE* fp_out);
// Constructure the box's tree from fp
int ParserContainer_mpeg4(FILE* fp, mpeg4* m);



typedef struct stsd__
{
    uint32 sample_count;
}stsd;
typedef struct stts__
{
    uint32 sample_count;
    uint32 sample_delta;
}stts;
typedef struct stss__
{
    uint32 sample_number;
}stss;
typedef struct ctts__
{
    uint32 sample_count;
    uint32 sample_offset;
}ctts;
typedef struct stsc__
{
    uint32 first_chunk;
    uint32 sample_perchunk;
    uint32 sample_description_index;
}stsc;
typedef struct stsz__
{
    uint32 sample_size;
    uint32 sample_count;

}stsz;
typedef struct stco__
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

typedef struct avcC__
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
typedef struct avc1__
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







size_t strlen_b(char* ch);

void RevertBigEndingChar(char* bytesToRevert, size_t length);
uint64 ChangeCharARToNumber(unsigned char* resource, size_t length);

// read data of length's char
void fread_m(void* data, FILE* fp, int isString, size_t length);
#endif //MPEG_4_H__