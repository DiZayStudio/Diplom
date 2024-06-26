#include "http_connection.h"

#include <sstream>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <iostream>
#include <pqxx/pqxx>
#include <regex>
#include "db.h"
#include <boost/algorithm/string.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

DataBase db;

std::string url_decode(const std::string& encoded) {
	std::string res;
	std::istringstream iss(encoded);
	char ch;

	while (iss.get(ch)) {
		if (ch == '%') {
			int hex;
			iss >> std::hex >> hex;
			res += static_cast<char>(hex);
		}
		else {
			res += ch;
		}
	}
	return res;
}

std::string convert_to_utf8(const std::string& str) {
	std::string url_decoded = url_decode(str);
	return url_decoded;
}

HttpConnection::HttpConnection(tcp::socket socket)
	: socket_(std::move(socket))
{
}


void HttpConnection::start(const Configure conf)
{
	config = conf;
	readRequest();
	checkDeadline();
}


void HttpConnection::readRequest()
{
	auto self = shared_from_this();

	http::async_read(
		socket_,
		buffer_,
		request_,
		[self](beast::error_code ec,
			std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);
			if (!ec)
				self->processRequest();
		});
}

void HttpConnection::processRequest()
{
	response_.version(request_.version());
	response_.keep_alive(false);

	switch (request_.method())
	{
	case http::verb::get:
		response_.result(http::status::ok);
		response_.set(http::field::server, "Beast");
		createResponseGet();
		break;
	case http::verb::post:
		response_.result(http::status::ok);
		response_.set(http::field::server, "Beast");
		createResponsePost();
		break;

	default:
		response_.result(http::status::bad_request);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body())
			<< "Invalid request-method '"
			<< std::string(request_.method_string())
			<< "'";
		break;
	}
	writeResponse();
}


void HttpConnection::createResponseGet()
{
	if (request_.target() == "/")
	{
		response_.set(http::field::content_type, "text/html");
		beast::ostream(response_.body())
			<< "<html>\n"
			<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
			<< "<body>\n"
			<< "<h1>Search Engine</h1>\n"
			<< "<p>Welcome!<p>\n"
			<< "<form action=\"/\" method=\"post\">\n"
			<< "    <label for=\"search\">Search:</label><br>\n"
			<< "    <input type=\"text\" id=\"search\" name=\"search\"><br>\n"
			<< "    <input type=\"submit\" value=\"Search\">\n"
			<< "</form>\n"
			<< "</body>\n"
			<< "</html>\n";
	}
	else
	{
		response_.result(http::status::not_found);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body()) << "File not found\r\n";
	}
}

void HttpConnection::createResponsePost()
{
	try {

		std::string CONST_CONNECTION = "host=" + config.dbHost + " port=" + config.dbPort + " dbname=" + config.dbName +
			" user=" + config.dbUser + " password=" + config.dbPass;

		std::unique_ptr<pqxx::connection> c = std::make_unique<pqxx::connection>(CONST_CONNECTION);
		db.SetConnection(std::move(c));

	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	if (request_.target() == "/")
	{
		std::string s = buffers_to_string(request_.body().data());

		std::cout << "POST data: " << s << std::endl;

		size_t pos = s.find('=');
		if (pos == std::string::npos)
		{
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "File not found\r\n";
			return;
		}

		std::string key = s.substr(0, pos);
		std::string value = s.substr(pos + 1);

		// ������� �����
		std::string utf8value = convert_to_utf8(value);
		
		//	������ ���� ��� ������
		//	utf8value = "additional album 	and name";

		boost::algorithm::to_lower(utf8value);

		if (key != "search")
		{
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "File not found\r\n";
			return;
		}

		// ��������� ��������� ���� �� �������
		std::regex word_regex("[\\w{3,30}]+"); //([.-]?\w{3,32})+   
		auto words_begin =
			std::sregex_iterator(utf8value.begin(), utf8value.end(), word_regex);
		auto words_end = std::sregex_iterator();
		
		int nWord = std::distance(words_begin, words_end);
		
		std::vector<std::string> searchResult;

		if (nWord > 0) {
			std::vector<std::string> words;

			for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
				std::smatch match = *i;
				if (match.str().size() > 2) {
					words.push_back(match.str());
				}
			}
			
			// ����� � �� ����, ���������� �������� 
			std::vector<int> wordId;
			for (int i = 0; i < words.size(); ++i) {
				wordId.push_back(db.GetIdWord(words[i]));
			}
			
			// ����������� ������� ����������
			std::map<int, int> reng;
			std::map<int, int>::iterator it;
			for (int i = 0; i < nWord; ++i){
				if (wordId[i] != 0) {
					auto documents = db.GetWordCount(wordId[i]);
					for (auto doc : documents) {
						it = reng.find(doc.first);
						if (it != reng.end()) {
							reng[doc.first] += doc.second;
						}
						else {
							reng[doc.first] = doc.second;
						}
					}
				}
			}
			if (reng.size() != 0) {
				std::multimap<int, int> revers_map;
				for (const auto& element : reng) {
					revers_map.insert({ element.second, element.first });
				}

				// TODO: Fetch your own search results here
				int i = 0;
				// ������� �� ���� ������ � ���������� � ������ 	
				for (auto iter = revers_map.end(); iter != revers_map.begin();) {
					iter--;
					if (i < 10) {
						Link l = db.GetLink(iter->second);
						std::string str = "";
						str = l.protocol + l.hostName + l.query;
						searchResult.push_back(str);
						i++;
					}
				}
			}
		}

		response_.set(http::field::content_type, "text/html");
		beast::ostream(response_.body())
			<< "<html>\n"
			<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
			<< "<body>\n"
			<< "<h1>Search Engine</h1>\n"
			<< "<p>Response:<p>\n"
			<< "<ul>\n";
		if (searchResult.size() != 0) {
			for (const auto& url : searchResult) {
				beast::ostream(response_.body())
					<< "<li><a href=\""
					<< url << "\">"
					<< url << "</a></li>";
			}
		}
		else {
			beast::ostream(response_.body())
				<< "<p>No words were found for your query</p>";
		}

		beast::ostream(response_.body())
			<< "</ul>\n"
			<< "</body>\n"
			<< "</html>\n";
	}
	else
	{
		response_.result(http::status::not_found);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body()) << "File not found\r\n";
	}
}

void HttpConnection::writeResponse()
{
	auto self = shared_from_this();

	response_.content_length(response_.body().size());

	http::async_write(
		socket_,
		response_,
		[self](beast::error_code ec, std::size_t)
		{
			self->socket_.shutdown(tcp::socket::shutdown_send, ec);
			self->deadline_.cancel();
		});
}

void HttpConnection::checkDeadline()
{
	auto self = shared_from_this();

	deadline_.async_wait(
		[self](beast::error_code ec)
		{
			if (!ec)
			{
				self->socket_.close(ec);
			}
		});
}