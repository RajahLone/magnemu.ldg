
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <gem.h>
#include <ldg.h>

#include "defs.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/* structures */

/* global variables */

#define SAVEDGAME_NAME_SIZE 384
#define TEXT_BUFFER_SIZE 1024
#define STATUS_BUFFER_SIZE 128
#define STATUS_WIDTH 78
#define ACTION_BUFFER_SIZE 256

static char savedgame_ptr[SAVEDGAME_NAME_SIZE];

static char* msg_fatal = NULL;
static int32_t is_fatal = 0;

static char text_buffer_ptr[TEXT_BUFFER_SIZE];
static uint16_t text_buffer_pos = 0;

static char status_buffer_ptr[STATUS_BUFFER_SIZE];
static uint16_t status_buffer_pos = 0;
static int32_t status_updated = 0;

static char action_buffer_ptr[ACTION_BUFFER_SIZE];
static uint16_t action_buffer_pos = 0;

static char picture_current_id = -1;
static unsigned char* picture_rawdata = NULL;
static uint16_t picture_width = 0;
static uint16_t picture_height = 0;
static uint16_t picture_palette[16];

/* implemented functions */

void ms_fatal(char* txt) { is_fatal = 1; msg_fatal = txt; }

void ms_flush(void)
{
  if (text_buffer_pos == 0) { return; }
  
  text_buffer_pos = 0;
  memset(text_buffer_ptr, 0, TEXT_BUFFER_SIZE);
}
void ms_putchar(uint8_t c)
{
  switch (c)
  {
    case 0x0a: text_buffer_ptr[text_buffer_pos++]= '\0'; ms_flush(); break;
    case 0x08: if (text_buffer_pos > 0) { text_buffer_pos--; } break;
    default: text_buffer_ptr[text_buffer_pos++] = c;
  }
}
void ms_statuschar(unsigned char c)
{
  if (c == 0x0a)
  {
    status_buffer_pos = 0;
    status_buffer_ptr[status_buffer_pos] = 0;
  }
  else if (c == 0x09)
  {
    while (status_buffer_pos < STATUS_WIDTH - 11)
    {
      status_buffer_ptr[status_buffer_pos] = 0x20;
      status_buffer_pos++;
    }
  }
  else if (status_buffer_pos < STATUS_BUFFER_SIZE - 1)
  {
    status_buffer_ptr[status_buffer_pos] = c;
    status_buffer_pos++;
  }
  
  status_updated = 1;
}

uint8_t ms_getchar(type8 trans)
{
  uint16_t c = action_buffer_ptr[action_buffer_pos++];
  
  if (c == 0x0a || c == 0)
  {
    c = 0x0a;
    action_buffer_pos = 0;
    action_buffer_ptr[0] = 0;
  }

  return (uint8_t)c;
}

void ms_showpic(type32 c, type8 mode)
{
  unsigned char is_animation = 0;

  if (mode > 0)
  {
    if (c == picture_current_id) { return; }

    picture_current_id = -1;
      
    picture_rawdata = ms_extract(c, &picture_width, &picture_height, picture_palette, &is_animation);
   
    if (picture_rawdata) { picture_current_id = c; }
    
    // TODO: handle animations
  }
  else
  {
    picture_current_id = -1;
    picture_rawdata = NULL;
    picture_width = 0;
    picture_height = 0;
  }
}

void ms_playmusic(type8 *midi_data, type32 length, type16 tempo)
{
  // TODO: if MIDI available?
}
type8 ms_showhints(struct ms_hint * hints)
{
  // TODO: openHints(hints)?
  return 1;
}

unsigned char ms_load_file(type8s *name, type8 *ptr, type16 size)
{
  FILE *fh;
  
  if (!(fh = fopen(savedgame_ptr, "rb"))) { return 1; }
  if (fread(ptr, 1, size, fh) != size) { return 1; }
  fclose(fh);

  return 0;
}
unsigned char ms_save_file(type8s *name, type8 *ptr, type16 size)
{
  FILE *fh;
  
  if (!(fh = fopen(savedgame_ptr, "wb"))) { return 1; }
  if (fwrite(ptr, 1, size, fh) != size) { return 1; }
  fclose(fh);
  
  return 0;
}

/* functions */

uint32_t CDECL gms_seed(uint32_t seed) { ms_seed(seed); return (uint32_t)1; }

uint32_t CDECL gms_set_saved_game(const char* name)
{
  memset(savedgame_ptr, 0, SAVEDGAME_NAME_SIZE);
  memcpy(savedgame_ptr, name, MIN(strlen(name), SAVEDGAME_NAME_SIZE - 1));
  return (uint32_t)1;
}

uint32_t CDECL gms_init(char* name, char* gfxname, char* hntname, char* sndname)
{
  picture_current_id = -1;
  return (uint32_t)ms_init(name, gfxname, hntname, sndname);
}
uint32_t CDECL gms_rungame() { return (uint32_t)ms_rungame(); }
uint32_t CDECL gms_freemem() { ms_freemem(); return (uint32_t)1; }

uint32_t CDECL gms_has_text() { if (text_buffer_pos > 0) { return (uint32_t)1; } return (uint32_t)0; }
char* CDECL gms_get_text() { return text_buffer_ptr; }
uint32_t CDECL gms_is_status_updated() { return (uint32_t)status_updated; }
char* CDECL gms_get_status() { status_updated = 0; return status_buffer_ptr; }
uint32_t CDECL gms_send_string(const char* action)
{
  memset(action_buffer_ptr, 0, ACTION_BUFFER_SIZE);
  memcpy(action_buffer_ptr, action, MIN(strlen(action), ACTION_BUFFER_SIZE - 1));
  action_buffer_pos = 0;
  return (uint32_t)1;
}

uint32_t CDECL gms_get_picture_current_id() { return (uint32_t)picture_current_id; }
unsigned char* CDECL gms_get_picture_rawdata() { return picture_rawdata; }
uint32_t CDECL gms_get_picture_width() { return (uint32_t)picture_width; }
uint32_t CDECL gms_get_picture_height() { return (uint32_t)picture_height; }
uint16_t* CDECL gms_get_picture_palette() { return picture_palette; }

uint32_t CDECL gms_is_running() { return (uint32_t)ms_is_running(); }
uint32_t CDECL gms_is_magwin() { return (uint32_t)ms_is_magwin(); }

uint32_t CDECL gms_status() { ms_status(); return (uint32_t)1; }
uint32_t CDECL gms_stop() { ms_stop(); return (uint32_t)1; }
uint32_t CDECL gms_count() { return (uint32_t)ms_count(); }

uint32_t CDECL gms_is_fatal() { return is_fatal; }
char* CDECL gms_get_fatal() { return msg_fatal; }


/* populate functions list and info for the LDG */

PROC LibFunc[] =
{
  {"gms_seed", "uint32_t CDECL gms_seed(uint32_t seed);\n", gms_seed},
  {"gms_set_saved_game", "uint32_t CDECL gms_set_saved_game(const char* name);\n", gms_set_saved_game},

  {"gms_init", "uint32_t CDECL gms_init(char* name, char* gfxname, char* hntname, char* sndname);\n", gms_init},
  {"gms_rungame", "uint32_t CDECL gms_rungame();\n", gms_rungame},
  {"gms_freemem", "uint32_t CDECL gms_freemem();\n", gms_freemem},

  {"gms_has_text", "uint32_t CDECL gms_has_text();\n", gms_has_text},
  {"gms_get_text", "char* CDECL gms_get_text();\n", gms_get_text},
  {"gms_is_status_updated", "uint32_t CDECL gms_is_status_updated();\n", gms_is_status_updated},
  {"gms_get_status", "char* CDECL gms_get_status();\n", gms_get_status},
  {"gms_send_string", "uint32_t CDECL gms_send_string(const char* action);\n", gms_send_string},

  {"gms_get_picture_current_id", "uint32_t CDECL gms_get_picture_current_id();\n", gms_get_picture_current_id},
  {"gms_get_picture_rawdata", "unsigned char* CDECL gms_get_picture_rawdata();\n", gms_get_picture_rawdata},
  {"gms_get_picture_width", "uint32_t CDECL gms_get_picture_width();\n", gms_get_picture_width},
  {"gms_get_picture_height", "uint32_t CDECL gms_get_picture_height();\n", gms_get_picture_height},
  {"gms_get_picture_palette", "uint16_t* CDECL gms_get_picture_palette();\n", gms_get_picture_palette},

  {"gms_is_running", "uint32_t CDECL gms_is_running();\n", gms_is_running},
  {"gms_is_magwin", "uint32_t CDECL gms_is_magwin();\n", gms_is_magwin},

  {"gms_status", "uint32_t CDECL gms_status();\n", gms_status},
  {"gms_stop", "uint32_t CDECL gms_stop();\n", gms_stop},
  {"gms_count", "uint32_t CDECL gms_count();\n", gms_count},

  {"gms_is_fatal", "uint32_t CDECL gms_is_fatal();\n", gms_is_fatal},
  {"gms_get_fatal", "char* CDECL gms_get_fatal();\n", gms_get_fatal},
};

LDGLIB LibLdg[] = { { 0x0001, 22, LibFunc, "Magnetic v2.3 (c) Niclas Karlsson, 1997-2008", 1} };

/*  */

int main(void)
{
  ldg_init(LibLdg);
  return 0;
}
