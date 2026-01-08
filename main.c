
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <ldg.h>

#include "defs.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/* global variables */

#define FILE_BUFFER_SIZE 384
#define TEXT_BUFFER_SIZE 4096
#define STATUS_BUFFER_SIZE 78
#define ACTION_BUFFER_SIZE 256

static char* msg_fatal = NULL;
static int32_t is_fatal = 0;

static char load_name[FILE_BUFFER_SIZE];
static char save_name[FILE_BUFFER_SIZE];

static char text_buffer_ptr[TEXT_BUFFER_SIZE];
static uint16_t text_buffer_pos = 0;
static uint16_t text_has_endline = 0;
static uint16_t text_has_prompt = 0;

static char status_buffer_ptr[STATUS_BUFFER_SIZE];
static uint16_t status_buffer_pos = 0;
static uint16_t status_updated = 0;

static char action_buffer_ptr[ACTION_BUFFER_SIZE];
static uint16_t action_buffer_pos = 0;

static char picture_current_id = -1;
static unsigned char* picture_rawdata = NULL;
static uint16_t picture_width = 0;
static uint16_t picture_height = 0;
static uint16_t picture_palette[16];

/* implemented functions */

void ms_fatal(char* txt) { is_fatal = 1; msg_fatal = txt; }

unsigned char ms_load_file(type8s *name, type8 *ptr, type16 size)
{
	FILE *fh;
	
  if (load_name[0] == 0) { return 1; }
 
  fh = fopen(load_name, "rb");

  if (fh == NULL) { return 1; }

	if (fread(ptr, 1, size, fh) != size) { return 1; }

	fclose(fh);

	return 0;
}

unsigned char ms_save_file(type8s *name, type8 *ptr, type16 size)
{
	FILE *fh;

  if (save_name[0] == 0) { return 1; }

  fh = fopen(save_name, "wb");
	
  if (fh == NULL) { return 1; }
	
  if (fwrite(ptr, 1, size, fh) != size) { return 1; }
	
  fclose(fh);
	
  return 0;
}

void ms_flush(void)
{
  if (text_buffer_pos == 0) { return; }
  
  text_buffer_pos = 0;
  text_has_endline = 0;
  
  memset(text_buffer_ptr, 0, TEXT_BUFFER_SIZE);
}
void ms_putchar(uint8_t c)
{
  if (text_buffer_pos < TEXT_BUFFER_SIZE)
  {
    switch(c)
    {
      case 0x00:
        text_buffer_ptr[text_buffer_pos] = c;
        break;
      case 0x08:
        if (text_buffer_pos > 0) { text_buffer_pos--; }
        break;
      case 0x0a:
        text_buffer_ptr[text_buffer_pos++] = c;
        text_has_endline = 1;
        break;
      case 0x3e:
        //text_buffer_ptr[text_buffer_pos++] = 0x3c;
        text_has_prompt = 1;
        break;
      default:
        text_buffer_ptr[text_buffer_pos++] = c;
    }
  }
}
void ms_statuschar(unsigned char c)
{
  if (c == 0x0a)
  {
    status_updated = 1;
  }
  else if (c == 0x09)
  {
    while (status_buffer_pos < STATUS_BUFFER_SIZE - 11)
    {
      status_buffer_ptr[status_buffer_pos] = 0x20;
      status_buffer_pos++;
    }
  }
  else if (status_buffer_pos < STATUS_BUFFER_SIZE)
  {
    status_buffer_ptr[status_buffer_pos] = c;
    status_buffer_pos++;
  }
}

uint8_t ms_getchar(type8 trans)
{
  uint8_t c = action_buffer_ptr[action_buffer_pos++];
  
  if (c == 0x0a || c == 0)
  {
    action_buffer_pos = 0;
    memset(action_buffer_ptr, 0, ACTION_BUFFER_SIZE);
  }

  return c;
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
type8 ms_showhints(struct ms_hint * hints)
{
  // TODO: handle hints
  return 1;
}
void ms_playmusic(type8 *midi_data, type32 length, type16 tempo)
{
  // TODO: if MIDI available?
}

/* functions */

uint32_t CDECL gms_init(char* name, char* gfxname, char* hntname, char* sndname)
{
  char *v_gfxname = NULL;
  char *v_hntname = NULL;
  char *v_sndname = NULL;
  
  text_buffer_pos = 0;
  memset(text_buffer_ptr, 0, TEXT_BUFFER_SIZE);
  text_has_endline = 0;
  text_has_prompt = 0;

  status_buffer_pos = 0;
  memset(status_buffer_ptr, 0, STATUS_BUFFER_SIZE);
  status_updated = 0;

  action_buffer_pos = 0;
  memset(action_buffer_ptr, 0, ACTION_BUFFER_SIZE);

  picture_current_id = -1;
  picture_rawdata = NULL;
  picture_width = 0;
  picture_height = 0;
  
  memset(load_name, 0, FILE_BUFFER_SIZE);
  memset(save_name, 0, FILE_BUFFER_SIZE);
  
  if (name == NULL) { return (uint32_t)0; }
  if (strlen(name) == 0) { return (uint32_t)0; }
  
  if (gfxname != NULL) { if (strlen(gfxname) > 0) { v_gfxname = gfxname; } }
  if (hntname != NULL) { if (strlen(hntname) > 0) { v_hntname = hntname; } }
  if (sndname != NULL) { if (strlen(sndname) > 0) { v_sndname = sndname; } }
  
  return (uint32_t)ms_init(name, v_gfxname, v_hntname, v_sndname);
}
uint32_t CDECL gms_rungame()
{
  uint32_t count = 4096;
  type8 running = 1;
  
  while(running && (count > 0) && (text_has_prompt == 0) /*&& (text_has_endline == 0)*/)
  {
    running = ms_rungame();
    count--;
  }
  
  return (uint32_t)1;
}
uint32_t CDECL gms_freemem() { ms_freemem(); return (uint32_t)1; }

uint32_t CDECL gms_has_text() { if (text_buffer_pos > 0) { return (uint32_t)1; } return (uint32_t)0; }
uint32_t CDECL gms_has_prompt() { return text_has_prompt; }
char* CDECL gms_get_text() { return text_buffer_ptr; }
uint32_t CDECL gms_flush_text() { ms_flush(); return (uint32_t)1; }

uint32_t CDECL gms_is_status_updated() { return (uint32_t)status_updated; }
char* CDECL gms_get_status() { return status_buffer_ptr; }
uint32_t CDECL gms_flush_status()
{
  status_updated = 0;
  status_buffer_pos = 0;
  memset(status_buffer_ptr, 0, STATUS_BUFFER_SIZE);
  
  return (uint32_t)1;
}

uint32_t CDECL gms_seed(uint32_t seed) { ms_seed(seed); return (uint32_t)1; }
uint32_t CDECL gms_send_string(const char* action)
{
  memset(action_buffer_ptr, 0, ACTION_BUFFER_SIZE);
  memcpy(action_buffer_ptr, action, MIN(strlen(action), ACTION_BUFFER_SIZE - 1));
  action_buffer_pos = 0;
  
  text_has_prompt = 0;

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

uint32_t CDECL gms_set_load_name(const char* name)
{
  if (name == NULL) { return (uint32_t)0; }
  if (strlen(name) == 0) { return (uint32_t)0; }
  if (strlen(name) > (FILE_BUFFER_SIZE -1)) { return (uint32_t)0; }

  strcpy(load_name, name);

  return (uint32_t)1;
}
uint32_t CDECL gms_load_game()
{
  if (text_has_prompt == 1 && action_buffer_pos == 0)
  {
    text_has_prompt = 0;
 
    action_buffer_ptr[action_buffer_pos++] = 0x4c; // L
    action_buffer_ptr[action_buffer_pos++] = 0x6f; // o
    action_buffer_ptr[action_buffer_pos++] = 0x61; // a
    action_buffer_ptr[action_buffer_pos++] = 0x64; // d
    action_buffer_ptr[action_buffer_pos++] = 0x23; // #
    action_buffer_ptr[action_buffer_pos++] = 0x0a; // \n
    
    return (uint32_t)1;
  }
  return (uint32_t)0;
}
uint32_t CDECL gms_set_save_name(const char* name)
{
  if (name == NULL) { return (uint32_t)0; }
  if (strlen(name) == 0) { return (uint32_t)0; }
  if (strlen(name) > (FILE_BUFFER_SIZE -1)) { return (uint32_t)0; }
  
  strcpy(save_name, name);

  return (uint32_t)1;
}
uint32_t CDECL gms_save_game()
{
  if (text_has_prompt == 1 && action_buffer_pos == 0)
  {
    text_has_prompt = 0;

    action_buffer_ptr[action_buffer_pos++] = 0x53; // S
    action_buffer_ptr[action_buffer_pos++] = 0x61; // a
    action_buffer_ptr[action_buffer_pos++] = 0x76; // v
    action_buffer_ptr[action_buffer_pos++] = 0x65; // e
    action_buffer_ptr[action_buffer_pos++] = 0x23; // #
    action_buffer_ptr[action_buffer_pos++] = 0x0a; // \n
     
    return (uint32_t)1;
  }
  return (uint32_t)0;
}

uint32_t CDECL gms_is_fatal() { return is_fatal; }
char* CDECL gms_get_fatal() { return msg_fatal; }


/* populate functions list and info for the LDG */

PROC LibFunc[] =
{
  {"gms_init", "uint32_t CDECL gms_init(char* name, char* gfxname, char* hntname, char* sndname);\n", gms_init},
  {"gms_rungame", "uint32_t CDECL gms_rungame();\n", gms_rungame},
  {"gms_freemem", "uint32_t CDECL gms_freemem();\n", gms_freemem},

  {"gms_has_text", "uint32_t CDECL gms_has_text();\n", gms_has_text},
  {"gms_has_prompt", "uint32_t CDECL gms_has_prompt();\n", gms_has_prompt},
  {"gms_get_text", "char* CDECL gms_get_text();\n", gms_get_text},
  {"gms_flush_text", "uint32_t CDECL gms_flush_text();\n", gms_flush_text},

  {"gms_is_status_updated", "uint32_t CDECL gms_is_status_updated();\n", gms_is_status_updated},
  {"gms_get_status", "char* CDECL gms_get_status();\n", gms_get_status},
  {"gms_flush_status", "uint32_t CDECL gms_flush_status();\n", gms_flush_status},

  {"gms_seed", "uint32_t CDECL gms_seed(uint32_t seed);\n", gms_seed},
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

  {"gms_set_load_name", "uint32_t CDECL gms_set_load_name(const char* name);\n", gms_set_load_name},
  {"gms_load_game", "uint32_t CDECL gms_load_game();\n", gms_load_game},
  {"gms_set_save_name", "uint32_t CDECL gms_set_save_name(const char* name);\n", gms_set_save_name},
  {"gms_save_game", "uint32_t CDECL gms_save_game();\n", gms_save_game},

  {"gms_is_fatal", "uint32_t CDECL gms_is_fatal();\n", gms_is_fatal},
  {"gms_get_fatal", "char* CDECL gms_get_fatal();\n", gms_get_fatal},
};

LDGLIB LibLdg[] = { { 0x0001, 28, LibFunc, "Magnetic Scrolls Interpreter v2.3 (c) Niclas Karlsson, 1997-2008", 1} };

/*  */

int main(void)
{
  ldg_init(LibLdg);
  return 0;
}
