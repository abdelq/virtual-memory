#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "conf.h"
#include "common.h"
#include "vmm.h"
#include "tlb.h"
#include "pt.h"
#include "pm.h"

static unsigned int read_count = 0;
static unsigned int write_count = 0;
static FILE* vmm_log;

// Page stuff
#define PAGE_MASK 255
// Offset stuff
#define OFFSET_BITS 8
#define OFFSET_MASK 255

typedef struct {
  unsigned int page;
  unsigned int offset;
  unsigned int frame;
  unsigned int paddress;
} address_data;

void vmm_init (FILE *log)
{
  // Initialise le fichier de journal.
  vmm_log = log;
}


// NE PAS MODIFIER CETTE FONCTION
static void vmm_log_command (FILE *out, const char *command,
    unsigned int laddress, /* Logical address. */
    unsigned int page,
    unsigned int frame,
    unsigned int offset,
    unsigned int paddress, /* Physical address.  */
    char c) /* Caractère lu ou écrit.  */
{
  if (out)
    fprintf (out, "%s[%c]@%05d: p=%d, o=%d, f=%d pa=%d\n", command, c, laddress,
        page, offset, frame, paddress);
}

unsigned int handle_page_fault(unsigned int page){
  //TODO
  return -1;
}

// Translate logical address and give info
address_data get_address_data(unsigned int laddress,bool write){
  address_data data;
  data.page = laddress >> OFFSET_BITS & PAGE_MASK;
  data.offset = laddress & OFFSET_MASK;
  // Check in TLB
  int frame = tlb_lookup(data.page,write);
  if(frame >= 0) data.frame = frame;
  // Not in TLB
  else {
    // Check in Page Table
    frame = pt_lookup(data.page);
    if(frame >= 0) data.frame = frame;
    // Page Fault
    else{
      // Error handling page fault would be
      // handled in handle_page_fault already
      data.frame = handle_page_fault(data.page);
    }
  }
  data.paddress = data.frame*PAGE_FRAME_SIZE + data.offset;
  return data;
}

/* Effectue une lecture à l'adresse logique `laddress`.  */
char vmm_read (unsigned int laddress)
{
  char c;
  read_count++;
  // Get info
  address_data data = get_address_data(laddress, false);
  // Read
  c = pm_read(data.paddress);
  // Log
  vmm_log_command (stdout, "READING", laddress,
      data.page,
      data.frame,
      data.offset,
      data.paddress,
      c);
  return c;
}

/* Effectue une écriture à l'adresse logique `laddress`.  */
void vmm_write (unsigned int laddress, char c)
{
  write_count++;
  // Get info
  address_data data = get_address_data(laddress, true);
  // Write
  pm_write(data.paddress,c);
  //Log
  vmm_log_command (stdout, "WRITING", laddress,
      data.page,
      data.frame,
      data.offset,
      data.paddress,
      c);
}


// NE PAS MODIFIER CETTE FONCTION
void vmm_clean (void)
{
  fprintf (stdout, "VM reads : %4u\n", read_count);
  fprintf (stdout, "VM writes: %4u\n", write_count);
}
