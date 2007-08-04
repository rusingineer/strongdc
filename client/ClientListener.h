#ifndef CLIENTLISTENER_H_
#define CLIENTLISTENER_H_

#include "forward.h"

class ClientListener
{
public:
	virtual ~ClientListener() { }
	template<int I>	struct X { enum { TYPE = I }; };

	typedef X<0> Connecting;
	typedef X<1> Connected;
	typedef X<3> UserUpdated;
	typedef X<4> UsersUpdated;
	typedef X<5> UserRemoved;
	typedef X<6> Redirect;
	typedef X<7> Failed;
	typedef X<8> GetPassword;
	typedef X<9> HubUpdated;
	typedef X<11> Message;
	typedef X<12> StatusMessage;
	typedef X<13> PrivateMessage;
	typedef X<14> HubUserCommand;
	typedef X<15> HubFull;
	typedef X<16> NickTaken;
	typedef X<17> SearchFlood;
	typedef X<18> NmdcSearch;
	typedef X<19> AdcSearch;
	typedef X<20> CheatMessage;
	typedef X<21> HubTopic;

	virtual void on(Connecting, const Client*) throw() { }
	virtual void on(Connected, const Client*) throw() { }
	virtual void on(UserUpdated, const Client*, const OnlineUser&) throw() { }
	virtual void on(UsersUpdated, const Client*, const OnlineUser::List&) throw() { }
	virtual void on(UserRemoved, const Client*, const OnlineUser&) throw() { }
	virtual void on(Redirect, const Client*, const string&) throw() { }
	virtual void on(Failed, const Client*, const string&) throw() { }
	virtual void on(GetPassword, const Client*) throw() { }
	virtual void on(HubUpdated, const Client*) throw() { }
	virtual void on(Message, const Client*, const OnlineUser&, const string&) throw() { }
	virtual void on(StatusMessage, const Client*, const string&) throw() { }
	virtual void on(PrivateMessage, const Client*, const OnlineUser&, const OnlineUser&, const OnlineUser&, const string&) throw() { }
	virtual void on(HubUserCommand, const Client*, int, int, const string&, const string&) throw() { }
	virtual void on(HubFull, const Client*) throw() { }
	virtual void on(NickTaken, const Client*) throw() { }
	virtual void on(SearchFlood, const Client*, const string&) throw() { }
	virtual void on(NmdcSearch, Client*, const string&, int, int64_t, int, const string&, bool) throw() { }
	virtual void on(AdcSearch, const Client*, const AdcCommand&, const CID&) throw() { }
	virtual void on(CheatMessage, const Client*, const string&) throw() { }
	virtual void on(HubTopic, const Client*, const string&) throw() { }
};


#endif /*CLIENTLISTENER_H_*/
