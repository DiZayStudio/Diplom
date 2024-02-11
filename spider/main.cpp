#include <iostream>
#include <fstream>
#include <vector>
#include <string>
//#include <boost/locale.hpp>
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
	std::regex word_regex("[\\w{3,30}]+"); //([.-]?\w{3,32})+    "[^\\s]+"
	auto words_begin =
		std::sregex_iterator(text.begin(), text.end(), word_regex);
	auto words_end = std::sregex_iterator();
	int n = std::distance(words_begin, words_end);
	std::vector<std::string> words;

	for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
		std::smatch match = *i;
		if (match.str().size() > 2) {
			words.push_back(match.str());
		}
	}

	// Индексация слов
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
	auto link_begin =
		std::sregex_iterator(html.begin(), html.end(), link_regex);
	auto link_end = std::sregex_iterator();

	std::vector<std::string> words;

	for (std::sregex_iterator i = link_begin; i != link_end; ++i) {
		std::smatch match = *i;
		links.push_back(match.str());
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
	// Удаление HEAD
	std::string  str = s;
	int index = s.find("<body");
	str.replace(0,index,"");

	// Удаление скриптов
//	std::regex script("<script[\s\S]*?>[\s\S]*?<\/script>"); // <script[^>]*>[\s\S]*?</\1>
//	str = regex_replace(str, script, " ");

	int indexStart;
	int indexStop;
	do {
		indexStart = str.find("<script");
		indexStop = str.find("</script");
		if (indexStart != -1) {
			str.replace(indexStart, indexStop, " ");
		}
	} while (indexStart != -1);

//	std::regex pattern_style("<style[^>]*?>[\s\S]*?<\/style>");  
//	str = regex_replace(str, pattern_style, " ");
	
	do {
		indexStart = str.find("<style");
		indexStop = str.find("</style");// ;
		if (indexStart != -1) {
			str.replace(indexStart, indexStop, " ");
		}
	} while (indexStart != -1);

	system("cls");
	std::cout << str << std::endl;

	// Удаление HTML тегов
	std::regex pattern("\<(/?[^\>]+)\>"); //<[^>]*>  
	str = regex_replace(str, pattern, " ");

	std::regex re("(\n|\t|[0-9])");
	str = regex_replace(str, re, "");

	std::regex point(",");
	str = regex_replace(str, point, " ");

//	std::regex simbol_replace("/[^\w\s\']|_/g");
//	str = regex_replace(str, simbol_replace, " ");

	boost::algorithm::to_lower(str);

	system("cls");
	std::cout << str << std::endl;

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

		// TODO: Parse HTML code 
		// Очистка от HTML тегов, знаков препинания и табуляции
		std::string text = RemoveHTMLTags(html);

		// Анализ частоты вхождения слов	
		std::map<std::string, int> words = getWorldsAndIndex(text);

		db.ClearTable("words");
		// Сохранение данных в БД 
		db.InsertData(words, link);

	//	std::cout << "Text content:" << std::endl;
	//	std::cout << text << std::endl;

		// TODO: Collect more links from HTML code and add them to the parser like that:
		std::vector<Link> links;
		
		// Поиск ссылок на другие страницы 
		std::vector<std::string> linkStr = getHtmlLink(html);
		for (int i = 0; i < linkStr.size(); ++i ) {
			Link tmp;
			// оставляем текст ссылки
			std::string strCopy = linkStr[i].substr(9, linkStr[i].size()-10);
			std::cout << strCopy << std::endl;
			std::regex ex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
			std::cmatch what;
			if (std::regex_search(strCopy.c_str(), what, ex)) {
				std::cout << "protocol: " << std::string(what[1].first, what[1].second) << std::endl;
				std::cout << "domain:   " << std::string(what[2].first, what[2].second) << std::endl;
				std::cout << "port:     " << std::string(what[3].first, what[3].second) << std::endl;
				std::cout << "path:     " << std::string(what[4].first, what[4].second) << std::endl;
				std::cout << "query:    " << std::string(what[5].first, what[5].second) << std::endl;
				std::cout << "fragment: " << std::string(what[6].first, what[6].second) << std::endl;
			
				// Добавление в вектор
			//	tmp.protocol = what[1].first, what[1].second;
				tmp.hostName = std::string(what[2].first, what[2].second);
				tmp.query = std::string(what[5].first, what[5].second);
				links.push_back(tmp);
			}
			else {	
				tmp.protocol = link.protocol;
				tmp.hostName = link.hostName;
				tmp.query = linkStr[i];
				links.push_back(tmp);
			}
		}
		//	{ProtocolType::HTTPS, "en.wikipedia.org", "/wiki/Wikipedia"},
		//	{ProtocolType::HTTPS, "wikimediafoundation.org", "/"},

		if (depth > 0) {
			std::lock_guard<std::mutex> lock(mtx);

			size_t count = links.size();
			size_t index = 0;
			for (auto& subLink : links)
			{
				tasks.push([subLink, depth]() { parseLink(subLink, depth - 1); });
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
	// Загружаем настройки 
	IniParser iniParser;
	iniParser.parse("config.ini", conf);

	std::string CONST_CONNECTION = "host=" + conf.dbHost + " port=" + conf.dbPort + " dbname=" + conf.dbName +
								   " user=" + conf.dbUser + " password=" + conf.dbPass;
		
	try {

	std::unique_ptr<pqxx::connection> c = std::make_unique<pqxx::connection>(CONST_CONNECTION);
	db.SetConnection(std::move(c));

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

		Link link{ ProtocolType::HTTPS, conf.host, conf.startPage };

		{
			std::lock_guard<std::mutex> lock(mtx);
			tasks.push([link]() { parseLink(link, 1); });
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
