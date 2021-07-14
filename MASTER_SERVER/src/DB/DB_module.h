#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sqlite3.h> 

typedef struct user_info {
			int user_id;
			std::string first_name;
			std::string second_name;
			std::string login;
			std::string password;
			int approve; // true|false
		} user_info;


// https://www.tutorialspoint.com/sqlite/sqlite_c_cpp.htm

class DB_module {
	
	public:
	
		DB_module();
		bool create_DB();
		bool insert_new_user(user_info info);
		bool select_all_users();
		bool user_exist(std::string _login);
		bool user_exist_approved(std::string _login, std::string _password);
		bool user_set_approved(int user_id, int approve);
		

	private:
		sqlite3 *db;
		char *zErrMsg = 0;
		int rc;
		
		std::vector<user_info> users_buffer;
		
		std::string form_sql_query(std::string query, std::vector<std::string> vec);
		std::vector<std::string> prepare_parameters(std::vector<std::string> vec);
		
		void look_all_users(std::vector<user_info> users_vec);
		
};