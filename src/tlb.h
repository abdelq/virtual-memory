#ifndef TLB_H
#define TLB_H

void tlb_init(FILE * log);
int tlb_lookup(unsigned int page_number, bool write);
void tlb_add_entry(unsigned int page_number, unsigned int frame_number,
		   bool readonly);
void tlb_clean(void);

#endif
