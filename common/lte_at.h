#ifndef LTE_AT_H
#define LTE_AT_H
/* CCS C. Define LTE_PUTC(c), LTE_AVAILABLE(), LTE_GETC(), LTE_MILLIS(),
 * LTE_RX_FAULT() before including common/lte_at.c once. */
#define LTE_AT_IDLE 0
#define LTE_AT_WAIT 1
#define LTE_AT_OK 2
#define LTE_AT_ERROR 3
#define LTE_AT_TIMEOUT 4
#define LTE_AT_OVERFLOW 5
static unsigned int8 lte_at_result=LTE_AT_IDLE;
static int1 lte_at_link_lost=FALSE;
static char lte_at_line[128], lte_at_expected[48];
static unsigned int8 lte_at_used=0;
static int1 lte_at_ok_seen, lte_at_event_seen, lte_at_prompt;
static unsigned int32 lte_at_started, lte_at_deadline_ms;
#endif
