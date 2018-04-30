#ifndef PT_H
#define PT_H

void pt_init(FILE * log);
int pt_lookup(unsigned int page_number);
void pt_set_entry(unsigned int page_number, unsigned int frame_number);
void pt_unset_entry(unsigned int page_number);
bool pt_readonly_p(unsigned int page_number);
void pt_set_readonly(unsigned int page_number, bool readonly);
void pt_clean(void);

#endif
