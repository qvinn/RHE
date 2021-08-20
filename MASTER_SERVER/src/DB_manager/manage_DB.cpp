#include "DB_module.h"
#include <iostream>
#include <sstream>
#include <readline/history.h>
#include <readline/readline.h>
	
std::vector<std::string> vocabulory{"create_db"	,
									"t_select"	,
									"t_approve"	,
									"t_delete"	,
									"t_insert"	,
									"help"};

char *command_generator(const char *text, int state) {
  static std::vector<std::string> matches;
  static size_t match_index = 0;

  if (state == 0) {
    matches.clear();
    match_index = 0;

    std::string textstr(text);
    for (std::string word : vocabulory) {
      if (word.size() >= textstr.size() && word.compare(0, textstr.size(), textstr) == 0)
	  {
        matches.push_back(word);
      }
    }
  }

  if (match_index >= matches.size()) {
    return nullptr;
  } else {
    return strdup(matches[match_index++].c_str());
  }
}

char **command_completion(const char *text, int start, int end) {
  rl_attempted_completion_over = 1;
  return rl_completion_matches(text, command_generator);
}

int main(int argc, char* argv[]){

	rl_attempted_completion_function = command_completion;
	
	DB_module *db = new DB_module();
	std::string cmd;
	
	std::string new_user__first_name;
	std::string new_user__second_name;
	std::string new_user__login;
	std::string new_user__password;
	
	int choose_user;
	int approve_mode;
	
	char * buf;
	
	std::cout << "*\t help\n"
			<< "*\t create_db\n"
			<< "*\t t_select\n"
			<< "*\t t_approve\n"
			<< "*\t t_delete\n"
			<< "*\t t_insert\n\n";
	
	while((buf = readline("Enter command -> ")) != nullptr)
    {		
		cmd = std::string(buf);
		if (cmd.size() > 0)
		{
			add_history(buf);
		}
			
		free(buf);
		std::stringstream scmd(cmd);
		scmd >> cmd;
		

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
			db->look_all_users();
        } else if(cmd == "t_approve")
        {
			std::cout << "\nchoose user_id for approve: ";	std::cin >> choose_user;			
			std::cout << "\nchoose approve mode: ";			std::cin >> approve_mode;			
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