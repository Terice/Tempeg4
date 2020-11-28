#include "mpeg-4.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "fn.h"


mpeg4* InitMpeg4()
{
    mpeg4* m = (mpeg4*)malloc(sizeof(mpeg4));
    m->structure = InitTree();
    m->bpe.index_raw = 0;

    return m;
}
void DeleteMpeg4(mpeg4* m)
{
    if(m->structure) DeleteTree(m->structure);
    free(m);
}



// delete the box's data by its name
void DataDeleter(Box* box);
// decode the box's data by its name
void DataPareser(FILE* fp, tree_node* tn, box_parser_environment* bpe);

// return t & c 's name is equal ?
bool FindTreeNode_ItemNameEqual(tree_node* t, tree_node* c)
{
    return !strncmp(t->item.name, c->item.name, 4);
}

int ReadBoxInfo(FILE* fp, box* b);
void ReadInfoIntoTree(FILE* fp, tree* t, box_parser_environment* bpe);
void ReadDataIntoTree(FILE* fp, tree* t, box_parser_environment* bpe);
int IsContainerBox(FILE* fp, box* b); // judge box follow the file "fp"

// find all the "trak"
// confirm the type of the "trak"
static void PreParse(mpeg4* m)
{
    tree* t;box_parser_environment* bpe;

    t = m->structure;
    bpe = &m->bpe;
    int i;

    tree_node stand; tree_node* tn_find;
    tree_node* tn_tmp;
    // find the all "trak" box's pointers
    strncpy(stand.item.name,"trak", 4);
    // stand is used to store a name "trak"
    // tn_find is used as the finded node
    FindNode(t, &stand, FindTreeNode_ItemNameEqual, &tn_find);

    // loop find the "trak" node
    i = bpe->index;
    do
    {
        bpe->tn_trak[i] = tn_find;
        i = ++bpe->index; // record the number of trak
    } while ((tn_find = tn_find->r_brother ) && !strncmp(tn_find->item.name, "trak", 4));
    strncpy(stand.item.name,"vmhd", 4);

    // loop find every "trak"'s type, and give index to corresponding node
    // video - 1
    // audio - 2
    for(i = 0; i < bpe->index; i++)
    {
        tn_tmp = bpe->tn_trak[i];
        TraverseNode(tn_tmp, 
            if(!strncmp(tn_tmp->item.name, "vmhd", 4)) {bpe->info_trak[i] = 1;}
            else if(!strncmp(tn_tmp->item.name, "smhd", 4)) bpe->info_trak[i] = 2;
        );
    }
}

bool PrintfTreeNode(tree_node* t)
{
    char ch[5];
    ch[5] = 0;
    if(t)
    {
        strncpy(ch, t->item.name, 4);
        for(size_t i = 0; i < t->depth; i++)
        {
            printf("     ");
        }
        printf("%s\n", ch);
    }
    return true;
}
int ParserContainer_mpeg4(FILE* fp, mpeg4* m)
{
    tree* t;
    box b; box_parser_environment* bpe;
    int i;

    t = m->structure;
    bpe = &m->bpe;
    
    bpe->index = 0;
    
    // get all file's size and make root box
    fseek(fp, 0L, SEEK_END);
    strncpy(b.name, "FILE", 4);             // constructe the FILE BOX (which is also the ROOT box)
    b.size = ftell(fp);b.pos_start = -8L;
    rewind(fp);
    Tree_InsertItem(t->head, &b, true);     // insert the "FILE"

    // construct the tree structure of the file "fp"
    // mainly read "size" & "start position"
    ReadInfoIntoTree(fp, t, bpe);

    // get some info: trak pointer and trak info
    PreParse(m);
    
    // read and parser the structure data of tree "t"
    ReadDataIntoTree(fp, t, bpe);
    Tree_Traverse_ROOT1(t, PrintfTreeNode);

    
    return 0;
}

// already know the index of the "sample"
// calculate the file position and size of the "sample"
static int GetSample(   stco* chunkoffset,  int length_stco,\
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
    // calc the index of "chunk"
    // "i" is the all count of the sample of before chunk
    do
    {
        ID_chunk++;
        // chunk ID is larger than or equal to next change chunk's sample count 
        // then change the calc index
        // no case of "overflow" because after size use the latest value
       
        // stsc.first_chunk start with "1" instead of "0"
        if(index_ChunkTable < length_stsc-1 && ID_chunk == sampletochunk[index_ChunkTable+1].first_chunk-1)
        {                           //   ^ -1 make index_ChunkTable always avaiable
            index_ChunkTable++;
        }
        i += sampletochunk[index_ChunkTable].sample_perchunk;

    }while(i < ID_sample + 1);
        

    if(ID_chunk > length_stco) printf("GetSample: stco overflow\n");
    //get the file offset(chunk offset)
    *re_spos = chunkoffset[ID_chunk].chunk_offset;
    

    // calc the index of the "sample"
    // get the sample's size 
    // i is the count of all ID_chunk !number! of chunk's sample summary(not index)
    sample_id_inchunk = sampletochunk[index_ChunkTable].sample_perchunk - (i - (ID_sample+1) ) - 1;  
    index_in_chunk = 0; // index add value
    before_sample_size = 0;
    while (index_in_chunk < sample_id_inchunk)
    {
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
    
    return 1;
    /* 
    eg: calc 7th sample's size and start-position in chunk 4
    sample id | chunk id
        ._____             samples    sample size
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
    
}
const unsigned char h264_nal_header[4] = {0,0,0,1};
static int WriteSample( FILE* fp_in, \
                        long sample_start_pos, long sample_size, \
                        FILE* fp_out,\
                        SPS* sps, PPS* pps\
)
{
    long size_writen;
    long size_current;
    unsigned char data_tmp[4];

    size_writen = 0;
    fseek(fp_in, sample_start_pos, SEEK_SET);
    printf("write a sample, start : 0x%x, size : %8d\n", sample_start_pos, sample_size);

    // there maybe the case that a "sample" have more than 1 "NAL unit"
    // so write until @ end of sample
    while (size_writen < sample_size)
    {
        fread(&data_tmp, sizeof(char), 4, fp_in);
        size_current = ChangeCharARToNumber(data_tmp, 4);
        printf("a nal size: %5d", size_current);
        if(size_current > sample_size) {printf("  x\n"); return -1;}
        else {printf("\n");}

        // write the h264 NAL header(0x 00 00 01)
        fwrite(h264_nal_header, sizeof(char), 4, fp_out);
        // if it is an "IDR" nal than insert the sps and pps
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
        printf("write a nal\n");
    }
    return 0;
}
int DataWriter_mpeg4_h264(FILE* fp_in, mpeg4* m, int trak_id, FILE* fp_out)
{
    int i;

    tree_node* trak, *tmp;
    tree_node* b_stco, *b_stsz, *b_stsd, *b_stsc;
    tree_node* b_avcC;

    box_parser_environment* bpe;
    long pos_start, size;

    bpe = &m->bpe;
    b_stco = \
    b_stsz = \
    b_stsd = \
    b_stsc = NULL;
    if(fp_out == NULL) {printf("FILE*: invalid out file pointer");return -1;}

    trak = (bpe->tn_trak[trak_id]);
    tmp = trak;
    TraverseNode(tmp, 
             if(!strncmp(tmp->item.name, "stco", 4)) b_stco = tmp;
        else if(!strncmp(tmp->item.name, "stsz", 4)) b_stsz = tmp;
        else if(!strncmp(tmp->item.name, "stsd", 4)) b_stsd = tmp;
        else if(!strncmp(tmp->item.name, "stsc", 4)) b_stsc = tmp;
    )
    b_avcC = b_stsd->l_child->l_child;
    
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
    // 3: get the start postition and size, then write (h264 add 0x00 00 00 01 at every nal)
    stco* data_stco = (stco*)(b_stco->item.common_data->full_data); int length_stco = b_stco->item.common_data->entrycount;
    stsz* data_stsz = (stsz*)(b_stsz->item.common_data->full_data); int length_stsz = b_stsz->item.common_data->entrycount;
    stsd* data_stsd = (stsd*)(b_stsd->item.common_data->full_data); int length_stsd = b_stsd->item.common_data->entrycount;
    stsc* data_stsc = (stsc*)(b_stsc->item.common_data->full_data); int length_stsc = b_stsc->item.common_data->entrycount;
    avcC* data_avcc = (avcC*)(b_avcC->item.common_data->full_data);
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
        printf("sample index: %5d,start:  0x%x, write bytes: %10x\n", i, pos_start, size);
        WriteSample(fp_in, pos_start, size, fp_out, sps, pps);
        i++;
    }
}
// no longer used
// read the box data into box
// static void ReadDataIntoBox0(FILE* fp, tree* t)
// {
    // Data* data;
    // box* b;
    // // if a b dont have child b, it is calc as a "fullbox"
    // if(b->l_child == NULL && \
    // 	strncmp(b->name, "mdat", 4) &&\
    //     strncmp(b->name, "ftyp", 4) &&\
    //     strncmp(b->name, "avc1", 4) &&\
    //     strncmp(b->name, "avcC", 4) &&\
    //     b->size > 8L)
    // {
    //     fseek(fp, b->pos_start + 8L, SEEK_SET);
    //     // malloc the b 
    //     data = (Data*)malloc(sizeof(Data));
    //     // data->data = (unsigned char*)malloc(b->size - 12L);

    //     // read the version and flags
    //     fread(&(data->version), sizeof(char), 1, fp);
    //     fread(&(data->flags), sizeof(char), 3, fp);
    //     RevertBigEndingChar(data->flags, 3);// revert the sort of flags
        
    //     // process the different kind of data here
    //     DataPareser(fp, b, data);
    //     b->data = data;
    // }
    //     if(b->r_broth != NULL) ReadDataIntoBox0(fp, b->r_broth);
    //     if(b->l_child != NULL) ReadDataIntoBox0(fp, b->l_child);
// }

// read data into the mp4 tree
void ReadDataIntoTree(FILE* fp, tree* t, box_parser_environment* bpe)
{
    tree_node* tn;
    box_data* data; box* b;
    char ch;

    tn = t->head->l_child;
    TraverseNode(tn, 
            b = &tn->item;
            // if a b dont have child b, it is calc as a "fullbox"
            if(!strncmp(b->name, "trak", 4))
            {
                if(bpe->info_trak[bpe->index_raw] == 1) //only parse the video "1" trak
                bpe->info_trak[bpe->index_raw] += 10;
                else
                    bpe->index_raw++;
            }
            if( tn->l_child == NULL && \
                strncmp(b->name, "mdat", 4) &&\
                strncmp(b->name, "ftyp", 4) &&\
                strncmp(b->name, "avc1", 4) &&\
                strncmp(b->name, "avcC", 4) &&\
                b->size > 8L)
            {
                fseek(fp, b->pos_start + 8L, SEEK_SET);
                // malloc the data
                data = (box_data*)malloc(sizeof(box_data));

                // read the version and flags
                fread(&(data->version), sizeof(char), 1, fp);
                fread(&(data->flags), sizeof(char), 3, fp);
                ch = data->flags[2]; data->flags[2] = data->flags[0]; data->flags[2] = ch; //revert big-ending code
                
                b->common_data = data;
                // process the different kind of data here
                DataPareser(fp, tn, bpe);
            }
    );
}

// read the "name" and "size" into the box tree
void ReadInfoIntoTree(FILE* fp, tree* t, box_parser_environment* bpe)
{
    box b;            // tmp data storage
    tree_node *cur, *parent;
    seqstack* s;
    tree_node tmp;
    
    long edge;       // parent box 's file edge

    s = InitSeqStack();

    cur = Tree_GetRoot(t);
    Push(s, cur);   // push root into the stack
    while(!IsEmpty(s))
    {
        Pop(s, &parent); edge = parent->item.pos_start + parent->item.size;
        fseek(fp, parent->item.pos_start + 8L, SEEK_SET);
        // read the base info of current box from the fp
        do
        {
            if(!ReadBoxInfo(fp, &b)) break;
            cur = Tree_InsertItem(parent, &b, true);
            cur->index = t->count++;
            if(IsContainerBox(fp, &b))
            {
                Push(s, cur);
            }
        } while (b.pos_start + b.size < edge);
    }
    DeleteSeqStack(s);
}

// fn: read information into box from fp
// re: void
// this function is used to construct the mp4's file structure
// fp will be rewind
// data is in box(box must be all file, especially the size)
// void ReadInfoIntoBox(FILE* fp,  Box* rootBox)
// {
//     Box * upBox, * tmp;
//     Box* usingBox;
//     char containerBox;

//     usingBox = InitBox(NULL, 0);
//     upBox = rootBox;

//     // compare to the file box
//     while(ftell(fp) <= rootBox->size)
//     {
//         if(!ReadBoxInfo(fp, usingBox)) break;
        
//         tmp = InsertNewBox(usingBox, upBox);

//         // if it is a container box, 
//         // then go forward 8 char(size + name)
//         // at this container's data start position
//         // containerBox = IsContainerBox(fp, ftell(fp), usingBox->size);
//         if(containerBox > 0) 
//         {
//             fseek(fp, 8L, SEEK_CUR);
//             upBox = tmp;
//         }
//         else fseek(fp, usingBox->size, SEEK_CUR); // else skip this box
        
//         // if at the container box's end, 
//         // then find the container box's wrap box
//         if(ftell(fp) >= upBox->pos_start + upBox->size)
//         {
//             upBox = FindWrapBoxCanUse(upBox);
//         }
//     }
//     DeleteBox(usingBox);
//     rewind(fp);
// }

// need set the fp pointer
// to read the basic info of the box
// fp(data) ---->  box
int ReadBoxInfo(FILE* fp, box* b)
{
    //read size - read type - read data
    unsigned char tmp[4];
    // position of start
    b->pos_start = ftell(fp);
    // size
    if(fread(tmp, sizeof(char), 4, fp) < 4) return 0;
    b->size = (long)tmp[0] << 24 | (long)tmp[1] << 16 | (long)tmp[2] << 8 | (long)tmp[3];
    // name
    fread(tmp, sizeof(char), 4, fp);
    strncpy(b->name, tmp, 4);

    return 1;
}

char IsFullBox(FILE* fp, box* b)
{
    if(!IsContainerBox(fp, b)) return 1;
    else return 0;
}
// this is to analyse whether the box is a container box
// return the count of contained box

//     FILE* set at the box's start + size (box 's end)
int IsContainerBox(FILE* fp, box* b)
{
    int result;
    unsigned long size;
    unsigned char tmp[4];

    int boxpos_start = b->pos_start;
    int boxSize = b->size;
    
    
    if(!strncmp(b->name, "mdat", 4)) 
    {
        fseek(fp, boxpos_start + boxSize, SEEK_SET);
        return 0;
    }

    while(ftell(fp) < boxpos_start + boxSize)
    {
        fread(tmp, sizeof(char), 4, fp);
        size = (long)tmp[0] << 24 | (long)tmp[1] << 16 | (long)tmp[2] << 8 | (long)tmp[3];
        fseek(fp, -4L, SEEK_CUR); // back -4L to at the child's start box
        fseek(fp, size, SEEK_CUR);// forward child's size
        result++;
        if(size == 0) {result = 0; break;}
    }
    if(ftell(fp) != boxpos_start + boxSize || boxSize == 8L || size == 0) result = 0;

    // if(result)
    //     fseek(fp, boxpos_start + 8L, SEEK_SET);
    // else
        fseek(fp, boxpos_start + boxSize, SEEK_SET);

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
void DataPareser(FILE* fp, tree_node* tn, box_parser_environment* bpe)
{
    unsigned char entry_count[4];// the entry's number(in char type-----the data in file)
    uint32 entryCount;           // the entry's number
    unsigned char tmp[4];

    box *b = &tn->item;
    char* name = tn->item.name;

    box btmp;
    // sample structure

    
    
    // 4(size) + 4(name) + 4(version 1 + flags 3)
    // so skip forward 12L at the data's start postion
    fseek(fp, b->pos_start + 12L, SEEK_SET);


    // only when trak need parse, then parse it
    if(bpe->info_trak[bpe->index_raw] <= 10) return;
    if(!strncmp(name, "stsd", 4))
    {
        avcC* data_avcC = (avcC*)malloc(sizeof(avcC));
        avc1* data_avc1 = (avc1*)malloc(sizeof(avc1));
        box_data* bdata_avc1 = (box_data*)malloc(sizeof(box_data));
        box_data* bdata_avcC = (box_data*)malloc(sizeof(box_data));
        
        // connect the fulldata of box's data
        bdata_avc1->full_data = data_avc1;
        bdata_avcC->full_data = data_avcC;

        // 4 uchar of entry's count
        fread(tmp, sizeof(char), 4L, fp);
        entryCount = ChangeCharARToNumber(tmp, 4);

        stsd* sample = (stsd*)malloc(sizeof(stsd)*entryCount);
        sample->sample_count = entryCount;

        //--------------------------------------------------------------------
        //  avc1
        //-------------------------
        // read avc1 box base info
        ReadBoxInfo(fp, &btmp);
        // insert avc1 into stsd
        tn = Tree_InsertItem(tn, &btmp, true);      //tn will be set at "avc1" box, while b is at stsd
        // connect box's data to box item
        tn->item.common_data = bdata_avc1;

        fread_m(&data_avc1->reserved               , fp, 1, 6);
        fread_m(&data_avc1->data_reference_index   , fp, 0, 2);
        fread_m(&data_avc1->pre_defined_0          , fp, 0, 2);
        fread_m(&data_avc1->reserved_2             , fp, 0, 2);
        fread_m(&data_avc1->pre_defined            , fp, 0, 12);
        fread_m(&data_avc1->width                  , fp, 0, 2);
        fread_m(&data_avc1->height                 , fp, 0, 2);
        fread_m(&data_avc1->horizons_res           , fp, 0, 4);
        fread_m(&data_avc1->vertical_res           , fp, 0, 4);
        fread_m(&data_avc1->reserved_3             , fp, 0, 4);
        fread_m(&data_avc1->frame_count            , fp, 0, 2);
        fread_m(&data_avc1->compress_name          , fp, 0, 32);
        fread_m(&data_avc1->bit_depth              , fp, 0, 2);
        fread_m(&data_avc1->pre_defined_2          , fp, 0, 2);
        //--------------------------------------------------------------------
        
        //--------------------------------------------------------------------
        //  avcC
        //-------------------------
        // read avcC's size
        ReadBoxInfo(fp, &btmp);
        tn = Tree_InsertItem(tn, &btmp, true);
        // connect box's data to box item
        tn->item.common_data = bdata_avcC;
        
        // read avcC's data
        fread_m(&data_avcC->configuration              , fp, 0, 1);
        fread_m(&data_avcC->avcProfileIndication       , fp, 0, 1);
        fread_m(&data_avcC->profile_compatibility      , fp, 0, 1);
        fread_m(&data_avcC->lengthSizeMinusOne         , fp, 0, 1);
        fread_m(&data_avcC->AVCLevelIndication         , fp, 0, 1);
        //--------------------------------------------------------------------
        
        
        // read SPS
        fread_m(&data_avcC->numOfSequenceParametersSets, fp, 0, 1);
        data_avcC->numOfSequenceParametersSets &= (0x1f);
        printf("number of SPS sets: %08d", data_avcC->numOfSequenceParametersSets);
        for (size_t i = 0; i < data_avcC->numOfSequenceParametersSets; i++)
        {
            fread_m(&data_avcC->sPS.length, fp, 0, 2);
            data_avcC->sPS.data = (char*)malloc(data_avcC->sPS.length * data_avcC->numOfSequenceParametersSets);
            fread(data_avcC->sPS.data, sizeof(char), data_avcC->sPS.length * data_avcC->numOfSequenceParametersSets, fp);
            printf("    first char is : [%2x]\n", data_avcC->sPS.data[i]);
        }
        // read PPS
        fread_m(&data_avcC->numOfPictureParametersSets, fp, 0, 1);
        printf("number of PPS sets: %08d", data_avcC->numOfPictureParametersSets);
        for (size_t i = 0; i < data_avcC->numOfPictureParametersSets; i++)
        {
            fread_m(&data_avcC->pPS.length, fp, 0, 2);
            data_avcC->pPS.data = (char*)malloc(data_avcC->pPS.length * data_avcC->numOfPictureParametersSets);
            fread(data_avcC->pPS.data, sizeof(char), data_avcC->pPS.length * data_avcC->numOfPictureParametersSets, fp);
            printf("    first char is : [%2x]\n", data_avcC->pPS.data[i]);
        }
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
        b->common_data->full_data = sample;
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
            // printf("stss:ID: %ld, sample_number : %d\n", i, sample[i].sample_number);
        }

        b->common_data->full_data = sample;
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
        b->common_data->full_data = sample;
    }
    // sample to chunk table
    else if(!strncmp(name, "stsc", 4))
    {
        fread(entry_count, sizeof(char), 4, fp);
        entryCount = ChangeCharARToNumber(entry_count, 4);

        stsc* sample = (stsc*)malloc(sizeof(stsc)*entryCount);

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
        b->common_data->full_data = sample;
    }
    // all sample count and the size of every sample
    else if(!strncmp(name, "stsz", 4))
    {
        stsz* sampleMultiple; // the result
        stsz sample;          // tmp stsz to store info
        
        // sample size 
        fread(tmp, sizeof(char), 4L, fp);
        sample.sample_size = ChangeCharARToNumber(tmp, 4);
        // sample count(the count of sample in trak)
        fread(tmp, sizeof(char), 4L, fp);
        sample.sample_count = ChangeCharARToNumber(tmp, 4);

        // data[0] is used to store the every size or all count 

        // every sampe have different size 
        if(sample.sample_size == 0)
        {
            printf("stsz: %d sample have different size: %d\n", sample.sample_count, sample.sample_size);
            sampleMultiple = (stsz*)malloc(sizeof(stsz)*(sample.sample_count + 1));
            sampleMultiple[0].sample_count = sample.sample_count;
            sampleMultiple[0].sample_size = sample.sample_size;
            for (int i = 1; i <= sample.sample_count; i++)
            {
                fread(tmp, sizeof(char), 4, fp);
                sampleMultiple[i].sample_size = ChangeCharARToNumber(tmp, 4);
                sampleMultiple[i].sample_count = i;
                // printf("stsz: sample count ID: %d, sample size: %d\n", i, sampleMultiple[i].sample_size);
            }
        }
        else // all sample have the same size
        {
            printf("stsz: %d sample have same size: %d\n", sample.sample_count, sample.sample_size);
            sampleMultiple = (stsz*)malloc(sizeof(stsz));
            sampleMultiple[0].sample_count = sample.sample_count;
            sampleMultiple[0].sample_size = sample.sample_size;
        }

        b->common_data->full_data = sampleMultiple;
    }
    // chunk offset (in file)
    else if(!strncmp(name, "stco", 4))
    {
        fread(entry_count, sizeof(char), 4L, fp);
        entryCount = ChangeCharARToNumber(entry_count, 4);
        printf("stco: chunk count :%d\n", entryCount);
        stco* chunk = (stco*)malloc(b->size - 16L);

        for (size_t i = 0; i < entryCount; i++)
        {
            fread(tmp, sizeof(char), 4, fp);
            chunk[i].chunk_offset = ChangeCharARToNumber(tmp, 4);
            // printf("stco: chunk ID: %3ld, Offset: 0x%10X\n", i, chunk[i].chunk_offset);
        }
        b->common_data->full_data = chunk;
    }
    b->common_data->entrycount = (int)entryCount;
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
