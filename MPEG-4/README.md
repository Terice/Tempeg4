# Parser of MPEG-4's container  

a simple parser of MPEG-4's structure & data writer



# input & output
input : a file which is made in MPEG-4 container.  
output(terminal): the structure of this file that organize by "tree".  
output(file): h264 file stream(some problems).  
data  : organized by "tree"(a type of tree: left child, right brother).  

# compile option
make:  


# function using
complete using is in main.c  
like following code: 

```c
    fmpeg4 = InitMpeg4();
    ParserContainer_mpeg4(fp, fmpeg4);
    // write the data from the "fp", 
    // follow the structure of "fmpeg4"
    // select the trak "0"
    // write into "fp_out"
    DataWriter_mpeg4_h264(fp, fmpeg4, 0, fp_out);
    DeleteMpeg4(fmpeg4);
    
```

# other