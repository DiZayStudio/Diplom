#pragma once

#include <iostream>

struct Configure {
	std::string host;
	std::string startPage;
	int depth;
	std::string dbHost;
	std::string dbPort;
	std::string dbName;
	std::string dbUser;
	std::string dbPass;
};