/*
 * unsafe.c: a simple program which is vulnerable to stack smashing/
 *   overflow
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#include <gccore.h>
#include <sdcard/wiisd_io.h>
#include <fat.h>

#define FILENAME "unsafe_savefile.dat"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

static u8 SysArea[CARD_WORKAREA] ATTRIBUTE_ALIGN(32);

void memcard_removed(s32 chn, s32 result){
  /* These are included to ensure that unsigned code can call them later
   *
   * I acknowledge that this may be a bit of a copout, however this
   *   project and homebrew loader exist as a practice in exploit
   *   development for me, meaning so long as the vulnerable app
   *   is semi-realistic, my primary goal will have been achieved.
   */
  fatInitDefault();
  fatMountSimple("sd", &__io_wiisd);
  FILE *fp = fopen("sd:/app.dol", "rb");
  fseek(fp, 0, SEEK_END);
  ftell(fp);
  fgetc(fp);

  CARD_Unmount(chn);
}

void memcard_load(){
  char file_buf[4096];
  int err, hit = 0;
  unsigned int sec_size;
  card_dir cd;
  card_file cf;

  CARD_Init("UNSF", "00");
  err = CARD_Mount(CARD_SLOTA, SysArea, memcard_removed);
  if(err >= 0){
    CARD_GetSectorSize(CARD_SLOTA, &sec_size);
    err = CARD_FindFirst(CARD_SLOTA, &cd, true);

    while(err != CARD_ERROR_NOFILE){
      err = CARD_FindNext(&cd);
      if(strcmp(FILENAME, (char*)cd.filename) == 0){ hit = 1; }
    }

    if(hit){
      err = CARD_Open(CARD_SLOTA, FILENAME, &cf);
      if(err == 0){
        CARD_Read(&cf, file_buf, sec_size, 0); /* !!! sec_size (likely 8192) > sizeof(file_buf) (4096) */
        CARD_Close(&cf);
      } else {
        printf("Failed to open file on memory card, error code %i\n", err);
      }
    } else {
      err = CARD_Create(CARD_SLOTA, FILENAME, sec_size, &cf);
      if(err == 0){
        printf("Data not present, writing to memory card.\n\n");
        sprintf(file_buf, "This is filler text, loaded from the memory card in Slot A.");
        CARD_Write(&cf, file_buf, sec_size, 0);
        CARD_Close(&cf);
      } else {
        printf("Failed to create file on memory card, error code %i\n", err);
        exit(err);
      }
    }

    CARD_Unmount(CARD_SLOTA);
  } else {
    printf("Failed to open memory card in Slot A, error code %i\n", err);
    exit(err);
  }

  printf("Data read from memory card:\n\n\"%s\"\n\n", file_buf);
}

int main(){
	VIDEO_Init();
	PAD_Init();

	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	printf("\x1b[2;0HLoading data from memory card.\n\n");
  
  usleep(500000);

  memcard_load();

  printf("Exiting...\n");
  usleep(5000000);
  exit(0);

	return 0;
}
