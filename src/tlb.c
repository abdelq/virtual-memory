#include <stdbool.h>
#include <stdio.h>

#include "conf.h"
#include "tlb.h"

struct tlb_entry {
	unsigned int page_number;
	int frame_number;	// Invalide si négatif
	bool readonly:1;
	unsigned int uses;
};

static FILE *tlb_log = NULL;
static struct tlb_entry tlb_entries[TLB_NUM_ENTRIES];

static unsigned int tlb_hit_count = 0;
static unsigned int tlb_miss_count = 0;
static unsigned int tlb_mod_count = 0;

/* Initialise le TLB, et indique où envoyer le log des accès */
void tlb_init(FILE * log)
{
	for (int i = 0; i < TLB_NUM_ENTRIES; i++)
		tlb_entries[i].frame_number = -1;
	tlb_log = log;
}

/* Recherche dans le TLB et renvoie le `frame_number` si trouvé,
 * ou un nombre négatif sinon */
static int tlb__lookup(unsigned int page_number, bool write)
{
	for (int i = 0; i < TLB_NUM_ENTRIES; i++) {
		if (tlb_entries[i].page_number == page_number) {
			tlb_entries[i].uses++;
			return tlb_entries[i].frame_number;	// If invalid, frame number will already be negative
		}
	}
	return -1;
}

/* Ajoute dans le TLB une entrée qui associe `frame_number` à `page_number` */
static void tlb__add_entry(unsigned int page_number, unsigned int frame_number,
			   bool readonly)
{
	/* Least-frequently used */
	int victim = 0;
	for (int i = 0; i < TLB_NUM_ENTRIES; i++) {
		if (tlb_entries[victim].frame_number < 0)
			break;

		if (tlb_entries[i].frame_number < 0 ||
		    tlb_entries[i].uses < tlb_entries[victim].uses)
			victim = i;
	}

	struct tlb_entry new_entry = { page_number, frame_number, readonly, 0 };
	tlb_entries[victim] = new_entry;
}

void tlb_add_entry(unsigned int page_number, unsigned int frame_number,
		   bool readonly)
{
	tlb_mod_count++;
	tlb__add_entry(page_number, frame_number, readonly);
}

int tlb_lookup(unsigned int page_number, bool write)
{
	int fn = tlb__lookup(page_number, write);
	(*(fn < 0 ? &tlb_miss_count : &tlb_hit_count))++;
	return fn;
}

/* Imprime un sommaires des accès */
void tlb_clean(void)
{
	printf("TLB misses   : %3u\n", tlb_miss_count);
	printf("TLB hits     : %3u\n", tlb_hit_count);
	printf("TLB changes  : %3u\n", tlb_mod_count);
	printf("TLB miss rate: %.1f%%\n",
	       100 * tlb_miss_count / (0.01 + tlb_hit_count + tlb_miss_count));
}
