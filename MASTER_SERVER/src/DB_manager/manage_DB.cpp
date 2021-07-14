#include "DB_module.h"
#include <iostream>

int main(int argc, char* argv[]){
	
	DB_module *db = new DB_module();
	std::string cmd;
	
	while(1)
    {
        std::cout << "Enter command -> ";
		std::cin >> cmd;
		
/* 		switch(cmd)
		{
			case "create_db":
			{
				db->create_DB();
				break;
			}
			
			case "tst":
			{
				db->string form_sql_query(std::string("Please, ? the vowels in this sentence by asterisks."),std::vector<std::string>{"rep"});
				break;
			}
			
			case "t_insert":
			{
				break;
			}
			
			case "t_select":
			{
				break;
			}
		} */

        if(cmd == "create_db")
        {
			db->create_DB();
        } else if(cmd == "t_insert")
        {
			db->insert_new_user(user_info{"Barbos","Pupkin","V4a","V4a",0});
        } else if(cmd == "t_select")
        {
			db->select_all_users();
        } else if(cmd == "help")
        {
			std::cout << "*\t create_db \n"
			<< "*\t t_select \n"
			<< "*\t t_insert \n\n";
        } 
    }
}