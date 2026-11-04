/* Include once after the board port and configuration. MQTT publish only,
 * client 0, clean session, QoS 1, no TLS. CCS PCH 5.x. */
#include "../common/lte_at.c"
#define A7670_STATE_READY 20
#define A7670_STATE_FAULT 255
static unsigned int8 a7670_state=0, a7670_last_error=0;
static unsigned int32 a7670_retry_at=0, a7670_published=0;
static int1 a7670_waiting=FALSE;
static char a7670_command[256];
static char a7670_payload[240];

static int1 a7670_field_valid(char *s, unsigned int16 maximum, int1 empty)
{
   unsigned int16 n=0;
   if(!empty && !*s) return FALSE;
   while(*s) { if(*s=='"' || *s<' ' || *s==127 || ++n>maximum) return FALSE; ++s; }
   return TRUE;
}
void a7670_init(void)
{
   a7670_state=0; a7670_waiting=FALSE;
   a7670_retry_at=LTE_MILLIS(); lte_at_link_lost=FALSE;
   lte_at_result=LTE_AT_IDLE;
   if(!a7670_field_valid(LTE_APN,63,FALSE) ||
      !a7670_field_valid(LTE_MQTT_HOST,63,FALSE) ||
      !a7670_field_valid(LTE_MQTT_CLIENT_ID,48,FALSE) ||
      !a7670_field_valid(LTE_MQTT_USER,48,TRUE) ||
      !a7670_field_valid(LTE_MQTT_PASSWORD,48,TRUE) ||
      !a7670_field_valid(LTE_MQTT_TOPIC,100,FALSE) || !LTE_MQTT_PORT)
   { a7670_state=A7670_STATE_FAULT; a7670_last_error=10; }
}
int1 a7670_ready(void) { return a7670_state==A7670_STATE_READY && !lte_at_link_lost; }
/* Copies one message; caller retains ownership. FALSE = busy/fault/invalid. */
int1 a7670_publish(char *payload)
{
   if(!a7670_ready() || !*payload || strlen(payload)>=sizeof(a7670_payload)) return FALSE;
   strcpy(a7670_payload,payload); a7670_state=21; return TRUE;
}
static void a7670_retry(void)
{
   a7670_state=0; a7670_waiting=FALSE; lte_at_link_lost=FALSE;
   a7670_retry_at=LTE_MILLIS();
}
void a7670_task(void)
{
   lte_at_poll();
   if(a7670_state==A7670_STATE_FAULT) return;
   if(lte_at_result==LTE_AT_OVERFLOW)
   { a7670_last_error=LTE_AT_OVERFLOW; a7670_state=A7670_STATE_FAULT; return; }
   if(lte_at_link_lost)
   {
      if(a7670_state>=21 && a7670_state<=24)
      { a7670_last_error=11; a7670_state=A7670_STATE_FAULT; }
      else a7670_retry();
      return;
   }
   if(a7670_waiting)
   {
      if(lte_at_result==LTE_AT_WAIT) return;
      a7670_waiting=FALSE;
      if(lte_at_result!=LTE_AT_OK)
      {
         a7670_last_error=lte_at_result;
         /* Cleanup errors mean the resource may already be absent. */
         if(a7670_state>=4 && a7670_state<=6 && lte_at_result==LTE_AT_ERROR) { }
         else if(a7670_state==1 || a7670_state==8 || a7670_state==10 || lte_at_result==LTE_AT_ERROR)
         { a7670_retry(); return; }
         else
         { /* Unknown command/data mode: require physical modem reset. */
            a7670_state=A7670_STATE_FAULT; return;
         }
      }
      ++a7670_state;
      if(a7670_state==15) { a7670_state=A7670_STATE_READY; a7670_last_error=0; }
      if(a7670_state==26) { ++a7670_published; a7670_state=A7670_STATE_READY; }
   }
   if(a7670_state==0)
   {
      if((unsigned int32)(LTE_MILLIS()-a7670_retry_at)<LTE_RETRY_MS) return;
      a7670_state=1;
   }
   switch(a7670_state)
   {
      case 1: lte_at_command("AT","",3000); break;
      case 2: lte_at_command("ATE0","",3000); break;
      case 3: lte_at_command("AT+CMEE=2","",3000); break;
      case 4: lte_at_command("AT+CMQTTDISC=0,60","+CMQTTDISC: 0,0",65000); break;
      case 5: lte_at_command("AT+CMQTTREL=0","",3000); break;
      case 6: lte_at_command("AT+CMQTTSTOP","+CMQTTSTOP: 0",65000); break;
      case 7: lte_at_command("AT+CFUN=1","",15000); break;
      case 8: lte_at_command("AT+CPIN?","+CPIN: READY",3000); break;
      case 9: lte_at_command("AT+CEREG=0","",3000); break;
      case 10: lte_at_command("AT+CEREG?","+CEREG: 0,1",3000); break;
      case 11:
         sprintf(a7670_command,"AT+CGDCONT=1,\"IP\",\"%s\"",LTE_APN);
         lte_at_command(a7670_command,"",3000); break;
      case 12: lte_at_command("AT+CMQTTSTART","+CMQTTSTART: 0",120000); break;
      case 13:
         sprintf(a7670_command,"AT+CMQTTACCQ=0,\"%s\",0",LTE_MQTT_CLIENT_ID);
         lte_at_command(a7670_command,"",3000); break;
      case 14:
         if(strlen(LTE_MQTT_USER) && strlen(LTE_MQTT_PASSWORD))
            sprintf(a7670_command,"AT+CMQTTCONNECT=0,\"tcp://%s:%LU\",60,1,\"%s\",\"%s\"",
               LTE_MQTT_HOST,(unsigned int32)LTE_MQTT_PORT,LTE_MQTT_USER,LTE_MQTT_PASSWORD);
         else if(strlen(LTE_MQTT_USER))
            sprintf(a7670_command,"AT+CMQTTCONNECT=0,\"tcp://%s:%LU\",60,1,\"%s\"",
               LTE_MQTT_HOST,(unsigned int32)LTE_MQTT_PORT,LTE_MQTT_USER);
         else
            sprintf(a7670_command,"AT+CMQTTCONNECT=0,\"tcp://%s:%LU\",60,1",
               LTE_MQTT_HOST,(unsigned int32)LTE_MQTT_PORT);
         lte_at_command(a7670_command,"+CMQTTCONNECT: 0,0",120000); break;
      case 21:
         sprintf(a7670_command,"AT+CMQTTTOPIC=0,%LU",(unsigned int32)strlen(LTE_MQTT_TOPIC));
         lte_at_begin("",10000,TRUE); lte_at_send(a7670_command); LTE_PUTC('\r'); break;
      case 22:
         lte_at_begin("",10000,FALSE); lte_at_send(LTE_MQTT_TOPIC); break;
      case 23:
         sprintf(a7670_command,"AT+CMQTTPAYLOAD=0,%LU",(unsigned int32)strlen(a7670_payload));
         lte_at_begin("",10000,TRUE); lte_at_send(a7670_command); LTE_PUTC('\r'); break;
      case 24:
         lte_at_begin("",10000,FALSE); lte_at_send(a7670_payload); break;
      case 25: lte_at_command("AT+CMQTTPUB=0,1,60,0,0","+CMQTTPUB: 0,0",65000); break;
      default: return;
   }
   a7670_waiting=TRUE;
}
