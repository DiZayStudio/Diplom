#pragma once
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "struct.h"
#include <string>

class IniParser
{
public:

    void parse(const std::string& filename, Configure& conf)
    {
        boost::property_tree::ptree pt;

        try
        {
            boost::property_tree::ini_parser::read_ini(filename, pt);

            conf.protocol = pt.get<std::string>("spider.protocol", "https://");
            conf.host = pt.get<std::string>("spider.host", "en.wikipedia.org");
            conf.startPage = pt.get<std::string>("spider.startPage", "/wiki/Main_Page");
            conf.depth = pt.get<int>("spider.depth", 0);

            conf.dbHost = pt.get<std::string>("db.host", "pg3.sweb.ru");
            conf.dbPort = pt.get<std::string>("db.port", "5432");
            conf.dbName = pt.get<std::string>("db.name", "samozaschi");
            conf.dbUser = pt.get<std::string>("db.user", "samozaschi");
            conf.dbPass = pt.get<std::string>("db.pass", "GQ5B57BNY$MY56Gw");
        }
        catch (boost::property_tree::ptree_error& ex)
        {
            std::cout << ex.what() << std::endl;
            std::cout << "Copy the <config.ini> file to the application folder" << std::endl;
        }
    }
};