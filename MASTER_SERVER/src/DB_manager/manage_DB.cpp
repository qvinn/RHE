#include "DB_module.h"
#include <iostream>

int main(int argc, char* argv[]){
	
	DB_module *db = new DB_module();
	std::string cmd;
	
	std::string new_user__first_name;
	std::string new_user__second_name;
	std::string new_user__login;
	std::string new_user__password;
	
	int choose_user;
	int approve_mode;
	
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
			std::cout << "\nenter first_name: ";	std::cin >> new_user__first_name;			
			std::cout << "\nenter second_name: ";	std::cin >> new_user__second_name;			
			std::cout << "\nenter login: ";			std::cin >> new_user__login;			
			std::cout << "\nenter password: ";		std::cin >> new_user__password;	
			
			db->insert_new_user(user_info{0,
				new_user__first_name,
				new_user__second_name,
				new_user__login,
				new_user__password,
				0});
				
			db->select_all_users();
			
        } else if(cmd == "t_select")
        {
			db->select_all_users();
        } else if(cmd == "t_approve")
        {
			std::cout << "\nchoose user_id for approve: "; std::cin >> choose_user;			
			std::cout << "\nchoose approve mode: "; std::cin >> approve_mode;			
			db->user_set_approved(choose_user,approve_mode);
			db->select_all_users();
        } else if(cmd == "t_delete")
        {
			std::cout << "\nchoose user_id for delete: "; std::cin >> choose_user;			
			db->delete_user(choose_user);
			db->select_all_users();
        } else if(cmd == "help")
        {
			std::cout << "*\t create_db\n"
			<< "*\t t_select\n"
			<< "*\t t_approve\n"
			<< "*\t t_delete\n"
			<< "*\t t_insert\n\n";
        } 
    }
}