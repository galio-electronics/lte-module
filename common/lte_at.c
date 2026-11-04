#include "lte_at.h"
#include <string.h>
#ifndef LTE_AT_LINE_HOOK
#define LTE_AT_LINE_HOOK(line)
#endif
static void lte_at_send(char *text)
{
   while(*text) LTE_PUTC(*text++);
}
static void lte_at_begin(char *expected, unsigned int32 timeout, int1 prompt)
{
   strcpy(lte_at_expected,expected);
   lte_at_used=0;
   lte_at_ok_seen=FALSE;
   lte_at_event_seen=(*expected==0);
   lte_at_prompt=prompt;
   lte_at_started=LTE_MILLIS();
   lte_at_deadline_ms=timeout;
   lte_at_result=LTE_AT_WAIT;
}
static void lte_at_command(char *command, char *expected, unsigned int32 timeout)
{
   lte_at_begin(expected,timeout,FALSE);
   lte_at_send(command);
   LTE_PUTC('\r');
}
static void lte_at_poll(void)
{
   char c;
   unsigned int16 budget=256;
   if(LTE_RX_FAULT()) { lte_at_result=LTE_AT_OVERFLOW; return; }
   while(LTE_AVAILABLE() && budget--)
   {
      c=LTE_GETC();
      if(c=='>' && lte_at_prompt && lte_at_result==LTE_AT_WAIT)
      { lte_at_result=LTE_AT_OK; lte_at_used=0; continue; }
      if(c=='\r') continue;
      if(c=='\n')
      {
         lte_at_line[lte_at_used]=0;
         LTE_AT_LINE_HOOK(lte_at_line);
         if(strstr(lte_at_line,"+CMQTTCONNLOST:") ||
            strstr(lte_at_line,"+CIPEVENT: NETWORK CLOSED") ||
            !strcmp(lte_at_line,"RDY")) lte_at_link_lost=TRUE;
         if(lte_at_result==LTE_AT_WAIT)
         {
            if(!strcmp(lte_at_line,"ERROR") || !strncmp(lte_at_line,"+CME ERROR:",11))
               lte_at_result=LTE_AT_ERROR;
            if(!strcmp(lte_at_line,"OK")) lte_at_ok_seen=TRUE;
            if(lte_at_expected[0] && !strcmp(lte_at_line,lte_at_expected)) lte_at_event_seen=TRUE;
            /* A completed MQTT operation with a nonzero result is a failure,
             * even when its immediate command response was OK. */
            if(!strncmp(lte_at_expected,"+CMQTT",6) &&
               ((!strncmp(lte_at_expected,"+CMQTTSTART:",12) && !strncmp(lte_at_line,"+CMQTTSTART:",12)) ||
                (!strncmp(lte_at_expected,"+CMQTTSTOP:",11) && !strncmp(lte_at_line,"+CMQTTSTOP:",11)) ||
                (!strncmp(lte_at_expected,"+CMQTTDISC:",11) && !strncmp(lte_at_line,"+CMQTTDISC:",11)) ||
                (!strncmp(lte_at_expected,"+CMQTTCONNECT:",14) && !strncmp(lte_at_line,"+CMQTTCONNECT:",14)) ||
                (!strncmp(lte_at_expected,"+CMQTTPUB:",10) && !strncmp(lte_at_line,"+CMQTTPUB:",10))) &&
               strcmp(lte_at_line,lte_at_expected)) lte_at_result=LTE_AT_ERROR;
            /* Registration accepts home and roaming; query uses n=0. */
            if(!strcmp(lte_at_expected,"+CEREG: 0,1") &&
               !strcmp(lte_at_line,"+CEREG: 0,5")) lte_at_event_seen=TRUE;
            if(lte_at_result==LTE_AT_WAIT && !lte_at_prompt && lte_at_ok_seen && lte_at_event_seen)
               lte_at_result=LTE_AT_OK;
         }
         lte_at_used=0;
      }
      else if(lte_at_used<sizeof(lte_at_line)-1) lte_at_line[lte_at_used++]=c;
      else { lte_at_result=LTE_AT_OVERFLOW; lte_at_used=0; return; }
   }
   if(lte_at_result==LTE_AT_WAIT &&
      (unsigned int32)(LTE_MILLIS()-lte_at_started)>=lte_at_deadline_ms)
      lte_at_result=LTE_AT_TIMEOUT;
}
