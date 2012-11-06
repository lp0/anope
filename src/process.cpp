/* Main processing code for Services.
 *
 * (C) 2003-2012 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

#include "services.h"
#include "modules.h"
#include "extern.h"
#include "protocol.h"
#include "servers.h"
#include "users.h"
#include "regchannel.h"

/** Main process routine
 * @param buffer A raw line from the uplink to do things with
 */
void process(const Anope::string &buffer)
{
	/* If debugging, log the buffer */
	Log(LOG_RAWIO) << "Received: " << buffer;

	/* Strip all extra spaces */
	Anope::string buf = buffer;
	while (buf.find("  ") != Anope::string::npos)
		buf = buf.replace_all_cs("  ", " ");

	if (buf.empty())
		return;

	Anope::string source;
	if (buf[0] == ':')
	{
		size_t space = buf.find_first_of(" ");
		if (space == Anope::string::npos)
			return;
		source = buf.substr(1, space - 1);
		buf = buf.substr(space + 1);
		if (source.empty() || buf.empty())
			return;
	}

	spacesepstream buf_sep(buf);
	Anope::string buf_token;

	Anope::string command = buf;
	if (buf_sep.GetToken(buf_token))
		command = buf_token;
	
	std::vector<Anope::string> params;
	while (buf_sep.GetToken(buf_token))
	{
		if (buf_token[0] == ':')
		{
			if (!buf_sep.StreamEnd())
				params.push_back(buf_token.substr(1) + " " + buf_sep.GetRemaining());
			else
				params.push_back(buf_token.substr(1));
			break;
		}
		else
			params.push_back(buf_token);
	}

	if (protocoldebug)
	{
		Log() << "Source : " << (source.empty() ? "No source" : source);
		Log() << "Command: " << command;

		if (params.empty())
			Log() << "No params";
		else
			for (unsigned i = 0; i < params.size(); ++i)
				Log() << "params " << i << ": " << params[i];
	}

	const std::vector<IRCDMessage *> *messages = IRCDMessage::Find(command);

	if (messages && !messages->empty())
	{
		MessageSource src(source);

		bool retVal = true;
		/* Newer messages take priority */
		for (unsigned i = messages->size(); retVal && i > 0; --i)
		{
			IRCDMessage *m = messages->at(i - 1);

			if (m->HasFlag(IRCDMESSAGE_SOFT_LIMIT) ? (params.size() < m->GetParamCount()) : (params.size() != m->GetParamCount()))
				Log(LOG_DEBUG) << "invalid parameters for " << command << ": " << params.size() << " != " << m->GetParamCount();
			else if (m->HasFlag(IRCDMESSAGE_REQUIRE_USER) && !src.GetUser())
				Log(LOG_DEBUG) << "unexpected non-user source " << source << " for " << command;
			else if (m->HasFlag(IRCDMESSAGE_REQUIRE_SERVER) && !source.empty() && !src.GetServer())
				Log(LOG_DEBUG) << "unexpected non-server soruce " << source << " for " << command;
			else
				retVal = m->Run(src, params);
		}
	}
	else
		Log(LOG_DEBUG) << "unknown message from server (" << buffer << ")";
}

