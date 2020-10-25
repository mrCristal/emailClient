#ifndef IMAP_H
#define IMAP_H
#include "imaputils.hpp"
#include <libetpan/libetpan.h>
#include <string>
#include <functional>
#include <map>

namespace IMAP{
class Session;
class Message {
	uint32_t const UID;
	std::map <std::string,std::string> info_set; //keys are fields, values are field info
	Session* session;
public:
	
	Message(uint32_t uid, Session* session);
	/**
	 * Get the body of the message. You may chose to either include the headers or not.
	 */
	std::string getBody();
	/**
	 * Get one of the descriptor fields (subject, from, ...)
	   Assuming all fieldnames will be in lower case!!!!!*/
	std::string getField(std::string fieldname);
	/**
	 * Remove this mail from its mailbox
	 */
	void deleteFromMailbox();
	/*
	* construct the info_set member
	*/
	void extract_info();
};

class Session {
public:

	mailimap* new_sn;
	Message** message_array;
	std::function<void()> updateui;

	Session(std::function<void()> updateUI);

	/**
	 * Get all messages in the INBOX mailbox terminated by a nullptr (like we did in class)
	 */
	Message** getMessages();

	/**
	 * connect to the specified server (143 is the standard unencrypte imap port)
	 */
	void connect(std::string const& server, size_t port = 143) const;

	/**
	 * log in to the server (connect first, then log in)
	 */
	void login(std::string const& userid, std::string const& password) const;

	/**
	 * select a mailbox (only one can be selected at any given time)
	 * 
	 * this can only be performed after login
	 */
	void selectMailbox(std::string const& mailbox) const;

	void logout() const;

	uint32_t get_uid(mailimap_msg_att* messageg_att) const;
	

	~Session();
};
}
#endif /* IMAP_H */
