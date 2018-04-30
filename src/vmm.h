#ifndef VMM_H
#define VMM_H

void vmm_init(FILE * log);
char vmm_read(unsigned int logical_address);
void vmm_write(unsigned int logical_address, char);
void vmm_clean(void);

#endif
