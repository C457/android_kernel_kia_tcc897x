//#define DBG
// if use memory dump log, uncomment follow define.
//#define DBG_DUMP

//#undef dev_dbg
//#define dev_dbg dev_info

/*
#ifdef DBG
#define PRINT_DEBUG(...) printk(__VA_ARGS__)
#else
#define PRINT_DEBUG(...)
#endif
*/

#ifdef DBG
 #define PRINT_DEBUG(...) printk(__VA_ARGS__)
 #undef pr_debug
 #define pr_debug pr_err
 #undef pr_info
 #define pr_info pr_err
#else
#define PRINT_DEBUG(...)
#endif

#ifdef DBG_DUMP
#define DBG_DUMP_MEMORY(buf, len) dbg_dump_memory(buf, len)
#else
#define DBG_DUMP_MEMORY(buf, len)
#endif

struct list_entry {
	struct list_entry *prev;
	struct list_entry *next;
}__attribute__ ((packed));

void initialize_list_head(struct list_entry *head);
void add_list_head(struct list_entry *head, struct list_entry *entry);
void add_list_tail(struct list_entry *head, struct list_entry *entry);
void remove_list(struct list_entry *entry);
void dbg_dump_memory(char *buffer, int length);
