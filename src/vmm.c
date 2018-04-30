#include <stdbool.h>
#include <stdio.h>

#include "common.h"
#include "conf.h"
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

/* Initialise le fichier de journal */
void vmm_init(FILE * log)
{
	vmm_log = log;
}

static void vmm_log_command(FILE * out, const char *command,
			    unsigned int laddress,
			    unsigned int page, unsigned int frame,
			    unsigned int offset, unsigned int paddress, char c)
{
	if (out)
		fprintf(out, "%s[%c]@%05d: p=%d, o=%d, f=%d pa=%d\n",
			command, c, laddress, page, offset, frame, paddress);
}

/* Translate logical address and give info */
addr get_addr(unsigned int laddress, bool write)
{
	addr data = {
		.page = laddress >> 8,
		.offset = laddress & 0xFF,
	};

	if ((data.frame = tlb_lookup(data.page, write)) < 0) {	// Not in TLB
		if ((data.frame = pt_lookup(data.page)) < 0) {	// Not in PT
			/* Source: Operating System Concepts, page 411 */
			if ((data.frame = pm_find_free()) < 0) {
				data.frame = pm_find_victim();
				int page = pm_get_page(data.frame);
				if (pm_is_dirty(data.frame)) {
					// Page out victim page
					pm_backup_page(data.frame, page);
				}
				// Change to invalid
				pt_unset_entry(page);
			}
			// Page in desired page
			pm_download_page(data.page, data.frame);
			// Reset page table for new page
			pt_set_entry(data.page, data.frame);
			pt_set_readonly(data.page, write);
		}

		tlb_add_entry(data.page, data.frame, pt_readonly_p(data.page));
	}
	pm_update_usage(data.frame);

	data.paddress = (data.frame << 8) + data.offset;

	return data;
}

/* Effectue une lecture à l'adresse logique `laddress` */
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

/* Effectue une écriture à l'adresse logique `laddress` */
void vmm_write(unsigned int laddress, char c)
{
	// Get info
	addr data = get_addr(laddress, true);
	// Write
	pt_set_readonly(data.page, false);	// XXX
	pm_write(data.paddress, c);
	// Log
	vmm_log_command(stdout, "WRITING", laddress,
			data.page, data.frame, data.offset, data.paddress, c);

	write_count++;
}

void vmm_clean(void)
{
	printf("VM reads: %4u\n", read_count);
	printf("VM writes: %4u\n", write_count);
}
