// #define	DEBUG			// error path messages, extra info
// #define	VERBOSE			// more; success messages

#include <linux/module.h>

#include "config.h"
#include "util.h"

#define DUMP_MEMORY_BYTE_PER_LINE	16

/*
 * Initialize list head
 *
 * Parameters
 *   head : pointer of list header
 *
 * Return value
 *   NONE
 */
void initialize_list_head(struct list_entry *head)
{
	head->next = 
	head->prev = head;
}

/*
 * Append node to list
 *
 * Parameters
 *   head : pointer of list header
 *   entry : node
 *
 * Return value
 *   NONE
 */
void add_list_head(struct list_entry *head, struct list_entry *entry)
{
	entry->next = head->next;
	entry->prev = head;
	head->next->prev = entry;
	head->next = entry;
}

/*
 * Append node to list
 *
 * Parameters
 *   head : pointer of list header
 *   entry : node
 *
 * Return value
 *   NONE
 */
void add_list_tail(struct list_entry *head, struct list_entry *entry)
{
	entry->next = head;
	entry->prev = head->prev;
	head->prev->next = entry;
	head->prev = entry;
}

/*
 * Remove node to list
 *
 * Parameters
 *   entry : node
 *
 * Return value
 *   NONE
 */
void remove_list(struct list_entry *entry)
{
	entry->next->prev = entry->prev;
	entry->prev->next = entry->next;
	entry->next = entry->prev = entry;
}

/*
 * Print memory dump
 *
 * Parameters
 *   buffer : memory buffer
 *   length : buffer length
 *
 * Return value
 *   NONE
 */
void dbg_dump_memory(char *buffer, int length)
{
	int i = 0;

	if (buffer == NULL)
		return;

	printk("debug dump memory 0x%0lx (%d bytes)", (unsigned long)buffer, length);
	for (i = 0 ; i < length ; i++) {
		if ((i % DUMP_MEMORY_BYTE_PER_LINE) == 0) printk("\n");
		printk("%02x ", (unsigned char)buffer[i]);
	}
	printk("\ndebug dump memory end\n");
}

