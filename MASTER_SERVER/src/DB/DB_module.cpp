#include "DB_module.h"

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
	reinterpret_cast<std::vector<user_info>*>(data)->push_back(user_info{std::string(argv[1]),
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
	
	/* Create SQL statement */
	//const char *
	std::string sql = "CREATE TABLE USERS("  \
	"USER_ID INT	SERIAL PRIMARY KEY," \
	"FIRST_NAME		CHAR(50)	NOT NULL," \
	"SECOND_NAME	CHAR(50)	NOT NULL," \
	"LOGIN			CHAR(50)	NOT NULL," \
	"PASSWORD		CHAR(50)	NOT NULL," \
	"APPROVE		BOOL		NOT NULL);";
	
	/* Execute SQL statement */
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
	/* Open database */
	rc = sqlite3_open("users.db", &db);
	
	if( rc ) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return(false);
		} else {
		fprintf(stdout, "Opened database successfully\n");
	}
	
	/* Create SQL statement */
	std::string sql = "INSERT INTO USERS(FIRST_NAME,SECOND_NAME,LOGIN,PASSWORD,APPROVE) " \
						"VALUES(?,?,?,?,?)";
	std::vector<std::string> parameters{info.first_name,
										info.second_name,
										info.login,
										info.password,
										std::to_string(info.approve)};
	sql = form_sql_query(sql,parameters);
/* 	std::string sql = "INSERT INTO USERS(FIRST_NAME,SECOND_NAME,LOGIN,PASSWORD,APPROVE) " \
	"VALUES('Vasya','Pupkin','V4a','V4a','1')"; */
	
	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
	
	if( rc != SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		} else {
		fprintf(stdout, "New user added successfully\n");
		return false;
	}
	sqlite3_close(db);
	return true;	
}

bool DB_module::select_all_users()
{
	/* Open database */
	rc = sqlite3_open("users.db", &db);
	//const char* data = "Callback function called";
	//std::vector<user_info> data;
	
	if( rc ) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return(false);
		} else {
		fprintf(stdout, "Opened database successfully\n");
	}
	
	/* Create SQL statement */
	std::string sql = "SELECT * FROM USERS";
	
	/* Execute SQL statement */
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

bool DB_module::user_exist(std::string _login, std::string _password)
{
	for(int i = 0; i < users_buffer.size(); i++)
	{
		if(users_buffer.at(i).login.compare(_login) == 0 && 
			users_buffer.at(i).password.compare(_password) == 0)
			{
				return true;
			}
	}
	return false;
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
	std::cout << "first_name\tsecond_name\tlogin\tpassword\tapprove\n";
	for(int i = 0; i < users_vec.size(); i++)
	{
		std::cout << users_vec.at(i).first_name << "\t"
					<< users_vec.at(i).second_name << "\t"
					<< users_vec.at(i).login << "\t"
					<< users_vec.at(i).password << "\t"
					<< users_vec.at(i).approve << "\t" << "\n";
	}
}