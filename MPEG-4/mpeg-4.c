#include "mpeg-4.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "box.h"
#include "fn.h"
Box* ParserContainer_mpeg4(FILE* fp, Box* data)
{
    // get all file's size and make root box
    fseek(fp, 0L, SEEK_END);
    strncpy(data->name, "FILE", 4);
    data->size = ftell(fp);
    rewind(fp);

    ReadInfoIntoBox(fp, data);
    PrintBox(data);
    printf("box's structure constructed\n");
    ReadDataIntoBox(fp, data);

    return data;
}

int GetSample(  stco* chunkoffset,  int length_stco,\
                stsz* samplesize,   int length_stsz,\
                stsc* sampletochunk,int length_stsc,\
                int ID_sample,\
                long* re_spos, long* re_size\
)
{
    int ID_chunk, i, index_ChunkTable, sample_id_inchunk, index_in_chunk;
    int sample_size, before_sample_size;

    ID_chunk = -1;
    i = 0;
    index_ChunkTable = 0;

    // i is the count of all ID_chunk !number! of chunk's sample summary(not index)
    do
    {
        i += sampletochunk[index_ChunkTable].sample_perchunk;
        ID_chunk++;

        // chunk ID is larger than or equal to next change chunk's sample count 
        // then change the calc index
        // no case of "overflow" because after size use the latest value
       
        // stsc.first_chunk start with "1" instead of "0"
        if(index_ChunkTable < length_stsc-1 && ID_chunk+1 >= sampletochunk[index_ChunkTable+1].first_chunk)
        {                           //-1 make index_ChunkTable always avaiable
            index_ChunkTable++;
        }
    }while(i < ID_sample + 1);
    if(ID_chunk > length_stco) printf("GetSample: stco overflow\n");
    //get the file offset(chunk offset)
    *re_spos = chunkoffset[ID_chunk].chunk_offset;
    if(ID_chunk > length_stco) printf("GetSample: stsz overflow\n");

    // get the sample's size 
    // i is the count of all ID_chunk !number! of chunk's sample summary(not index)
    sample_id_inchunk = sampletochunk[index_ChunkTable].sample_perchunk - (i - ID_sample); 
    index_in_chunk = 0;
    before_sample_size = 0;
    while (index_in_chunk < sample_id_inchunk)
    {
        /* eg: calc 7th sample's size in chunk 4
        sample id | chunk id
             _____             samples    sample size
            0_____  chunk 1       1        size[1]
            1_____  chunk 2       
            2
            3       chunk 3       3
            4_____
            5                              size[6]                          0
            6       chunk 4                size[7]                          1
            7_____                         size[8]        sample_id_inchunk 2

            size[7 - 2 + 0 + 1] + size[7 - 2 + 1 + 1]
        */
        sample_size = samplesize[0].sample_size == 0?\
            samplesize[(ID_sample - sample_id_inchunk + index_in_chunk)+1].sample_size:\
            samplesize[0].sample_size;
        before_sample_size += sample_size;
        index_in_chunk++;
    }
    *re_spos += before_sample_size;
    *re_size  = samplesize[0].sample_size == 0?\
        samplesize[ID_sample+1].sample_size:\
        samplesize[0].sample_size;
}
const unsigned char h264_nal_header[4] = {0,0,0,1};
int WriteSample(FILE* fp_in, long sample_start_pos, long sample_size, FILE* fp_out,\
                SPS* sps, PPS* pps\
)
{
    long size_writen;
    long size_current;
    unsigned char data_tmp[4];

    size_writen = 0;
    fseek(fp_in, sample_start_pos, SEEK_SET);
    while (size_writen < sample_size)
    {
        fread(&data_tmp, sizeof(char), 4, fp_in);
        size_current = ChangeCharARToNumber(data_tmp, 4);
        if(size_current > sample_size) return -1;
        // write the h264 NAL header(0x 00 00 01)
        fwrite(h264_nal_header, sizeof(char), 4, fp_out);

        if(fgetc(fp_in) == 0x65) 
        {
            // insert SPS
            fwrite(sps->data, sps->length, sizeof(char), fp_out);
            // insert PPS
            fwrite(h264_nal_header, sizeof(char), 4, fp_out);
            fwrite(pps->data, pps->length, sizeof(char), fp_out);

            fwrite(h264_nal_header, sizeof(char), 4, fp_out);
        }
        fseek(fp_in, -1L, SEEK_CUR);// take back file pointer

        f_WriteBytes(fp_in, ftell(fp_in), size_current, fp_out);
        size_writen += size_current+4;
        
    }
    return 0;
}
int DataWriter_mpeg4_h264(FILE* fp_in, Box* data, int ID_trak, FILE* fp_out)
{
    Box* result[10];
    int result_find, i;
    Box* trak;
    Box* b_stco, *b_stsz, *b_stsd, *b_stsc;
    Box* b_avcc;
    long pos_start, size;

    b_stco = \
    b_stsz = \
    b_stsd = \
    b_stsc = NULL;
    if(fp_out == NULL) {printf("FILE*: invalid out file pointer");return -1;}
    result_find = FindBoxs("trak", data, result, 10);
    if(ID_trak + 1 > result_find) {printf("ID_TRAK: invalid ID (not enough in file)\n");return -1;}
    else trak = result[ID_trak];
    b_stco = FindBox("stco", trak);
    b_stsz = FindBox("stsz", trak);
    b_stsd = FindBox("stsd", trak);
    b_stsc = FindBox("stsc", trak);
    b_avcc = b_stsd->l_child->l_child;
    
    if( b_stco == NULL ||\
        b_stsz == NULL ||\
        b_stsd == NULL ||\
        b_stsc == NULL)
    {
        printf("BOX: invalid box structure\n");
        return -1;
    }
    // start is SEEK_SET
    // 1: calc the ID of the chunk (where the sample located)--------------------------1st offset(in file)
    // 2: calc the sample's offset(size[0] + size[1] + ... + size[target]) ------------2nd offset(in chunk)
    // 3: get the start postition and size, then write (h264 add 0x00 00 01 at every nal)
    stco* data_stco = (stco*)(b_stco->data->data); int length_stco = b_stco->data->entrycount;
    stsz* data_stsz = (stsz*)(b_stsz->data->data); int length_stsz = b_stsz->data->entrycount;
    stsd* data_stsd = (stsd*)(b_stsd->data->data); int length_stsd = b_stsd->data->entrycount;
    stsc* data_stsc = (stsc*)(b_stsc->data->data); int length_stsc = b_stsc->data->entrycount;
    avcC* data_avcc = (avcC*)(b_avcc->data->data);
    SPS* sps = &(data_avcc->sPS);
    PPS* pps = &(data_avcc->pPS);
    
    i = 0;
    while (i < data_stsz[0].sample_count)
    {
        GetSample(
            data_stco,length_stco,\
            data_stsz,length_stsz,\
            data_stsc,length_stsc,\
            i,\
            &pos_start, &size\
        );
        WriteSample(fp_in, pos_start, size, fp_out, sps, pps);
        printf("sample index: %5d, write bytes: %10x\n", i, size);
        i++;
    }
}
// read the box data into box
void ReadDataIntoBox0(FILE* fp, Box* box)
{
    Data* data;
    // if a box dont have child box, it is calc as a "fullbox"
    if(box->l_child == NULL && strncmp(box->name, "mdat", 4) &&\
        strncmp(box->name, "ftyp", 4) &&\
        strncmp(box->name, "avc1", 4) &&\
        strncmp(box->name, "avcC", 4) &&\
        box->size > 8L)
    {
        fseek(fp, box->pos_start + 8L, SEEK_SET);
        // malloc the box 
        data = (Data*)malloc(sizeof(Data));
        // data->data = (unsigned char*)malloc(box->size - 12L);

        // read the version and flags
        fread(&(data->version), sizeof(char), 1, fp);
        fread(&(data->flags), sizeof(char), 3, fp);
        RevertBigEndingChar(data->flags, 3);// revert the sort of flags
        
        // process the different kind of data here
        DataPareser(fp, box, data);
        box->data = data;
    }
    if(box->r_broth != NULL) ReadDataIntoBox0(fp, box->r_broth);
    if(box->l_child != NULL) ReadDataIntoBox0(fp, box->l_child);

}
void ReadDataIntoBox(FILE* fp, Box* box)
{
    ReadDataIntoBox0(fp, box);
    rewind(fp);
}
// fn: read information into box from fp
// re: void
// this function is used to construct the mp4's file structure
// fp will be rewind
// data is in box(box must be all file, especially the size)
void ReadInfoIntoBox(FILE* fp,  Box* rootBox)
{
    Box * upBox, * tmp;
    Box* usingBox;
    char containerBox;

    usingBox = InitBox(NULL, 0);
    upBox = rootBox;

    // compare to the file box
    while(ftell(fp) <= rootBox->size)
    {
        if(!ReadBoxInfo(fp, usingBox)) break;
        
        tmp = InsertNewBox(usingBox, upBox);

        // if it is a container box, 
        // then go forward 8 char(size + name)
        // at this container's data start position
        containerBox = IsContainerBox(fp, ftell(fp), usingBox->size);
        if(containerBox > 0) 
        {
            fseek(fp, 8L, SEEK_CUR);
            upBox = tmp;
        }
        else fseek(fp, usingBox->size, SEEK_CUR); // else skip this box
        
        // if at the container box's end, 
        // then find the container box's wrap box
        if(ftell(fp) >= upBox->pos_start + upBox->size)
        {
            upBox = FindWrapBoxCanUse(upBox);
        }
    }
    DeleteBox(usingBox);
    rewind(fp);
}


// to read the basic info of the box
// fp(data) ---->  box
int ReadBoxInfo(FILE* fp, Box* box)
{
    //read size - read type - read data
    unsigned char tmp[4];

    // size
    if(fread(tmp, sizeof(char), 4, fp) < 4) return 0;
    box->size = (long)tmp[0] << 24 | (long)tmp[1] << 16 | (long)tmp[2] << 8 | (long)tmp[3];
    // name
    fread(tmp, sizeof(char), 4, fp);
    strncpy(box->name, tmp, 4);
    fseek(fp, -8L, SEEK_CUR);
    box->pos_start = ftell(fp);

    return 1;
}

char IsFullBox(FILE* fp, size_t boxpos_start, size_t boxSize)
{
    if(!IsContainerBox(fp, boxpos_start, boxSize)) return 1;
    else return 0;
}
// this is to analyse whether it is a container box
// return the count of contained box
// FILE* fp must be set at the start of box's start(the pos of size's start)
char IsContainerBox(FILE* fp, size_t boxpos_start, size_t boxSize)
{
    char result;
    unsigned long size;
    unsigned char tmp[4];

    

    fseek(fp, 4L, SEEK_CUR);
    fread(tmp, sizeof(char), 4, fp);
    if(!strncmp(tmp, "mdat", 4)) 
    {
        fseek(fp, boxpos_start, SEEK_SET);
        return 0;
    }
    while(ftell(fp) < boxpos_start + boxSize)
    {
        fread(tmp, sizeof(char), 4, fp);
        size = (long)tmp[0] << 24 | (long)tmp[1] << 16 | (long)tmp[2] << 8 | (long)tmp[3];
        fseek(fp, -4L, SEEK_CUR);
        fseek(fp, size, SEEK_CUR);
        result++;
        if(size == 0) {result = 0; break;}
    }
    if(ftell(fp) != boxpos_start + boxSize || boxSize == 8L || size == 0) result = 0;

    fseek(fp, boxpos_start, SEEK_SET);
    
    return result;
}

//change BigEnd Code to normal
void RevertBigEndingChar(char* bytesToRevert, size_t length)
{
    size_t i;
    char tmp;

    i = 0;
    for(i = 0; i < length / 2; i++)
    {
        tmp = bytesToRevert[i];
        bytesToRevert[i] = bytesToRevert[length - 1 - i];
        bytesToRevert[length - 1 - i] = tmp;
    }
}
// fn: change BigEnd char to number
// re: the calc number
uint64 ChangeCharARToNumber(unsigned char* resource, size_t length)
{
    uint64 result;

    result = 0;
    RevertBigEndingChar(resource, length);
    
    for (int i = 0; i < length; i++)
    {
        //printf("%2x", resource[i]);
        result += (long)resource[i]<<(8 * i);
        //printf("result :%ld\n", result);
    }
    
    // printf("result :%ld\n", result);
    return result;
}

// fn: decode the date of the box
// note: no postion's need for the fp
// ------it will set fp at correct postion
// ------decode by the name of the box
void DataPareser(FILE* fp, Box* box, Data* data)
{
    unsigned char entry_count[4];// the entry's number(in char type-----the data in file)
    uint32 entryCount; // the entry's number
    unsigned char tmp[4];
    char* name = box->name;

    // sample structure

    // 4(size) + 4(name) + 4(version 1 + flags 3)
    // so skip forward 12L at the data's start postion
    fseek(fp, box->pos_start + 12L, SEEK_SET);
    if(!strncmp(name, "stsd", 4))
    {
        avcC* avccTmp = (avcC*)malloc(sizeof(avcC));
        avc1* avc1Tmp = (avc1*)malloc(sizeof(avc1));
        Box* exchange;

        // 4 uchar of entry's count
        fread(tmp, sizeof(char), 4L, fp);
        entryCount = ChangeCharARToNumber(tmp, 4);

        stsd* sample = (stsd*)malloc(sizeof(stsd));
        sample->sample_count = entryCount;


        // read avc1 box
        fread(tmp, sizeof(char), 4L, fp);
        entryCount = ChangeCharARToNumber(tmp, 4);
        exchange = InsertExistBox(InitBox("avc1", (size_t)entryCount), box);
        exchange->pos_start = ftell(fp) - 4L;
        fseek(fp, 4L, SEEK_CUR);
        fread_m(&avc1Tmp->reserved               , fp, 1, 6);
        fread_m(&avc1Tmp->data_reference_index   , fp, 0, 2);
        fread_m(&avc1Tmp->pre_defined_0          , fp, 0, 2);
        fread_m(&avc1Tmp->reserved_2             , fp, 0, 2);
        fread_m(&avc1Tmp->pre_defined            , fp, 0, 12);
        fread_m(&avc1Tmp->width                  , fp, 0, 2);
        fread_m(&avc1Tmp->height                 , fp, 0, 2);
        fread_m(&avc1Tmp->horizons_res           , fp, 0, 4);
        fread_m(&avc1Tmp->vertical_res           , fp, 0, 4);
        fread_m(&avc1Tmp->reserved_3             , fp, 0, 4);
        fread_m(&avc1Tmp->frame_count            , fp, 0, 2);
        fread_m(&avc1Tmp->compress_name          , fp, 0, 32);
        fread_m(&avc1Tmp->bit_depth              , fp, 0, 2);
        fread_m(&avc1Tmp->pre_defined_2          , fp, 0, 2);
        exchange->data = InitBoxData();
        exchange->data->data = avc1Tmp;
        // read avcC's size
        fread_m(&entryCount, fp, 0, 4);
        // read avcC
        exchange = InsertExistBox(InitBox("avcC", (size_t)entryCount), exchange);
        exchange->pos_start = ftell(fp) - 4L;
        // read avcC's data
        fseek(fp, 4L, SEEK_CUR);
        fread_m(&avccTmp->configuration              , fp, 0, 1);
        fread_m(&avccTmp->avcProfileIndication       , fp, 0, 1);
        fread_m(&avccTmp->profile_compatibility      , fp, 0, 1);
        fread_m(&avccTmp->lengthSizeMinusOne         , fp, 0, 1);
        fread_m(&avccTmp->AVCLevelIndication         , fp, 0, 1);
        fread_m(&avccTmp->numOfSequenceParametersSets, fp, 0, 1);
        
        avccTmp->numOfSequenceParametersSets &= (0x1f);
        // printf("\nnumber of SPS sets: %8x", avccTmp->numOfSequenceParametersSets);
        
        // read SPS
        for (size_t i = 0; i < avccTmp->numOfSequenceParametersSets; i++)
        {
            fread_m(&avccTmp->sPS.length, fp, 0, 2);
            // printf("avccTmp->sPS.length %08x\n", avccTmp->sPS.length);
            avccTmp->sPS.data = (char*)malloc(avccTmp->sPS.length * avccTmp->numOfSequenceParametersSets);
            fread(avccTmp->sPS.data, sizeof(char), avccTmp->sPS.length * avccTmp->numOfSequenceParametersSets, fp);
            // printf("%x ", avccTmp->sPS.data[i]);
        }
        fread_m(&avccTmp->numOfPictureParametersSets, fp, 0, 1);
        // read PPS
        // printf("\nnumber of PPS sets: %d", avccTmp->numOfPictureParametersSets);
        for (size_t i = 0; i < avccTmp->numOfPictureParametersSets; i++)
        {
            fread_m(&avccTmp->pPS.length, fp, 0, 2);
            avccTmp->pPS.data = (char*)malloc(avccTmp->pPS.length * avccTmp->numOfPictureParametersSets);
            fread(avccTmp->pPS.data, sizeof(char), avccTmp->pPS.length * avccTmp->numOfPictureParametersSets, fp);
            // printf("%x ", avccTmp->pPS.data[i]);
        }
        exchange->data = InitBoxData();
        exchange->data->data = avccTmp;
        data->data = sample;
    }
    // time to sample
    else if(!strncmp(name, "stts", 4))
    {
        fread(entry_count, sizeof(char), 4L, fp);
        entryCount = ChangeCharARToNumber(entry_count, 4);
        stts* sample = (stts*)malloc(sizeof(stts)*entryCount);
        for (size_t i = 0; i < entryCount; i++)
        {
            fread(tmp, sizeof(char), 4L, fp);
            sample[i].sample_count = ChangeCharARToNumber(tmp, 4);
            fread(tmp, sizeof(char), 4L, fp);
            sample[i].sample_delta = ChangeCharARToNumber(tmp, 4);
            // printf("stts:sample cout: %d, sample delta: %d \n",sample[i].sample_count,sample[i].sample_delta);
        }
        data->data = sample;
    }
    // sync sample
    else if(!strncmp(name, "stss", 4))
    {
        fread(entry_count, sizeof(char), 4, fp);
        entryCount = ChangeCharARToNumber(entry_count, 4);
        // printf("entryCount:%d\n", entryCount);
        stss* sample = (stss*)malloc(sizeof(stss)*entryCount);

        for (size_t i = 0; i < entryCount; i++)
        {
            fread(tmp, sizeof(char), 4, fp);
            sample[i].sample_number = ChangeCharARToNumber(tmp, 4);
            printf("stss:ID: %ld, sample_number : %d\n", i, sample[i].sample_number);
        }

        data->data = sample;
    }
    else if(!strncmp(name, "ctts", 4))
    {
        fread(entry_count, sizeof(char), 4, fp);
        entryCount = ChangeCharARToNumber(entry_count, 4);

        // printf("entry count : %d\n", entryCount);

        ctts* sample = (ctts*)malloc(sizeof(ctts) * entryCount);
        for (size_t i = 0; i < entryCount; i++)
        {
            fread(tmp, sizeof(char), 4L, fp);
            sample[i].sample_count = ChangeCharARToNumber(tmp, 4);
            fread(tmp, sizeof(char), 4L, fp);
            sample[i].sample_offset = ChangeCharARToNumber(tmp, 4);
            fread(tmp, sizeof(char), 4L, fp);

            // printf("ID: %ld, sample count: %d,\t sample offset: %d \n", i, sample[i].sample_count, sample[i].sample_offset);
        }
        data->data = sample;
    }
    // sample to chunk table
    else if(!strncmp(name, "stsc", 4))
    {
        fread(entry_count, sizeof(char), 4, fp);
        entryCount = ChangeCharARToNumber(entry_count, 4);

        stsc* sample = (stsc*)malloc(sizeof(stsc)*\
        entryCount);

        for (size_t i = 0; i < entryCount; i++)
        {
            fread(tmp, sizeof(char), 4L, fp);
            sample[i].first_chunk = ChangeCharARToNumber(tmp, 4);
            fread(tmp, sizeof(char), 4L, fp);
            sample[i].sample_perchunk = ChangeCharARToNumber(tmp, 4);
            fread(tmp, sizeof(char), 4L, fp);
            sample[i].sample_description_index = ChangeCharARToNumber(tmp, 4);
            printf("stsc: first chunk ID: %d, sample per chunk: %d, sample_description:%d\n",\
            sample[i].first_chunk, \
            sample[i].sample_perchunk, \
            sample[i].sample_description_index\
            );
        }
        data->data = sample;
    }
    // 
    // all sample count and the size of every sample
    else if(!strncmp(name, "stsz", 4))
    {
        stsz* sampleMultiple;
        stsz sample;
        
        // sample size 
        fread(tmp, sizeof(char), 4L, fp);
        sample.sample_size = ChangeCharARToNumber(tmp, 4);
        // sample count(the count of sample in trak)
        fread(tmp, sizeof(char), 4L, fp);
        sample.sample_count = ChangeCharARToNumber(tmp, 4);

        // data[0] is used to store the every size and all count 

        // every sampe have different size 
        if(sample.sample_size == 0)
        {
            sampleMultiple = (stsz*)malloc(sizeof(stsz)*(sample.sample_count + 1));
            sampleMultiple[0].sample_count = sample.sample_count;
            sampleMultiple[0].sample_size = sample.sample_size;
            for (int i = 1; i <= sample.sample_count; i++)
            {
                fread(tmp, sizeof(char), 4, fp);
                sampleMultiple[i].sample_size = ChangeCharARToNumber(tmp, 4);
                sampleMultiple[i].sample_count = i;
                printf("stsz: sample count ID: %d, sample size: %d\n", i, sampleMultiple[i].sample_size);
            }
        }
        else // all sample have the same size
        {
            printf("stsz: %d sample have same size: %d\n", sample.sample_count, sample.sample_size);
            sampleMultiple = (stsz*)malloc(sizeof(stsz));
            sampleMultiple[0].sample_count = sample.sample_count;
            sampleMultiple[0].sample_size = sample.sample_size;
        }
        data->data = sampleMultiple;
    }
    // chunk offset (in file)
    else if(!strncmp(name, "stco", 4))
    {
        fread(entry_count, sizeof(char), 4L, fp);
        entryCount = ChangeCharARToNumber(entry_count, 4);
        printf("stco: chunk offset list count :%d\n", entryCount);
        stco* chunk = (stco*)malloc(box->size - 16L);

        for (size_t i = 0; i < entryCount; i++)
        {
            fread(tmp, sizeof(char), 4, fp);
            chunk[i].chunk_offset = ChangeCharARToNumber(tmp, 4);
            printf("stco: chunk ID: %3ld, Offset: 0x%10X\n", i, chunk[i].chunk_offset);
        }
        data->data = chunk;
    }
    data->entrycount = (int)entryCount;
}
void DataDeleter(Box* box)
{

}
void fread_m(void* data, FILE* fp, int isString, size_t length)
{
    fread(data, sizeof(char), length, fp);
    // printf("address of data : %p sizeof data : %ld\n", data, length);

    if(isString) RevertBigEndingChar(data, length);
    else ChangeCharARToNumber(data, length);
}