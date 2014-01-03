/*

  ucg_dev_ic_ssd1351.c
  
  Specific code for the SSD1351 controller (OLED displays)

  Universal uC Color Graphics Library
  
  Copyright (c) 2014, olikraus@gmail.com
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

const uint8_t ucg_ssd1351_set_pos_seq[] = 
{
  UCG_C10(0x015),	UCG_VAR0(0,0x0ff, 0),		/* set x position */
  UCG_C10(0x075),	UCG_VAR1(0,0x0ff, 0),		/* set y position */
  UCG_C10(0x05c),							/* write to RAM */
  UCG_DATA(),								/* change to data mode */
  UCG_END()
};


ucg_int_t u8g_dev_ic_ssd1351(ucg_t *ucg, ucg_int_t msg, void *data)
{
  switch(msg)
  {
    case UCG_MSG_DEV_POWER_UP:
      /* setup com interface and provide information on the clock speed */
      /* of the serial and parallel interface. Values are nanoseconds. */
      return ucg_com_PowerUp(ucg, 50, 300);
    case UCG_MSG_DEV_POWER_DOWN:
      /* not yet implemented */
      return 1;
    case UCG_MSG_GET_DIMENSION:
      ((ucg_wh_t *)data)->w = 128;
      ((ucg_wh_t *)data)->h = 128;
      return 1;
    case UCG_MSG_DRAW_PIXEL:
      if ( ucg_clip_is_pixel_visible(ucg) !=0 )
      {
	ucg->com_var[0] = ucg->arg.pixel.pos.x;
	ucg->com_var[1] = ucg->arg.pixel.pos.y;
	ucg_com_SendCmdSeq(ucg, ucg_ssd1351_set_pos_seq);	
	ucg_com_SendRepeat3Bytes(ucg, 1, ucg->arg.pixel.rgb.color);
      }
      return 1;
    case UCG_MSG_DRAW_L90FX:
      ucg_handle_l90fx(ucg, u8g_dev_ic_ssd1351);
      return 1;
    case UCG_MSG_DRAW_L90TC:
      ucg_handle_l90tc(ucg, u8g_dev_ic_ssd1351);
      return 1;
    case UCG_MSG_DRAW_L90SE:
      ucg_handle_l90se(ucg, u8g_dev_ic_ssd1351);
      return 1;
  }
  return ucg_dev_default_cb(ucg, msg, data);  
}

