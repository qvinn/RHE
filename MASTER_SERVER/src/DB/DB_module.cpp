#include "DB_module.h"

static int callback(void *data, int argc, char **argv, char **azColName) {
	int i;
	fprintf(stderr, "%s: ", (const char*)data);
	for(i = 0; i<argc; i++) {
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

DB_module::DB_module()
{
	
}

int DB_module::create_DB()
{
	/* Open database */
	rc = sqlite3_open("users.db", &db);
	
	if( rc ) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return(0);
		} else {
		fprintf(stdout, "Opened database successfully\n");
	}
	
	/* Create SQL statement */
	//const char *
	std::string sql = "CREATE TABLE USERS("  \
	"ID INT PRIMARY KEY NOT NULL," \
	"FIRST_NAME CHAR(50) NOT NULL," \
	"SECOND_NAME CHAR(50) NOT NULL," \
	"LOGIN CHAR(50) NOT NULL," \
	"PASSWORD CHAR(50) NOT NULL);";
	
	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
	
	if( rc != SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		} else {
		fprintf(stdout, "Table created successfully\n");
	}
	sqlite3_close(db);
}

std::string DB_module::form_sql_query(std::string query, std::vector<std::string>)
{
	//std::string str=base;           // "this is a test string."
	//str.replace(9,5,str2);          // "this is an example string." (1)
	
/* 	std::size_t found = str.find(str2);
	if (found!=std::string::npos)
	{
		std::cout << "first 'needle' found at: " << found << '\n';
	} */
    
}

