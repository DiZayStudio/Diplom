#pragma once

#include <iostream>

struct Configure {
	std::string host;
	int port;

	std::string dbHost;
	std::string dbPort;
	std::string dbName;
	std::string dbUser;
	std::string dbPass;
};