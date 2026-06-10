/** A reader-writer test program.

A writer sends four values to the reader.
The reader collects the values and sums them up.

Also note the use of the `volatile` keyword.
*/

#include <math.h>
#include <cortos.h>
#include <ajit_shim_itoa.h>

#define TOTAL_MESSAGES 4

struct CortosQueueHeader * volatile hdr;

void main() {} // important, but kept empty.

void cortos_entry_func_001() {
  uint32_t sentCount, i, totalSent = 0;
  uint32_t msgs[TOTAL_MESSAGES];

  CORTOS_TRACE("Hello from Sender!");
  for (i = 0; i < TOTAL_MESSAGES; ++i) {
    msgs[i] = i;
  }

  hdr = cortos_reserveQueue(
      sizeof(uint32_t) /*single msg size in bytes*/,
      2 /*length i.e. max messages in the queue*/,
      1 /*1 means non-cacheable*/);
  while (totalSent < TOTAL_MESSAGES) {
    sentCount = cortos_writeMessages(hdr,
      (uint8_t*)(msgs+totalSent), (TOTAL_MESSAGES-totalSent));
    {
      char remaining_buf[11];
      char sent_buf[11];
      ajit_shim_u32_to_dec(remaining_buf, TOTAL_MESSAGES - totalSent);
      ajit_shim_u32_to_dec(sent_buf, sentCount);
      CORTOS_DEBUG("Sending %s messages: sent %s.", remaining_buf, sent_buf);
    }
    totalSent += sentCount;
  }

  cortos_exit(totalSent);
}

void cortos_entry_func_010() {
  /* do something */
  return;
}

void cortos_entry_func_101() {
  uint32_t sum = 0;
  uint32_t i = 0, count;
  uint32_t msgs[TOTAL_MESSAGES];

  CORTOS_TRACE("Hello from Receiver!");

  while(hdr == 0);

  CORTOS_TRACE("Hello from Receiver: hdr is set.");
  while(i < TOTAL_MESSAGES) {
    count = cortos_readMessages(hdr, (uint8_t*)(msgs+i), TOTAL_MESSAGES-i);
    i += count;
    if (count) {
      char count_buf[11];
      char remain_buf[11];
      ajit_shim_u32_to_dec(count_buf, count);
      ajit_shim_u32_to_dec(remain_buf, TOTAL_MESSAGES - i);
      CORTOS_DEBUG("Received %s messages. Need %s more.", count_buf, remain_buf);
    }
  }

  for (i = 0; i < TOTAL_MESSAGES; ++i) {
    sum += msgs[i];
  }

  cortos_exit(sum);
}

void cortos_entry_func_110() {
  /* do something */
  return;
}
