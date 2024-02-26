#pragma once
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include "link.h"

class DataBase
{
public:
    DataBase();
    void SetConnection(std::unique_ptr<pqxx::connection> in_c);

    DataBase(const DataBase&) = delete;
    DataBase& operator=(const DataBase&) = delete;

    void InsertData(const std::map<std::string, int>& words, const Link& link);
    void DataBase::ClearTable(const std::string tableName);
    int DataBase::GetIdWord(const std::string word);
    std::map<int, int> DataBase::GetWordCount(const int idWord);
    Link DataBase::GetLink(const int id);

private:  

    std::unique_ptr<pqxx::connection> c_;
};
