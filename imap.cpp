#include "imap.hpp"
#include <iostream>

using namespace IMAP;
using namespace std;

Message::Message(uint32_t uid, Session* session):UID(uid), session(session)
{ 	// asigning default values in case object does not contain feild
	info_set["body"] = "EMPTY";
	info_set["Subject"] = "EMPTY";
	info_set["date_sent"] = "EMPTY";
	info_set["in_reply_to"] = "EMPTY";
	info_set["sender_name"] = "EMPTY";
	info_set["From"] = "EMPTY";
}

string Message::getBody()
{
	return info_set["body"];
			
}

string Message::getField(string fieldname)
{	
	 return info_set[fieldname];
}

void Message::deleteFromMailbox()
{
	struct mailimap_set* set = mailimap_set_new_single(UID);
	struct mailimap_flag* deleted = mailimap_flag_new_deleted();
	struct mailimap_flag_list* list = mailimap_flag_list_new_empty();
	mailimap_flag_list_add(list, deleted);
	struct mailimap_store_att_flags* set_flag = mailimap_store_att_flags_new_set_flags(list);

	int function_result = mailimap_uid_store(session->new_sn, set, set_flag);
	check_error(function_result, "COULD NOT SET DELETE FLAG");

	function_result = mailimap_expunge(session->new_sn);
	check_error(function_result,"COULT NOT DELETE MESSAGE");

	mailimap_store_att_flags_free(set_flag);
	mailimap_set_free(set);

	for (int m = 0; session->message_array[m] != nullptr; m++)
	{
		if (session->message_array[m]->UID != UID)
		{
			delete session->message_array[m];
		}
	}

	delete [] session->message_array;
	session->updateui();
	delete this;
}

void Message::extract_info()
{
	struct mailimap_set* set = mailimap_set_new_single(UID);
	struct mailimap_fetch_type* type = mailimap_fetch_type_new_fetch_att_list_empty();
	struct mailimap_fetch_att* att = mailimap_fetch_att_new_envelope();
	mailimap_fetch_type_new_fetch_att_list_add(type, att);

	struct  mailimap_section* section = mailimap_section_new(nullptr);
    struct  mailimap_fetch_att* body_att = mailimap_fetch_att_new_body_section(section);
    mailimap_fetch_type_new_fetch_att_list_add(type, body_att);
    
    clist* result;
	int function_result = mailimap_uid_fetch(session->new_sn, set, type, &result);
    check_error(function_result,"COULT NOT GET MESSAGE");

    clistiter* iter = clist_begin(result); // contains only one clistcell (at least should)
    mailimap_msg_att* msg_att = (mailimap_msg_att*)clist_content(iter);
	//clistiter* iter;
    for (iter = clist_begin(msg_att->att_list); iter != nullptr; iter = clist_next(iter)) 
	{
		mailimap_msg_att_item* msg_att_item = (mailimap_msg_att_item*)clist_content(iter);
		if (msg_att_item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC) // Contained in static att type
			continue;

		if (msg_att_item->att_data.att_static->att_type == MAILIMAP_MSG_ATT_BODY_SECTION) 
		{
			if (msg_att_item->att_data.att_static->att_data.att_body_section->sec_body_part)
			info_set["body"] = msg_att_item->att_data.att_static->att_data.att_body_section->sec_body_part;
			// can also get body via the .att_body member but given all the different possible types that can  
			// take the above is more efficient
		}

		if (msg_att_item->att_data.att_static->att_type == MAILIMAP_MSG_ATT_ENVELOPE) 
		{ 
			if (msg_att_item->att_data.att_static->att_data.att_env->env_subject)
			//  msg_att_item->att_data.att_static->att_data.att_env->env_subject;
			info_set["Subject"] = msg_att_item->att_data.att_static->att_data.att_env->env_subject;
			
			if (msg_att_item->att_data.att_static->att_data.att_env->env_date)
			info_set["date_sent"] = msg_att_item->att_data.att_static->att_data.att_env->env_date;

			if (msg_att_item->att_data.att_static->att_data.att_env->env_in_reply_to)
			info_set["in_reply_to"] = msg_att_item->att_data.att_static->att_data.att_env->env_in_reply_to;
			
			if (msg_att_item->att_data.att_static->att_data.att_env->env_from->frm_list)
			{
				clistiter* cell; // List because possibly multiple senders
				for (cell = clist_begin(msg_att_item->att_data.att_static->att_data.att_env->env_from->frm_list); 
										cell != nullptr; cell = clist_next(cell)) 
				{
					mailimap_address* address = (mailimap_address*)clist_content(cell);
					
					if (address->ad_personal_name) 
						info_set["sender_name"] = address->ad_personal_name;
					
					if (address->ad_host_name && address->ad_mailbox_name) 
					{
						info_set["From"] = address->ad_mailbox_name;
						info_set["From"] += "@";
						info_set["From"] += address->ad_host_name;
					}
				}
			} // add cc at some point
		}
    }
    
    mailimap_fetch_list_free(result);
    mailimap_fetch_type_free(type);
    mailimap_set_free(set);
}

// 			SESSION

Session::Session(function<void()> updateUI): message_array(nullptr)/////
{
	new_sn = mailimap_new(0, NULL);
	updateui = updateUI;
}

Message** Session:: getMessages()/////
{	// getting the nr of messages via the uid, mailimap_fetch,clist_count route  
	// does not workd as mailimap_fetch returns an error (nr 9) when there are 0 messages in the mailbox
	// have to get the nr of messages via the mailbox status way

	struct mailimap_status_att_list * status_att = mailimap_status_att_list_new_empty();
	mailimap_status_att_list_add(status_att,MAILIMAP_STATUS_ATT_MESSAGES);
	mailimap_mailbox_data_status* result_return;

	int function_result = mailimap_status(new_sn,"INBOX",status_att,&result_return);
	check_error(function_result,"COULT NOT GET NR OF MESSAGES");

	int no_of_messages = ((mailimap_status_info*)clist_content(clist_begin(result_return->st_info_list)))->st_value;
	
	mailimap_mailbox_data_status_free(result_return);
    mailimap_status_att_list_free(status_att);
	
	message_array = new Message*[no_of_messages+1];
	if (no_of_messages==0)
	{
		message_array[0] = nullptr;
		return message_array;
	}

	struct mailimap_set* set;
	set = mailimap_set_new_interval(1,0);
	struct mailimap_fetch_type* type; // mailimap_fetch_type is the description of the fetch OPERATION
	struct mailimap_fetch_att* att; // mailimap_fetch_att is the description of the fetch ATTRIBUTE
	att = mailimap_fetch_att_new_uid(); // requestS the unique identifier of a message
	type = mailimap_fetch_type_new_fetch_att(att); // function creates a mailimap_fetch_type structure to 
												   // request the given fetch attribute, i.e. the UID
	clist * result;
	function_result = mailimap_fetch(new_sn, set, type, &result);
	check_error(function_result, "COULD NOT GET MESSAGES");

	clistiter* iter;
	int index = 0;
	for (iter = clist_begin(result); iter != nullptr; iter = clist_next(iter)) 
	{
		mailimap_msg_att* msg_att = (mailimap_msg_att*)clist_content(iter);
		uint32_t uid = get_uid(msg_att);
		if (uid) 
		{
			message_array[index] = new Message(uid,this);// initialises message object
			// given getBody() and getField() take either no arguments or just a string
			// it makes sense to store the relevant message info now
			message_array[index]->extract_info();
			index++;
		}
	}
	message_array[index] = nullptr; // terminates with null pointer at the end

	mailimap_fetch_list_free(result);
	mailimap_fetch_type_free(type);
	mailimap_set_free(set);

	return message_array;
}

void Session::connect(string const& server, size_t port) const
{
	int function_result = mailimap_socket_connect(new_sn, server.c_str(), port);
	check_error(function_result, "COULD NOT CONNECT TO SERVER");
}

void Session::login(string const& userid, string const& password) const
{
	int function_result = mailimap_login(new_sn, userid.c_str(), password.c_str());
	check_error(function_result, "COULD NOT LOG IN");
}

void Session::selectMailbox(string const& mailbox) const
{
	int function_result = mailimap_select(new_sn, mailbox.c_str());
	check_error(function_result, "COULD NOT SELECT MAILBOX");
}
		
void Session::logout() const
{
	int function_result = mailimap_logout(new_sn);
	check_error(function_result, "COULD NOT LOG OUT");
	mailimap_free(new_sn);
}

uint32_t Session::get_uid(mailimap_msg_att* messageg_att) const // the uid in the message staic attribute is uint32_t
{
    clistiter* cur;

    for (cur = clist_begin(messageg_att->att_list); cur != nullptr; cur = clist_next(cur)) 
	{
		mailimap_msg_att_item* obj = (mailimap_msg_att_item*)clist_content(cur);
		
		if (obj->att_type == MAILIMAP_MSG_ATT_ITEM_STATIC) // uid is static for each message
		{ 
			if (obj->att_data.att_static->att_type == MAILIMAP_MSG_ATT_UID)
				return obj->att_data.att_static->att_data.att_uid;
		}	
    }
    
    return 0;
}
Session::~Session()
{
	for (int m = 0; message_array[m] != nullptr; m++) // deleting all messages from memory
	{
		delete message_array[m];
	}

	delete [] message_array;
	mailimap_logout(new_sn);
	mailimap_free(new_sn);
}
