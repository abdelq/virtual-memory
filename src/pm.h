#ifndef PHYSICAL_MEMORY_H
#define PHYSICAL_MEMORY_H

void pm_init(FILE * backing_store, FILE * log);
void pm_download_page(unsigned int page_number, unsigned int frame_number);
void pm_backup_page(unsigned int frame_number, unsigned int page_number);
char pm_read(unsigned int physical_address);
void pm_write(unsigned int physical_address, char);
unsigned int pm_find_free(void);
unsigned int pm_find_victim(void);
bool pm_is_dirty(unsigned int frame_number);
int pm_get_page(unsigned int frame_number);
void pm_update_usage(unsigned int frame_number);
void pm_clean(void);

#endif
