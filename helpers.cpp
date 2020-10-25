struct mailimap_status_att_list 
{
	clist* att_list; /* list of (uint32_t *) */
};

struct mailimap_mailbox_data_status 
{
	const char* st_mailbox;
	clist* st_info_list;
	mailimap_mailbox_data_status(const char* name, clist* list=nullptr) : st_mailbox(name) {}
	
}:
