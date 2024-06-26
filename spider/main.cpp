#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>

#include <regex>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

#include <map>
#include "http_utils.h"
#include <functional>

#include "iniParser.h"
#include "struct.h"

#include <algorithm>
#include <cctype> 
#include <pqxx/pqxx>
#include "db.h"

#include <Windows.h>
#pragma execution_character_set( "utf-8")

std::mutex mtx;
std::condition_variable cv;
std::queue<std::function<void()>> tasks;
bool exitThreadPool = false;
DataBase db;

std::map<std::string, int> getWorldsAndIndex(const std::string text) {
	std::regex word_regex("(\\b\\w{3,32}\\b)", std::regex_constants::ECMAScript);
	auto words_begin =
		std::sregex_iterator(text.begin(), text.end(), word_regex);
	auto words_end = std::sregex_iterator();
	int n = std::distance(words_begin, words_end);
	std::vector<std::string> words;

	for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
		std::smatch match = *i;
		words.push_back(match.str());
	}

	// ���������� ����
	std::map<std::string, int> indexWords;

	for (int i = 0; i < words.size(); ++i) {
		if (indexWords.find(words[i]) != indexWords.end()) {
			indexWords[words[i]] += 1;
		}
		else {
			indexWords.insert({ words[i], 1});
		}
	}
	return indexWords;
}

std::vector<std::string> getHtmlLink(const std::string html) {
	std::vector<std::string> links;
	std::regex link_regex("<a href=\"(.*?)\"");
	auto link_begin = std::sregex_iterator(html.begin(), html.end(), link_regex);
	auto link_end = std::sregex_iterator();

	std::vector<std::string> words;

	for (std::sregex_iterator i = link_begin; i != link_end; ++i) {
		std::string sTemp;
		std::smatch match = *i;
			if (match[1].matched) {
				sTemp = std::string(match[1].first, match[1].second);
			}
		links.push_back(sTemp);
	}	
	return links;
}

void threadPoolWorker() {
	std::unique_lock<std::mutex> lock(mtx);
	while (!exitThreadPool || !tasks.empty()) {
		if (tasks.empty()) {
			cv.wait(lock);
		}
		else {
			auto task = tasks.front();
			tasks.pop();
			lock.unlock();
			task();
			lock.lock();
		}
	}
}

std::string RemoveHTMLTags(const std::string s) {
	// �������� HEAD
	std::string  str = s;
	int index = s.find("<body");
	str.replace(0,index,"");

	// �������� ��������
	std::regex script("<script[\\s\\S]*?>[\\s\\S]*?<\/script>", std::regex_constants::ECMAScript);
	str = regex_replace(str, script, " ");

	std::regex pattern_style("<style[^>]*?>[\\s\\S]*?<\/style>", std::regex_constants::ECMAScript);
	str = regex_replace(str, pattern_style, " ");

	std::regex pattern_a("<a[^>]*?>[\\s\\S]*?<\/a>", std::regex_constants::ECMAScript);
	str = regex_replace(str, pattern_a, " ");

	// �������� HTML �����
	std::regex pattern("\\<(\/?[^\\>]+)\\>", std::regex_constants::ECMAScript);
	str = regex_replace(str, pattern, " ");

	std::regex re("(\\n|\\t|[0-9]|\\s+)", std::regex_constants::ECMAScript);
	str = regex_replace(str, re, " ");

	boost::algorithm::to_lower(str);

	//system("cls");
	//std::cout << str << std::endl;

	return str;
}

void parseLink(const Link& link, int depth )
{
	try {

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		std::string html = getHtmlContent(link);

 		if (html.size() == 0)
		{
			std::cout << "Failed to get HTML Content" << std::endl;
			return;
		}

		// ������� �� HTML �����, ������ ���������� � ���������
		std::string text = RemoveHTMLTags(html);

		// ������ ������� ��������� ����	
		std::map<std::string, int> words = getWorldsAndIndex(text);

		// ���������� ������ � �� 
		if (db.SearchLink(link)) {
			db.InsertData(words, link);
			std::cout << "Stranica proindeksirovana: " << link.protocol << link.hostName << link.query << std::endl;
		} else 
			std::cout << "Stranica uge est v BD: " << link.protocol << link.hostName << link.query << std::endl;
		// TODO: Collect more links from HTML code and add them to the parser like that:
		std::vector<Link> links;
		
		// ����� ������ �� ������ �������� 
		std::vector<std::string> linkStr = getHtmlLink(html);
		for (int i = 0; i < linkStr.size(); ++i ) {
			Link tmp;
			// (http[s]?:\/\/)?(\\w{3}\\.)?(\\w+[\\.\\w+]+)?([/\\\S+]+:?\\w+[\\.\\w+]?(php)?)*(\\?\\S+)?
			std::regex ex("(http[s]?:)*(\/\/)?(\\w{3}\\.)?(\\w+[\\.\\w+]+)?([\/\\w+.?:?=?&?;]*)*(#\\w+)?", std::regex_constants::ECMAScript);
			std::cmatch what;
			if (std::regex_search(linkStr[i].c_str(), what, ex)) {
				// ��������
				if (what[1].matched) {
					tmp.protocol = std::string(what[1].first, what[1].second);
					if (what[2].matched) {
						tmp.protocol += std::string(what[2].first, what[2].second);
					}
				}
				else {
					tmp.protocol = link.protocol;
				}		
				if (what[3].matched) {
					tmp.hostName = std::string(what[3].first, what[3].second);
				}
				if (what[4].matched) {
					tmp.hostName += std::string(what[4].first, what[4].second);
				}
				else {
					tmp.hostName = link.hostName;
				}
				if (what[5].matched) {
					tmp.query = std::string(what[5].first, what[5].second);
				}
				links.push_back(tmp);
			}	
		}

		if (depth > 0) {
			std::lock_guard<std::mutex> lock(mtx);

			size_t count = links.size();
			size_t index = 0;
			for (auto& subLink : links)
			{
				if (db.SearchLink(subLink)) {
					tasks.push([subLink, depth]() { parseLink(subLink, depth - 1); });
				}
			}
			cv.notify_one();
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
}

int main()
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	setvbuf(stdout, nullptr, _IOFBF, 1000);

	Configure conf;
	// ��������� ��������� 
	IniParser iniParser;
	iniParser.parse("config.ini", conf);

	std::string CONST_CONNECTION = "host=" + conf.dbHost + " port=" + conf.dbPort + " dbname=" + conf.dbName +
								   " user=" + conf.dbUser + " password=" + conf.dbPass;
		
	try {

	std::unique_ptr<pqxx::connection> c = std::make_unique<pqxx::connection>(CONST_CONNECTION);
	db.SetConnection(std::move(c));

	//db.DeleteTable();

	db.CreateTable();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	try {
		int numThreads = std::thread::hardware_concurrency();
		std::vector<std::thread> threadPool;

		for (int i = 0; i < numThreads; ++i) {
			threadPool.emplace_back(threadPoolWorker);
		}

		Link link{ conf.protocol, conf.host, conf.startPage };

		{
			std::lock_guard<std::mutex> lock(mtx);
			tasks.push([link, conf]() { parseLink(link, conf.depth - 1); });
			cv.notify_one();
		}

		std::this_thread::sleep_for(std::chrono::seconds(2));

		{
			std::lock_guard<std::mutex> lock(mtx);
			exitThreadPool = true;
			cv.notify_all();
		}

		for (auto& t : threadPool) {
			t.join();
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}
