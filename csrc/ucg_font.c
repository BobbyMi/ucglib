/*

  ucg_font.c
  
  UCG Font High Level Interface; mostly taken over from U8glib

  Universal uC Color Graphics Library
  
  Copyright (c) 2013, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list 
    of conditions and the following disclaimer.
    
  * Redistributions in binary form must reproduce the above copyright notice, this 
    list of conditions and the following disclaimer in the documentation and/or other 
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
  
*/

#include "ucg.h"

/* font api */

/* pointer to the start adress of the glyph, points to progmem area */
typedef void * ucg_glyph_t;

/* size of the font data structure, there is no struct or class... */
#define UCG_FONT_DATA_STRUCT_SIZE 17

/*
  ... instead the fields of the font data structure are accessed directly by offset 
  font information 
  offset
  0             font format
  1             FONTBOUNDINGBOX width           unsigned
  2             FONTBOUNDINGBOX height          unsigned
  3             FONTBOUNDINGBOX x-offset         signed
  4             FONTBOUNDINGBOX y-offset        signed
  5             capital A height                                unsigned
  6             start 'A'
  8             start 'a'
  10            encoding start
  11            encoding end
  12            descent 'g'                     negative: below baseline
  13            font max ascent
  14            font min decent             negative: below baseline 
  15            font xascent
  16            font xdecent             negative: below baseline 
  
*/

/* use case: What is the width and the height of the minimal box into which string s fints? */
void ucg_font_GetStrSize(const void *font, const char *s, ucg_int_t *width, ucg_int_t *height);
void ucg_font_GetStrSizeP(const void *font, const char *s, ucg_int_t *width, ucg_int_t *height);

/* use case: lower left edge of a minimal box is known, what is the correct x, y position for the string draw procedure */
void ucg_font_AdjustXYToDraw(const void *font, const char *s, ucg_int_t *x, ucg_int_t *y);
void ucg_font_AdjustXYToDrawP(const void *font, const char *s, ucg_int_t *x, ucg_int_t *y);

/* use case: Baseline origin known, return minimal box */
void ucg_font_GetStrMinBox(ucg_t *ucg, const void *font, const char *s, ucg_int_t *x, ucg_int_t *y, ucg_int_t *width, ucg_int_t *height);

/* procedures */

/*========================================================================*/
/* low level byte and word access */

/* removed NOINLINE, because it leads to smaller code, might also be faster */
//static uint8_t ucg_font_get_byte(const ucg_fntpgm_uint8_t *font, uint8_t offset) UCG_NOINLINE;
static uint8_t ucg_font_get_byte(const ucg_fntpgm_uint8_t *font, uint8_t offset)
{
  font += offset;
  return ucg_pgm_read( (ucg_pgm_uint8_t *)font );  
}

static uint16_t ucg_font_get_word(const ucg_fntpgm_uint8_t *font, uint8_t offset) UCG_NOINLINE; 
static uint16_t ucg_font_get_word(const ucg_fntpgm_uint8_t *font, uint8_t offset)
{
    uint16_t pos;
    font += offset;
    pos = ucg_pgm_read( (ucg_pgm_uint8_t *)font );
    font++;
    pos <<= 8;
    pos += ucg_pgm_read( (ucg_pgm_uint8_t *)font);
    return pos;
}

/*========================================================================*/
/* direct access on the font */

static uint8_t ucg_font_GetFormat(const ucg_fntpgm_uint8_t *font) UCG_NOINLINE;
static uint8_t ucg_font_GetFormat(const ucg_fntpgm_uint8_t *font)
{
  return ucg_font_get_byte(font, 0);
}

static uint8_t ucg_font_GetFontGlyphStructureSize(const ucg_fntpgm_uint8_t *font) UCG_NOINLINE;
static uint8_t ucg_font_GetFontGlyphStructureSize(const ucg_fntpgm_uint8_t *font)
{
  switch(ucg_font_GetFormat(font))
  {
    case 0: return 6;
    case 1: return 3;
    case 2: return 6;
  }
  return 3;
}

static uint8_t ucg_font_GetBBXWidth(const void *font)
{
  return ucg_font_get_byte(font, 1);
}

static uint8_t ucg_font_GetBBXHeight(const void *font)
{
  return ucg_font_get_byte(font, 2);
}

static int8_t ucg_font_GetBBXOffX(const void *font)
{
  return ucg_font_get_byte(font, 3);
}

static int8_t ucg_font_GetBBXOffY(const void *font)
{
  return ucg_font_get_byte(font, 4);
}

uint8_t ucg_font_GetCapitalAHeight(const void *font)
{
  return ucg_font_get_byte(font, 5);
}

uint16_t ucg_font_GetEncoding65Pos(const void *font) UCG_NOINLINE;
uint16_t ucg_font_GetEncoding65Pos(const void *font)
{
    return ucg_font_get_word(font, 6);
}

uint16_t ucg_font_GetEncoding97Pos(const void *font) UCG_NOINLINE;
uint16_t ucg_font_GetEncoding97Pos(const void *font)
{
    return ucg_font_get_word(font, 8);
}

uint8_t ucg_font_GetFontStartEncoding(const void *font)
{
  return ucg_font_get_byte(font, 10);
}

uint8_t ucg_font_GetFontEndEncoding(const void *font)
{
  return ucg_font_get_byte(font, 11);
}

int8_t ucg_font_GetLowerGDescent(const void *font)
{
  return ucg_font_get_byte(font, 12);
}

int8_t ucg_font_GetFontAscent(const void *font)
{
  return ucg_font_get_byte(font, 13);
}

int8_t ucg_font_GetFontDescent(const void *font)
{
  return ucg_font_get_byte(font, 14);
}

int8_t ucg_font_GetFontXAscent(const void *font)
{
  return ucg_font_get_byte(font, 15);
}

int8_t ucg_font_GetFontXDescent(const void *font)
{
  return ucg_font_get_byte(font, 16);
}


/* return the data start for a font and the glyph pointer */
static uint8_t *ucg_font_GetGlyphDataStart(const void *font, ucg_glyph_t g)
{
  return ((ucg_fntpgm_uint8_t *)g) + ucg_font_GetFontGlyphStructureSize(font);
}

/* calculate the overall length of the font, only used to create the picture for the google wiki */
size_t ucg_font_GetSize(const void *font)
{
  uint8_t *p = (uint8_t *)(font);
  uint8_t font_format = ucg_font_GetFormat(font);
  uint8_t data_structure_size = ucg_font_GetFontGlyphStructureSize(font);
  uint8_t start, end;
  uint8_t i;
  uint8_t mask = 255;
  
  start = ucg_font_GetFontStartEncoding(font);
  end = ucg_font_GetFontEndEncoding(font);

  if ( font_format == 1 )
    mask = 15;

  p += UCG_FONT_DATA_STRUCT_SIZE;       /* skip font general information */  

  i = start;  
  for(;;)
  {
    if ( ucg_pgm_read((ucg_pgm_uint8_t *)(p)) == 255 )
    {
      p += 1;
    }
    else
    {
      p += ucg_pgm_read( ((ucg_pgm_uint8_t *)(p)) + 2 ) & mask;
      p += data_structure_size;
    }
    if ( i == end )
      break;
    i++;
  }
    
  return p - (uint8_t *)font;
}

/*========================================================================*/
/* u8g interface, font access */

uint8_t ucg_GetFontBBXWidth(ucg_t *ucg)
{
  return ucg_font_GetBBXWidth(ucg->font);
}

uint8_t ucg_GetFontBBXHeight(ucg_t *ucg)
{
  return ucg_font_GetBBXHeight(ucg->font);
}

int8_t u8g_GetFontBBXOffX(ucg_t *ucg) UCG_NOINLINE;
int8_t u8g_GetFontBBXOffX(ucg_t *ucg)
{
  return ucg_font_GetBBXOffX(ucg->font);
}

int8_t ucg_GetFontBBXOffY(ucg_t *ucg) UCG_NOINLINE;
int8_t ucg_GetFontBBXOffY(ucg_t *ucg)
{
  return ucg_font_GetBBXOffY(ucg->font);
}

uint8_t ucg_GetFontCapitalAHeight(ucg_t *ucg) UCG_NOINLINE; 
uint8_t ucg_GetFontCapitalAHeight(ucg_t *ucg)
{
  return ucg_font_GetCapitalAHeight(ucg->font);
}

/*========================================================================*/
/* glyph handling */

static void ucg_CopyGlyphDataToCache(ucg_t *ucg, ucg_glyph_t g)
{
  uint8_t tmp;
  switch( ucg_font_GetFormat(ucg->font) )
  {
    case 0:
    case 2:
  /*
    format 0
    glyph information 
    offset
    0             BBX width                                       unsigned
    1             BBX height                                      unsigned
    2             data size                                          unsigned    (BBX width + 7)/8 * BBX height
    3             DWIDTH                                          signed
    4             BBX xoffset                                    signed
    5             BBX yoffset                                    signed
  byte 0 == 255 indicates empty glyph
  */
      ucg->glyph_width =  ucg_pgm_read( ((ucg_pgm_uint8_t *)g) + 0 );
      ucg->glyph_height =  ucg_pgm_read( ((ucg_pgm_uint8_t *)g) + 1 );
      ucg->glyph_dx =  ucg_pgm_read( ((ucg_pgm_uint8_t *)g) + 3 );
      ucg->glyph_x =  ucg_pgm_read( ((ucg_pgm_uint8_t *)g) + 4 );
      ucg->glyph_y =  ucg_pgm_read( ((ucg_pgm_uint8_t *)g) + 5 );
      break;
    case 1:
    default:
      /*
format 1
  0             BBX xoffset                                    signed   --> upper 4 Bit
  0             BBX yoffset                                    signed --> lower 4 Bit
  1             BBX width                                       unsigned --> upper 4 Bit
  1             BBX height                                      unsigned --> lower 4 Bit
  2             data size                                           unsigned -(BBX width + 7)/8 * BBX height  --> lower 4 Bit
  2             DWIDTH                                          signed --> upper  4 Bit
  byte 0 == 255 indicates empty glyph
      */
    
      tmp = ucg_pgm_read( ((ucg_pgm_uint8_t *)g) + 0 );
      ucg->glyph_y =  tmp & 15;
      ucg->glyph_y-=2;
      tmp >>= 4;
      ucg->glyph_x =  tmp;
    
      tmp = ucg_pgm_read( ((ucg_pgm_uint8_t *)g) + 1 );
      ucg->glyph_height =  tmp & 15;
      tmp >>= 4;
      ucg->glyph_width =  tmp;
      
      tmp = ucg_pgm_read( ((ucg_pgm_uint8_t *)g) + 2 );
      tmp >>= 4;
      ucg->glyph_dx = tmp;
    
      
      break;
  }
}

//void ucg_FillEmptyGlyphCache(u8g_t *ucg) UCG_NOINLINE;
static void ucg_FillEmptyGlyphCache(ucg_t *ucg)
{
  ucg->glyph_dx = 0;
  ucg->glyph_width = 0;
  ucg->glyph_height = 0;
  ucg->glyph_x = 0;
  ucg->glyph_y = 0;
}

/*
  Find (with some speed optimization) and return a pointer to the glyph data structure
  Also uncompress (format 1) and copy the content of the data structure to the u8g structure
*/
ucg_glyph_t ucg_GetGlyph(ucg_t *ucg, uint8_t requested_encoding)
{
  uint8_t *p = (uint8_t *)(ucg->font);
  uint8_t font_format = ucg_font_GetFormat(ucg->font);
  uint8_t data_structure_size = ucg_font_GetFontGlyphStructureSize(ucg->font);
  uint8_t start, end;
  uint16_t pos;
  uint8_t i;
  uint8_t mask = 255;

  if ( font_format == 1 )
    mask = 15;
  
  start = ucg_font_GetFontStartEncoding(ucg->font);
  end = ucg_font_GetFontEndEncoding(ucg->font);

  pos = ucg_font_GetEncoding97Pos(ucg->font);
  if ( requested_encoding >= 97 && pos > 0 )
  {
    p+= pos;
    start = 97;
  }
  else 
  {
    pos = ucg_font_GetEncoding65Pos(ucg->font);
    if ( requested_encoding >= 65 && pos > 0 )
    {
      p+= pos;
      start = 65;
    }
    else
      p += UCG_FONT_DATA_STRUCT_SIZE;       /* skip font general information */  
  }
  
  if ( requested_encoding > end )
  {
    ucg_FillEmptyGlyphCache(ucg);
    return NULL;                      /* not found */
  }
  
  i = start;
  if ( i <= end )
  {
    for(;;)
    {
      if ( ucg_pgm_read((ucg_pgm_uint8_t *)(p)) == 255 )
      {
        p += 1;
      }
      else
      {
        if ( i == requested_encoding )
        {
          ucg_CopyGlyphDataToCache(ucg, p);
          return p;
        }
        p += ucg_pgm_read( ((ucg_pgm_uint8_t *)(p)) + 2 ) & mask;
        p += data_structure_size;
      }
      if ( i == end )
        break;
      i++;
    }
  }
  
  ucg_FillEmptyGlyphCache(ucg);
    
  return NULL;
}

uint8_t ucg_IsGlyph(ucg_t *ucg, uint8_t requested_encoding)
{
  if ( ucg_GetGlyph(ucg, requested_encoding) != NULL )
    return 1;
  return 0;
}

int8_t ucg_GetGlyphDeltaX(ucg_t *ucg, uint8_t requested_encoding)
{
  if ( ucg_GetGlyph(ucg, requested_encoding) == NULL )
    return 0;  /* should never happen, so return something */
  return ucg->glyph_dx;
}

ucg_int_t ucg_draw_glyph(ucg_t *ucg, ucg_int_t x, ucg_int_t y, uint8_t dir, uint8_t encoding)
{
  const ucg_pgm_uint8_t *data;
  uint8_t j;
  ucg_int_t dx, dy;
  uint8_t bytes_per_line;

  {
    ucg_glyph_t g = ucg_GetGlyph(ucg, encoding);
    if ( g == NULL  )
      return 0;
    data = ucg_font_GetGlyphDataStart(ucg->font, g);
  }
  
  bytes_per_line = ucg->glyph_width;
  bytes_per_line += 7;
  bytes_per_line /= 8;
    
  dx = 0;
  dy = 0;
  switch(dir)
  {
    case 0:
      x += ucg->glyph_x;
      y -= ucg->glyph_y;
      y -= ucg->glyph_height;
      dy = 1;
      break;
    case 1:
      x += ucg->glyph_y;
      y += ucg->glyph_x;
      x += ucg->glyph_height;
      dx = -1;
      break;
    case 2:
      x -= ucg->glyph_x;
      y += ucg->glyph_y;
      y += ucg->glyph_height;
      dy = -1;
      break;
    case 3:
      x -= ucg->glyph_y;
      y -= ucg->glyph_x;
      x -= ucg->glyph_height;
      dx = 1;
      break;
  }

  for( j = 0; j < ucg->glyph_height; j++ )
  {
    ucg_DrawBitmapLine(ucg, x, y, dir, ucg->glyph_width, data);
    data += bytes_per_line;
    y+=dy;
    x+=dx;
  }
  
  return ucg->glyph_dx;
}

ucg_int_t ucg_DrawGlyph(ucg_t *ucg, ucg_int_t x, ucg_int_t y, uint8_t dir, uint8_t encoding)
{
  //y += ucg->font_calc_vref(u8g);
  return ucg_draw_glyph(ucg, x, y, dir, encoding);
}

/*===============================================*/

/* set ascent/descent for reference point calculation */

void ucg_UpdateRefHeight(ucg_t *ucg)
{
  if ( ucg->font == NULL )
    return;
  if ( ucg->font_height_mode == UCG_FONT_HEIGHT_MODE_TEXT )
  {
    ucg->font_ref_ascent = ucg_font_GetCapitalAHeight(ucg->font);
    ucg->font_ref_descent = ucg_font_GetLowerGDescent(ucg->font);
  }
  else if ( ucg->font_height_mode == UCG_FONT_HEIGHT_MODE_XTEXT )
  {
    ucg->font_ref_ascent = ucg_font_GetFontXAscent(ucg->font);
    ucg->font_ref_descent = ucg_font_GetFontXDescent(ucg->font);
  }
  else
  {
    ucg->font_ref_ascent = ucg_font_GetFontAscent(ucg->font);
    ucg->font_ref_descent = ucg_font_GetFontDescent(ucg->font);
  }  
}

void ucg_SetFontRefHeightText(ucg_t *ucg)
{
  ucg->font_height_mode = UCG_FONT_HEIGHT_MODE_TEXT;
  ucg_UpdateRefHeight(ucg);
}

void ucg_SetFontRefHeightExtendedText(ucg_t *ucg)
{
  ucg->font_height_mode = UCG_FONT_HEIGHT_MODE_XTEXT;
  ucg_UpdateRefHeight(ucg);
}

void ucg_SetFontRefHeightAll(ucg_t *ucg)
{
  ucg->font_height_mode = UCG_FONT_HEIGHT_MODE_ALL;
  ucg_UpdateRefHeight(ucg);
}

/*===============================================*/
/* callback procedures to correct the y position */

ucg_int_t ucg_font_calc_vref_font(ucg_t *ucg)
{
  return 0;
}

void ucg_SetFontPosBaseline(ucg_t *ucg)
{
  ucg->font_calc_vref = ucg_font_calc_vref_font;
}


ucg_int_t ucg_font_calc_vref_bottom(ucg_t *ucg)
{
  return (ucg_int_t)(ucg->font_ref_descent);
}

void ucg_SetFontPosBottom(ucg_t *ucg)
{
  ucg->font_calc_vref = ucg_font_calc_vref_bottom;
}

ucg_int_t ucg_font_calc_vref_top(ucg_t *ucg)
{
  ucg_int_t tmp;
  /* reference pos is one pixel above the upper edge of the reference glyph */
  tmp = (ucg_int_t)(ucg->font_ref_ascent);
  tmp++;
  return tmp;
}

void ucg_SetFontPosTop(ucg_t *ucg)
{
  ucg->font_calc_vref = ucg_font_calc_vref_top;
}

ucg_int_t ucg_font_calc_vref_center(ucg_t *ucg)
{
  int8_t tmp;
  tmp = ucg->font_ref_ascent;
  tmp -= ucg->font_ref_descent;
  tmp /= 2;
  tmp += ucg->font_ref_descent;  
  return tmp;
}

void ucg_SetFontPosCenter(ucg_t *ucg)
{
  ucg->font_calc_vref = ucg_font_calc_vref_center;
}

/*===============================================*/

void ucg_SetFont(ucg_t *ucg, const ucg_fntpgm_uint8_t  *font)
{
  if ( ucg->font != font )
  {
    ucg->font = font;
    ucg_UpdateRefHeight(ucg);
    //ucg_SetFontPosBaseline(ucg);
  }
}