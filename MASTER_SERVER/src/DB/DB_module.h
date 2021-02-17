#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sqlite3.h> 

class DB_module {
	
	public:
		DB_module();
		int create_DB();
		
	private:	
		std::string form_sql_query(std::string query, std::vector<std::string>);
		sqlite3 *db;
		char *zErrMsg = 0;
		int rc;
		
};