#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return BF_ERROR;        \
  }                         \
}

 //tha to xrisimopoiiso gia tis sinartiseis pou epistrefoun pointer

#define CALL_NULL(call)     \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK){       \
    BF_PrintError(code);    \
    return NULL;            \
  }                         \
}

static HP_info hp_info_st;                  //domi 

int HP_CreateFile(char *fileName){
  int fd;                                    //anagnoristiko tou arxeiou mou
  BF_Block* fblock;                         //
  void* data_start;
  void* data_move;
  int err;
  
  CALL_BF(BF_CreateFile(fileName));         //dimiourgia arxeiou
  CALL_BF(BF_OpenFile(fileName, &fd));      //anoigma tou neou arxeiou 

  BF_Block_Init(&fblock);                     //deusmeuoume xoro kai arxikopoioume ton prosorino ptr prokeimenu na parei ta xaraktiristika enos block

  CALL_BF(BF_AllocateBlock(fd,fblock));       //desmeuoume ena block gia to arxeio fd, to neo block fortonetai stin antistoixi selida tis endiamesis (fblock)
  data_start = BF_Block_GetData(fblock);      //vazouμε to deikti data na deixnei stin arxi ton dedomenon tou protou block
  data_move = data_start;                     //xrisimopoio deutero deikti o opoios tha kineitai kata to dokoun
  HP_info hp_info;                            //ftiaxnoume mia prosorini domi gia to hp_info
  hp_info.Last_block_Id=0;                    //dinoume tis katalliles times sta pedia tou hp_info
  hp_info.Max_Records_In_Block = BF_BLOCK_SIZE / sizeof(Record);    
  memcpy(data_move, &hp_info, sizeof(hp_info));       //grafoume sto block ti domi hp_info, ksekinontas apo to simeio data_move

  data_move = data_move + hp_info.Max_Records_In_Block * sizeof(Record);  //vazoume to data_move na deixnei sto telos tou block
  HP_block_info hpbi;                         //dimiurgoume ena block info
  hpbi.Num_Of_Records = 0;                    //dinoume katalliles times
  hpbi.Next_Block = NULL;
  hpbi.Block_Id = 0;                      
  memcpy(data_move, &hpbi, sizeof(HP_block_info));  //to grafoume sto telos tu block

  BF_Block_SetDirty(fblock);                  //to kanoume dirty kai to afino pinned mexri to telos tu programmatos
  CALL_BF(BF_UnpinBlock(fblock));             //unpin gia na mporei na figei apo tin endiamesi  
  BF_Block_Destroy(&fblock);                  //to kanoume destroy
  CALL_BF(BF_CloseFile(fd));                  //kai kleinoume to arxeio mias kai teliosame ti dimiurgia

  return 0;                                           
}

HP_info* HP_OpenFile(char *fileName, int *file_desc){
  
  BF_Block* fblock;              //pointer pu tha xrisimopoiisoume gia to proto block tou arxeiou
  BF_Block_Init(&fblock);        //
  HP_info* info_ptr;
  void* data_start;
  void* data_move;

  CALL_NULL(BF_OpenFile(fileName, file_desc));        //anoigei to arxeio fileName kai epistrefei to anagnoristiko tou stin fd 
  CALL_NULL(BF_GetBlock(*file_desc, 0, fblock));     //fortonoume to proto block tu arxeiou sorou
  data_start = BF_Block_GetData(fblock);              //vazoume ta dedomena tou block sto data_start
  data_move = data_start;       
  
  memcpy(&hp_info_st, data_move, sizeof(HP_info));    //kanoume egrafi sti statiki gia na apofigume ti xrisi malloc

  return &hp_info_st;
}


int HP_CloseFile(int file_desc, HP_info* hp_info){
  BF_Block* fblock;              //pointer pu tha xrisimopoiisoume gia to proto block tou arxeiou
  BF_Block_Init(&fblock);        //arxikopoiisi tou protou block  
  int* fd = &file_desc;

  CALL_BF(BF_GetBlock(*fd, 0, fblock));  //vazoume to deikti fblock na deixnei sto proto block

  CALL_BF(BF_UnpinBlock(fblock));   //unpin 
  BF_Block_Destroy(&fblock);         //destroy

  CALL_BF(BF_CloseFile(file_desc)); 

  return 0;
}

int HP_InsertEntry(int file_desc, HP_info* hp_info, Record record){
  
  int num_blocks;
  char* data_start;       //pointer stin arxi tou block
  char* data_end;         //pointer pou metakinite mesa sto block
  int return_id;
  CALL_BF(BF_GetBlockCounter(file_desc, &num_blocks));        //pairnume to anagnoristiko meso tis blockcounter
  int last_block_id = num_blocks -1; 
  BF_Block* lblock;         //block pou tha fortosoume ta dedomena ta dedomena tou teleutaiou block
  BF_Block_Init(&lblock);
  CALL_BF(BF_GetBlock(file_desc, last_block_id, lblock));   

  data_start = BF_Block_GetData(lblock);    //vazoume ton data_start na deixnei stin arxi tou block
  data_end = data_start;                    //to idio kai ston data_end

  data_end = data_end + hp_info->Max_Records_In_Block * sizeof(Record);   //metakinume to pointer sto telos ton eggrafon
  HP_block_info block_info; 
  memcpy(&block_info, data_end, sizeof(HP_block_info));                   //grafume ti domi block_info ekei pou deixnei o data_end

  if (block_info.Num_Of_Records == hp_info->Max_Records_In_Block || last_block_id == 0) { //an to block einai gemato i an exei molis dimiurgithei to arxeio
    BF_Block* new_block;            //desmeuoume  kai arxikopoioume neo block
    BF_Block_Init(&new_block);   
    char* data_start1;
    char* data_end1;  

    CALL_BF(BF_AllocateBlock(file_desc, new_block));  
    data_start1 = BF_Block_GetData(new_block);      //vazume ton data_start1 na deixnei stin arxi ton dedomenon tu neou block
    data_end1 = data_start1;                        //to idio kai ton data_end1
    memcpy(data_end1, &record, sizeof(Record));     //grafume tin eggrafh stin arxi tou block

    data_end1 = data_end1 + hp_info->Max_Records_In_Block * sizeof(Record); //metakinume to pointer sto telos ton eggrafon
    HP_block_info block_info1;
    block_info1.Num_Of_Records = 1;     //dinume katallili timi sto pedio tiw block_info1
    memcpy(data_end1, &block_info1, sizeof(HP_block_info));   //grafume ti domi block_info ekei pou deixnei o data_end
    return_id = block_info1.Block_Id;   //kratame to id tou block opou egine i eggrafh
    BF_Block_SetDirty(new_block);
    CALL_BF(BF_UnpinBlock(new_block));
    BF_Block_Destroy(&new_block);

    } else {  //allios an to block exei xoro gia tin eggrafi

    data_end = data_start + block_info.Num_Of_Records * sizeof(Record);  //metakinume to pointer sto telos ton eggrafon
    memcpy(data_end, &record, sizeof(Record));      //grafume tin eggrafi ekei pou deixnei o data_end
    block_info.Num_Of_Records++;    
    data_end = data_start + hp_info->Max_Records_In_Block * sizeof(Record);   //metakinume to pointer sto telos ton eggrafon
    memcpy(data_end, &block_info, sizeof(HP_block_info));   //grafume ti domi block_info ekei pou deixnei o data_end
    return_id = block_info.Block_Id;  //kratame to id tou block opou egine i eggrafh
    BF_Block_SetDirty(lblock);
  }
  CALL_BF(BF_UnpinBlock(lblock));
  BF_Block_Destroy(&lblock);
  return return_id; //epistrefume to id tou block sto opoio egine i eggrafi
}

int HP_GetAllEntries(int file_desc, HP_info* hp_info, int value){
  
  int readed = 0;   //edo tha kratisume ta block pou diavastikan
  int blocks_num;   //to sinolo ton block
  CALL_BF(BF_GetBlockCounter(file_desc, &blocks_num));  //kratame to sinolo ton block meso tis getblockcounter
  char* data_start;
  char* data_end;

  for (int i = 1; i < blocks_num; i++) {
    BF_Block* current_block;            //block pu tha fortonume ta dedomena kathe block tou arxeiou sorou
    BF_Block_Init(&current_block);
    CALL_BF(BF_GetBlock(file_desc, i, current_block));  
    data_start = BF_Block_GetData(current_block); //vazume ton data_start na deixnei sta dedomena tu kathe block
    data_end = data_start;

    data_end = data_end + hp_info->Max_Records_In_Block * sizeof(Record); 
    HP_block_info hpbi;
    memcpy(&hpbi, data_end, sizeof(HP_block_info)); 
    
    for (int j = 0; j < hpbi.Num_Of_Records; j++) {
      Record record;
      data_end = data_start + j * sizeof(Record);
      memcpy(&record, data_end, sizeof(Record));
      if (record.id == value) {     //an to record.id einai auto pou psaxnume (value)
        Record temporary;           //dimiurgume mia prosorini domi record
        char *id = malloc(3 * sizeof(char));    
        strncpy(id, "ID", 3 * sizeof(char));        //grafume stin prosorini domi tis onomasies ton pedion
        strncpy(temporary.name, "Name", 15 * sizeof(char));
        strncpy(temporary.surname, "Surname", 20 * sizeof(char));
        strncpy(temporary.city, "City", 20 * sizeof(char));

        printf("%-10s%-15s%-20s%-20s\n", id, temporary.name,  //kai tis ektiponume
          temporary.surname, temporary.city);
        free(id);
        printf("%-10d%-15s%-20s%-20s\n", record.id, record.name, record.surname, record.city); //ektiponume tis times ton pedion pou vrikame
        CALL_BF(BF_UnpinBlock(current_block));
        BF_Block_Destroy(&current_block);
        return readed;
      }
    }
    CALL_BF(BF_UnpinBlock(current_block));
    BF_Block_Destroy(&current_block);
    readed++; 
  }

  return readed;  //epistrefume ton arithmo ton block pou diavastikan
}
 

