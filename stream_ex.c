#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<unistd.h>
#include<string.h>

typedef struct bound {
   char *ref;
   char *roller;
   size_t len; 
}bound;


void ver_arg_num(int argc){
    if(argc!=2){
        fprintf(stderr,"No file specified!\n");
        exit(1);
    }
    return;
}

void ver_file_acc(char *fpath){
    if(access(fpath,R_OK)!=0){
        fprintf(stderr,"Can't access file %s!\n",fpath);
        exit(1);
    }
    return;
}

FILE *open_file(char* fpath, char *mode){
    FILE *fp = NULL;
    fp = fopen(fpath,mode);
    if(fp==NULL){
        fprintf(stderr,"Can't open file %s in mode %s!\n",fpath,mode);
        exit(1);
    }
    return fp;
}

void shift_bound(bound *in){
    if(in->len > 1){
        for(size_t i=0;i<(in->len-1);i++){
            in->roller[i]=in->roller[i+1];
        }
    }
    else{
        in->roller[0]=0x00;
    }
    return;
}

int check_bound(bound *in){
    //Default State found (0)
    int res=0;
    for(size_t i=0; i<in->len; i++){
        if(in->ref[i]!=in->roller[i]){
            res=1;
            break;
        }
    }
    return res;
}

int update_bound(bound *in, int cursor){
    
    //Default state not found (1)
    int res = 1;

    shift_bound(in);
    in->roller[in->len-1]=cursor;
    
    if(check_bound(in)==0)
        res = 0;

    return res;
}

void reset_bound(bound *in){
    memset(in->roller,0x00,in->len);
}

int process_stream(FILE* fp, uint16_t obj_num, bound *end){

    //Reset End Roller
    reset_bound(end);

    //Create output file for stream object
    char obj_name[15];
    memset(obj_name,0x00,15);
    snprintf(obj_name,15,"stream_%05d",obj_num);
    FILE *obj = open_file(obj_name,"w");

    int cursor=0;
    uint32_t pos = 0;
    while (cursor!=EOF){
        cursor = getc(fp);
        if(cursor!=EOF){
            //skip 1 or 2 0X0A 0X0D at the start transfer to new file
            if(!(pos<2 && (cursor==0x0A || cursor==0X0D))){
                fputc(cursor,obj);
            }
            //check roller
            if(update_bound(end,cursor)==0){
                //Match --> end object capture
                fprintf(stdout,"Found end of stream %05d!\n",obj_num);
                break;
            }
            pos++;
        }
    }
    fclose(obj);
    //Transfer out possible EOF
    return cursor;
}

void process_file(FILE *fp, bound *start, bound *end){

    reset_bound(start);

    int cursor=0;
    int obj_count=0;

    while (cursor!=EOF){
        cursor = getc(fp);
        if(cursor!=EOF && update_bound(start,cursor)==0){
            //Match --> start object capture
            fprintf(stdout,"Found start of stream %05d!\n",obj_count);
            cursor = process_stream(fp,obj_count,end);
            obj_count++;
        }
    }  
    return;
}

//Compile: gcc streamex.c -o streamex -Wall -std=gnu99
int main(int argc, char **argv){

    //Check input
    ver_arg_num(argc);
    ver_file_acc(argv[1]);

    FILE *in_pdf = open_file(argv[1],"r");

    //Declare Delimiters
    char start_ref[]={'s','t','r','e','a','m'};
    char start_roller[6];
    char end_ref[]={'e','n','d','s','t','r','e','a','m'};
    char end_roller[9];
    bound start = {.ref = start_ref, .roller = start_roller, .len = 6};
    bound end = {.ref = end_ref, .roller = end_roller, .len = 9};

    //Extract Streams
    process_file(in_pdf, &start, &end);

    fclose(in_pdf);

    return 0;
}