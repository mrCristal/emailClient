 #include "imap.hpp"
 #include <"helpers.cpp">

using namespace IMAP;
using namespace std;
		//int ID, UID;

Message::Message(uint32_t uid, Session* session):UID(uid), session(session){}

string Message::getBody()
{
	size_t m_length;
	auto type = mailimap_fetch_type_new_fetch_att_list_empty();
	auto set = mailimap_set_new_single(UID); 

	auto env_att = mailimap_fetch_att_new_envelope(); // Envelope contains the fields
    mailimap_fetch_type_new_fetch_att_list_add(type, env_att);

    // free section
    auto section = mailimap_section_new(nullptr); 

	//free bod section
    auto body_att = mailimap_fetch_att_new_body_section(section);

	//adds a given fetch attribute to the mailimap_fetch structure
    mailimap_fetch_type_new_fetch_att_list_add(type, body_att);
    
    clist* result;
	int conn_val = mailimap_uid_fetch((*session).new_sn, set, type, &result);
    check_error(conn_val,"COULD NOT GET BODY OF MESSAGE");

	clistiter * cur;

    for(cur = clist_begin(result) ; cur != NULL ; cur = clist_next(cur)) {
		struct mailimap_msg_att * msg_att;
		size_t m_size;
		char * m_content;
		
		msg_att = clist_content(cur);
		m_content = //getField(msg_att, "body")//get_msg_att_msg_content(msg_att, &m_size);
		if (m_content == NULL) {
			continue;
		}
		return m_content;
	}
	
	return NULL;

    mailimap_fetch_list_free(result);
    mailimap_fetch_type_free(type);
    mailimap_set_free(set);
			
}
// need to have the object to look in for the field
string Message::getField(struct mailimap_msg_att * msg_att,string fieldname)
		{
		
		clistiter * cur;
	
		/* iterate on each result of one given message */
			for(cur = clist_begin(msg_att->att_list) ; cur != NULL ; cur = clist_next(cur)) {
				struct mailimap_msg_att_item * item;
				
				item = clist_content(cur);
				if (item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC) {
					continue;
				}
				
			if (item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_BODY_SECTION) 
			{
					continue;
			}
			// the message content is in the static message attributes
				*p_msg_size = item->att_data.att_static->att_data.att_body_section->sec_length; 
				// dont get the point of this p_msg_size; not used anywhere
				return item->att_data.att_static->att_data.att_body_section->sec_body_part;
			}
			
			return NULL;
		}

void Message::deleteFromMailbox()
		{

		}
	

// 			SESSION

Session::Session(function<void()> updateUI);/////

Message** Session:: getMessages()/////
		{
			nr_of_msg = get_nr_of_msg();
			message_array = new Message[nr_of_msg+1]; // extra 1 for the null pointer at the end

			struct mailimap_set* set;
			set = mailimap_set_new_interval(1,0);
			struct mailimap_fetch_att* att;
			att = mailimap_fetch_att_new_uid();
			struct mailimap_fetch_type* type;
			type = mailimap_fetch_type_new_fetch_att(att);
			clist * result;

			int conn_val = mailimap_fetch(new_sn, set, type, &result)

			clistiter* iter;
			for (iter = clist_begin(result), int index =0; iter != nullptr; iter = clist_next(iter)) {
			mailimap_msg_att* msg_att = (mailimap_msg_att*)clist_content(iter);
			uint32_t uid = get_uid(msg_att);
			if (uid) {
				message_array[index] = new Message(uid,this);// fix Message() constructor
				//message_array[index]->getBody();
				index++;
			}
			}
			message_array[index] = nullptr;

			mailimap_fetch_list_free(result);
			mailimap_fetch_type_free(type);
			mailimap_set_free(set);
    
    		return messages;
		}

void Session:: connect(string const& server, size_t port = 143)/////
		{
			int conn_val = mailimap_socket_connect(new_sn, server, port);
			check_error(conn_val, "COULD NOT CONNECT");

		}

void Session::login(string const& userid, string const& password)/////
		{
			int conn_val = mailimap_login(new_sn, usderid, password);
			check_error(conn_val, "COULD NOT LOG IN");
		}

void Session::selectMailbox(string const& mailbox)/////
		{
			int conn_val = mailimap_select(new_sn, mailbox);
			check_error(conn_val, "COULD NOT SELECT MAILBOX");
			
			struct mailimap_status_att_list* att_list;
			att_list = mailimap_status_att_list_new_empty(void);
			
			mailimap_status_att_list_add(att_list,MAILIMAP_STATUS_ATT_MESSAGES);
			mailimap_status_att_list_add(att_list,MAILIMAP_STATUS_ATT_UNSEEN);
			
			struct mailimap_mailbox_data_status mb_status("INBOX");
			//mb_status.st_mailbox = "INBOX"
		}
		
void Session::logout()/////
		{
			int conn_val = mailimap_logout(new_sn);
			check_error(conn_val, "COULD NOT LOG OUT");
			mailimap_free(new_sn);
		}

uint32_t Session::get_uid(mailimap_msg_att* messageg_att) 
{
    clistiter* cur;

    for (cur = clist_begin(messageg_att->att_list); cur != nullptr; cur = clist_next(cur)) 
	{
		mailimap_msg_att_item* obj = (mailimap_msg_att_item*)clist_content(cur);
		
		if (obj->att_type == MAILIMAP_MSG_ATT_ITEM_STATIC) 
		{ 
			if (obj->att_data.att_static->att_type == MAILIMAP_MSG_ATT_UID)
				return obj->att_data.att_static->att_data.att_uid;
		}	
    }
    
    return 0;
}
  

