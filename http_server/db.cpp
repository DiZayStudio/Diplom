#include "db.h"
#include <iostream>
#include <string.h>

DataBase::DataBase(){}

void DataBase::SetConnection(std::unique_ptr<pqxx::connection> in_c)
{
	c_ = std::move(in_c);
}


void DataBase::InsertData(const std::map<std::string, int>& words, const Link& link) {
	pqxx::work tx{ *c_ };
	std::string query;

	// сохраняем ссылку
	query = "INSERT INTO Documents VALUES ( nextval('documents_id_seq'::regclass), "
		"'" + link.protocol + "', '" + link.hostName + "', '" + link.query + "') RETURNING id";
	
	int id_dokument = tx.query_value<int>(query);
	//tx.commit();
	int id_word = 0;
	for (const auto element : words) {
		// поиск слова в таблице 
		query = "SELECT id FROM Words WHERE word='" + element.first + "'";
		id_word = tx.query_value<int>(query);
		if (id_word == 0) {
			query = "INSERT INTO Words VALUES ( nextval('words_id_seq'::regclass), '" + element.first + "') RETURNING id";
			id_word = tx.query_value<int>(query);
		}
		
		query = "INSERT INTO DocumentsWords(docLink_id, word_id, count) "
				"VALUES (" + std::to_string(id_dokument)+", "+std::to_string(id_word)+", "+std::to_string(element.second)+ ") ";
		// загрузить индекс слова
		tx.exec(query);
	}
	tx.commit();
};

void DataBase::ClearTable(const std::string tableName) {
	pqxx::work tx{ *c_ };
	tx.exec("DELETE from " + tableName);
	tx.commit();
}

int DataBase::GetIdWord(const std::string word) {
	pqxx::work tx{ *c_ };
	// поиск слова в таблице 
	std::string query = "SELECT id FROM Words WHERE word='" + word + "'";
	int id_word = tx.query_value<int>(query);
	tx.exec(query);
	return id_word;
}

std::map<int, int> DataBase::GetWordCount(const int idWord) {
	pqxx::work tx{ *c_ };
	// поиск слова в таблице
	std::map<int, int> wordCount;

	for (auto [docId, count] : tx.query<int, int>("SELECT docLink_id, count FROM DocumentsWords WHERE word_id='" + std::to_string(idWord)+ "'")){
		wordCount.insert({ docId , count });
	}
	return wordCount;
}

Link DataBase::GetLink(const int docId) {
	pqxx::work tx{ *c_ };
	Link lk;
	for (std::tuple<std::string, std::string, std::string> tpl: tx.query<std::string, std::string, std::string>("SELECT protocol, hostname, query FROM Documents WHERE id = '" + std::to_string(docId) + "'")) {
		lk.protocol = std::get<0>(tpl);
		lk.hostName = std::get<1>(tpl);
		lk.query = std::get<2>(tpl);
	}

	return lk;
}