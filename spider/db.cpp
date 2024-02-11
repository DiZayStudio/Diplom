#include "db.h"
#include <iostream>
#include <string.h>

DataBase::DataBase(){}

void DataBase::SetConnection(std::unique_ptr<pqxx::connection> in_c)
{
	c_ = std::move(in_c);
}

void DataBase::CreateTable() {
	pqxx::work tx{ *c_ };

	//tx.exec("DROP table Documents");
	//tx.exec("DROP table Words");
	//tx.exec("DROP table DocumentsWords");

	tx.exec("CREATE TABLE IF NOT EXISTS Documents (id SERIAL PRIMARY KEY, protocol integer NOT NULL, "
			"hostName VARCHAR(250) NOT NULL, query VARCHAR(250) NOT NULL); ");
	tx.exec("CREATE TABLE IF NOT EXISTS Words (id SERIAL PRIMARY KEY, word VARCHAR(32) UNIQUE NOT NULL); ");
	tx.exec("CREATE TABLE IF NOT EXISTS DocumentsWords (docLink_id integer NOT NULL, word_id integer NOT NULL, count integer NOT NULL); ");

	tx.commit();
}

void DataBase::InsertData(const std::map<std::string, int>& words, const Link& link) {
	pqxx::work tx{ *c_ };
	std::string query;

	// сохраняем ссылку
	query = "INSERT INTO Documents VALUES ( nextval('words_id_seq'::regclass), "
		"'" + std::to_string(int(link.protocol)) + "', '" + link.hostName + "', '" + link.query + "')";
	tx.exec(query);

	//int id_document = tx.query_value<std::int>("SELECT id FROM Documents WHERE id = 3");
	 
	for (const auto element : words) {
		query = "INSERT INTO words VALUES ( nextval('words_id_seq'::regclass), '" + element.first + "')";
		tx.exec(query);
		// загрузить индекс слова
	//	query = "INSERT INTO DocumentsWords VALUES ( nextval('words_id_seq'::regclass), '" + element.first + "')";
		tx.exec(query);
		tx.commit();
	}
	
	
};

void DataBase::ClearTable(const std::string tableName) {
	pqxx::work tx{ *c_ };
	tx.exec("DELETE from " + tableName);
	tx.commit();
}

	