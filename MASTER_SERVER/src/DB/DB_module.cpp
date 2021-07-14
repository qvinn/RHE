#include "DB_module.h"

#include <iostream>			// for write to wile with c++
#include <fstream>			// for write to wile with c++

//---STATIC--!!!
static int callback(void *data, int argc, char **argv, char **azColName) {
	int i;
	fprintf(stderr, "%s: ", (const char*)data);
	for(i = 0; i<argc; i++) {
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

static int select_callback(void *data, int argc, char **argv, char **azColName) {
	int i;
	//fprintf(stderr, "%s: ", (const char*)data);
/* 	for(i = 0; i<argc; i++) {
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	} */
	
	//reinterpret_cast<std::vector<user_info>*>(data)->push_back(user_info{"Barbos","Pupkin","V4a","V4a",0});
	reinterpret_cast<std::vector<user_info>*>(data)->push_back(user_info{std::stoi(argv[0]),
																		std::string(argv[1]),
																		std::string(argv[2]),
																		std::string(argv[3]),
																		std::string(argv[4]),
																		std::stoi(argv[5])
																		});
	
	//printf("\n");
	return 0;
}

//---PUBLIC--!!!

DB_module::DB_module()
{
	// Проверим наличие .ini-файла
	std::ifstream db_file("users.db");
	if(!db_file.good())
	{
		this->create_DB();
		this->select_all_users();
	}
}

bool DB_module::create_DB()
{
	/* Open database */
	rc = sqlite3_open("users.db", &db);
	
	if( rc ) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return(false);
		} else {
		fprintf(stdout, "Opened database successfully\n");
	}
	
	std::string sql = "CREATE TABLE USERS("  \
	"USER_ID		INTEGER		PRIMARY KEY AUTOINCREMENT," \
	"FIRST_NAME		CHAR(50)	NOT NULL," \
	"SECOND_NAME	CHAR(50)	NOT NULL," \
	"LOGIN			CHAR(50)	NOT NULL," \
	"PASSWORD		CHAR(50)	NOT NULL," \
	"APPROVE		BOOL		NOT NULL);";
	
	rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
	
	if( rc != SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		} else {
		fprintf(stdout, "Table created successfully\n");
		return false;
	}
	sqlite3_close(db);
	return true;
}

bool DB_module::insert_new_user(user_info info)
{
	rc = sqlite3_open("users.db", &db);
	
	if( rc ) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return(false);
		} else {
		fprintf(stdout, "Opened database successfully\n");
	}
	
	std::string sql = "INSERT INTO USERS(FIRST_NAME,SECOND_NAME,LOGIN,PASSWORD,APPROVE) " \
						"VALUES(?,?,?,?,?)";
	std::vector<std::string> parameters{info.first_name,
										info.second_name,
										info.login,
										info.password,
										std::to_string(info.approve)};
	sql = form_sql_query(sql,parameters);

	rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
	
	if( rc != SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		} else {
		fprintf(stdout, "New user added successfully\n");
		return true;
	}
	sqlite3_close(db);
	return true;	
}

bool DB_module::select_all_users()
{
	rc = sqlite3_open("users.db", &db);

	if( rc ) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return(false);
		} else {
		fprintf(stdout, "Opened database successfully\n");
	}
	
	std::string sql = "SELECT * FROM USERS";
	
	users_buffer = {};
	rc = sqlite3_exec(db, sql.c_str(), select_callback, &users_buffer, &zErrMsg);
	look_all_users(users_buffer);
	
	if( rc != SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		} else {
		fprintf(stdout, "SELECT operation successfully\n");
		return false;
	}
	sqlite3_close(db);
	return true;	
}

bool DB_module::user_exist(std::string _login)
{
	for(int i = 0; i < users_buffer.size(); i++)
	{
		if(users_buffer.at(i).login.compare(_login) == 0) // && users_buffer.at(i).password.compare(_password) == 0
			{
				return true;
			}
	}
	return false;
}

int DB_module::user_exist_approved(std::string _login, std::string _password)
{
	for(int i = 0; i < users_buffer.size(); i++)
	{
		if(users_buffer.at(i).login.compare(_login) == 0 &&  users_buffer.at(i).password.compare(_password) == 0)
		{
			// Нашли данного пользователя
			if(users_buffer.at(i).approve > 0)
			{
				return 0;
			} else 
			{
				return -2;
			}				
		}
	}
	return -1;
}

bool DB_module::get_first_name_second_name(std::string _login, std::string *_first_name, std::string *_second_name)
{
	for(int i = 0; i < users_buffer.size(); i++)
	{
		if(users_buffer.at(i).login.compare(_login) == 0)
		{
			*_first_name = users_buffer.at(i).first_name;
			*_second_name = users_buffer.at(i).second_name;
			return true;
		}
	}
	return false;
}

bool DB_module::user_set_approved(int user_id, int approve)
{
	rc = sqlite3_open("users.db", &db);
	
	if( rc ) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return(false);
		} else {
		fprintf(stdout, "Opened database successfully\n");
	}
	
	std::string sql = "UPDATE USERS set APPROVE = ? WHERE USER_ID= ?;";
	
	std::vector<std::string> parameters{
	std::to_string(approve),
	std::to_string(user_id)
							};								
	
	sql = form_sql_query(sql,parameters);
	
	rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
	
	if( rc != SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		} else {
		fprintf(stdout, "New user added successfully\n");
		return true;
	}
	sqlite3_close(db);
	return true;
}

bool DB_module::delete_user(int user_id)
{
	rc = sqlite3_open("users.db", &db);
	
	if( rc ) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return(false);
		} else {
		fprintf(stdout, "Opened database successfully\n");
	}
	
	std::string sql = "DELETE FROM USERS WHERE USER_ID= ?;";
	
	std::vector<std::string> parameters{std::to_string(user_id)};								
	
	sql = form_sql_query(sql,parameters);
	
	rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
	
	if( rc != SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		} else {
		fprintf(stdout, "New user added successfully\n");
		return true;
	}
	sqlite3_close(db);
	return true;	
}

//---PRIVATE--!!!

std::string DB_module::form_sql_query(std::string str, std::vector<std::string> vec)
{
	int i = 0;
	std::vector<std::string> parameters = prepare_parameters(vec);
	std::size_t found = str.find_first_of("?");
	while (found!=std::string::npos)
	{
		str.replace(found, 1, "");
		str.insert(found,parameters.at(i));
		found=str.find_first_of("?",found+1);
		i++;
	}

	//std::cout << str << '\n';
   
   return str;
}

std::vector<std::string> DB_module::prepare_parameters(std::vector<std::string> vec)
{
	std::vector<std::string> result;
	std::string tmp_str;
	for(int i = 0; i < vec.size(); i++)
	{
		tmp_str = vec.at(i);
		tmp_str.insert(0,"\'");
		tmp_str.insert(tmp_str.length(),"\'");
		result.push_back(tmp_str);
	}
	return result;
}

void DB_module::look_all_users(std::vector<user_info> users_vec)
{
	std::cout << "users size: " << users_vec.size() << "\n";
	std::cout << "user_id\tfirst_name\tsecond_name\tlogin\tpassword\tapprove\n";
	for(int i = 0; i < users_vec.size(); i++)
	{

		std::cout << users_vec.at(i).user_id << "\t"
					<< users_vec.at(i).first_name << "\t"
					<< users_vec.at(i).second_name << "\t"
					<< users_vec.at(i).login << "\t"
					<< users_vec.at(i).password << "\t"
					<< users_vec.at(i).approve << "\t" << "\n";
	}
}