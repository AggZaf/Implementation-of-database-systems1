#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "hp_file.h"

#define RECORDS_NUM 1000 // you can change it if you want
#define FILE_NAME "data.db"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

int main() {
  BF_Init(LRU);

  CALL_OR_DIE(HP_CreateFile(FILE_NAME));
  int file_desc;
  HP_info* hp_info2=HP_OpenFile(FILE_NAME, &file_desc);

  Record record;
  srand(12569874);
  int r;
  int x;
  printf("Insert Entries\n");
  for (int id = 1; id <= RECORDS_NUM; id++) {
    record = randomRecord();
    printRecord(record);
    x = HP_InsertEntry(file_desc,hp_info2, record);
  }
  printf("RUN PrintAllEntries\n");
  for (int i=1; i<= RECORDS_NUM ; i++){
  printf("\nSearching for: %d\n",i);
  HP_GetAllEntries(file_desc,hp_info2, i);
  }


  CALL_OR_DIE(HP_CloseFile(file_desc,hp_info2));
}
