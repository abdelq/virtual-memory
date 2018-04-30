#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "conf.h"
#include "common.h"
#include "vmm.h"
#include "tlb.h"
#include "pt.h"
#include "pm.h"

static unsigned int read_count = 0;
static unsigned int write_count = 0;
static FILE *vmm_log;

typedef struct {
	unsigned int page;
	unsigned int offset;
	unsigned int frame;
	unsigned int paddress;
} addr;

void vmm_init(FILE * log)
{
	// Initialise le fichier de journal.
	vmm_log = log;
}

// NE PAS MODIFIER CETTE FONCTION
static void vmm_log_command(FILE * out, const char *command,
			    unsigned int laddress,
			    unsigned int page, unsigned int frame,
			    unsigned int offset, unsigned int paddress, char c)
{
	if (out)
		fprintf(out, "%s[%c]@%05d: p=%d, o=%d, f=%d pa=%d\n",
			command, c, laddress, page, offset, frame, paddress);
}

// Translate logical address and give info
addr get_addr(unsigned int laddress, bool write)
{
	addr data = {
		.page = laddress >> 8,
		.offset = laddress & 0xFF,
		.frame = tlb_lookup(data.page, write)
	};

	if (data.frame < 0) {	// Not in TLB
		if ((data.frame = pt_lookup(data.page)) < 0) {	// Not in PT
			// FIXME Choose a frame deterministically
			srand(time(NULL));
			data.frame = rand() % NUM_FRAMES;	// XXX

			pm_download_page(data.page, data.frame);
			pt_set_entry(data.page, data.frame);
		}

		tlb_add_entry(data.page, data.frame, pt_readonly_p(data.page));	// XXX
	}

	data.paddress = (data.frame << 8) + data.offset;	// XXX
	//data.paddress = data.frame * PAGE_FRAME_SIZE + data.offset;

	return data;
}

/* Effectue une lecture à l'adresse logique `laddress`.  */
char vmm_read(unsigned int laddress)
{
	// Get info
	addr data = get_addr(laddress, false);
	// Read
	char c = pm_read(data.paddress);
	// Log
	vmm_log_command(stdout, "READING", laddress,
			data.page, data.frame, data.offset, data.paddress, c);

	read_count++;
	return c;
}

/* Effectue une écriture à l'adresse logique `laddress`.  */
void vmm_write(unsigned int laddress, char c)
{
	// Get info
	addr data = get_addr(laddress, true);
	// Write
	pm_write(data.paddress, c);
	//Log
	vmm_log_command(stdout, "WRITING", laddress,
			data.page, data.frame, data.offset, data.paddress, c);

	write_count++;
}

// NE PAS MODIFIER CETTE FONCTION
void vmm_clean(void)
{
	fprintf(stdout, "VM reads : %4u\n", read_count);
	fprintf(stdout, "VM writes: %4u\n", write_count);
}
