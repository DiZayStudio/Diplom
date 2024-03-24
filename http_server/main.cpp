#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

#include <pqxx/pqxx>

#include "http_connection.h"
#include <Windows.h>
#include "iniParser.h"
#include "struct.h"

Configure conf;

void httpServer(tcp::acceptor& acceptor, tcp::socket& socket)
{
	acceptor.async_accept(socket,
		[&](beast::error_code ec)
		{
			if (!ec)
				std::make_shared<HttpConnection>(std::move(socket))->start(conf);
			httpServer(acceptor, socket);
		});
}


int main(int argc, char* argv[])
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	setvbuf(stdout, nullptr, _IOFBF, 1000);

	try
	{	
		// Загружаем настройки 
		IniParser iniParser;
		// загрузка IP и Port из ini файла
		iniParser.parse("config.ini", conf);
		auto const address = net::ip::make_address(conf.host);
		unsigned short port = conf.port;



		net::io_context ioc{1};

		tcp::acceptor acceptor{ioc, { address, port }};
		tcp::socket socket{ioc};
		httpServer(acceptor, socket);

		std::cout << "Open browser and connect to http://localhost:8080 to see the web server operating" << std::endl;

		ioc.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}