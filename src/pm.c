#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "conf.h"
#include "pm.h"

static FILE *pm_backing_store;
static FILE *pm_log;
static char pm_memory[PHYSICAL_MEMORY_SIZE];

static int pm_pages[NUM_FRAMES];
static int pm_usage[NUM_FRAMES];
static bool pm_dirty[NUM_FRAMES];

static unsigned int download_count = 0;
static unsigned int backup_count = 0;
static unsigned int read_count = 0;
static unsigned int write_count = 0;
static unsigned int usage_count = 0;

/* Initialise la mémoire physique */
void pm_init(FILE * backing_store, FILE * log)
{
	pm_backing_store = backing_store;
	pm_log = log;
	memset(pm_memory, '\0', sizeof(pm_memory));
	//memset(pm_pages, 0, sizeof(pm_pages)); // XXX
	memset(pm_usage, 0, sizeof(pm_usage));
	memset(pm_dirty, false, sizeof(pm_dirty));	// XXX
}

/* Charge la page demandée du backing store */
void pm_download_page(unsigned int page_number, unsigned int frame_number)
{
	if (page_number >= NUM_PAGES || frame_number >= NUM_FRAMES) {
		fprintf(stderr, "PM: invalid page or frame number\n");
		return;
	}

	if (fseek(pm_backing_store, page_number * PAGE_FRAME_SIZE, SEEK_SET) <
	    0) {
		perror("PM");
		return;
	}
	// XXX Buffer?
	fread(pm_memory + frame_number * PAGE_FRAME_SIZE,
	      sizeof(char), PAGE_FRAME_SIZE, pm_backing_store);

	if (ferror(pm_backing_store)) {
		fprintf(stderr, "PM: error in backing store\n");
		return;
	}

	download_count++;
	pm_pages[frame_number] = page_number;
}

/* Sauvegarde le frame spécifiée dans la page du backing store */
void pm_backup_page(unsigned int frame_number, unsigned int page_number)
{
	if (page_number >= NUM_PAGES || frame_number >= NUM_FRAMES) {
		fprintf(stderr, "PM: invalid page or frame number\n");
		return;
	}

	if (fseek(pm_backing_store, page_number * PAGE_FRAME_SIZE, SEEK_SET) <
	    0) {
		perror("PM");
		return;
	}
	// XXX Buffer?
	fwrite(pm_memory + frame_number * PAGE_FRAME_SIZE,
	       sizeof(char), PAGE_FRAME_SIZE, pm_backing_store);

	if (ferror(pm_backing_store)) {
		fprintf(stderr, "PM: error in backing store\n");
		return;
	}

	backup_count++;
	pm_dirty[frame_number] = false;
}

char pm_read(unsigned int physical_address)
{
	if (physical_address >= PHYSICAL_MEMORY_SIZE) {
		fprintf(stderr, "PM: invalid physical address\n");
		return '!';
	}

	read_count++;
	return pm_memory[physical_address];
}

void pm_write(unsigned int physical_address, char c)
{
	if (physical_address >= PHYSICAL_MEMORY_SIZE) {
		fprintf(stderr, "PM: invalid physical address\n");
		return;
	}

	write_count++;
	pm_memory[physical_address] = c;
	pm_dirty[physical_address / PAGE_FRAME_SIZE] = true;	// XXX
}

unsigned int pm_find_free()
{
	for (int i = 0; i < NUM_FRAMES; i++) {
		for (int j = 0; j < PAGE_FRAME_SIZE; j++) {
			if (pm_memory[i * PAGE_FRAME_SIZE + j] != '\0') {
				goto continue_outer;
			}
		}
		return i;
 continue_outer:;		// NOOP
	}
	return -1;
}

unsigned int pm_find_victim()
{
	/* Least-recently used */
	unsigned int frame = 0;
	for (int i = 0; i < NUM_FRAMES; i++)
		if (pm_usage[i] < pm_usage[frame])
			frame = i;
	return frame;
}

bool pm_is_dirty(unsigned int frame_number)
{
	return pm_dirty[frame_number];
}

int pm_get_page(unsigned int frame_number)
{
	return pm_pages[frame_number];
}

void pm_update_usage(unsigned int frame_number)
{
	usage_count++;
	pm_usage[frame_number] = usage_count;
}

void pm_clean(void)
{
	for (int i = 0; i < NUM_FRAMES; i++)
		if (pm_dirty[i])
			pm_backup_page(i, pm_pages[i]);

	// Enregistre l'état de la mémoire physique
	if (pm_log) {
		for (unsigned int i = 0; i < PHYSICAL_MEMORY_SIZE; i++) {
			if (i % 80 == 0)
				fprintf(pm_log, "%c\n", pm_memory[i]);
			else
				fprintf(pm_log, "%c", pm_memory[i]);
		}
	}

	printf("Page downloads: %2u\n", download_count);
	printf("Page backups  : %2u\n", backup_count);
	printf("PM reads : %4u\n", read_count);
	printf("PM writes: %4u\n", write_count);
}
