#pragma once 
#include <string>
#include <unordered_set>

struct Link
{
	std::string protocol;
	std::string hostName;
	std::string query;

	bool operator==(const Link& l) const
	{
		return protocol == l.protocol
			&& hostName == l.hostName
			&& query == l.query;
	}
};

