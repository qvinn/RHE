#include "DB_module.h"
#include <iostream>

int main(int argc, char* argv[]){
	
	DB_module *db = new DB_module();
	std::string cmd;
	
	while(1)
    {
        std::cin >> cmd;

        if(cmd == "create_db")
        {
			db->create_DB();
        } else if(cmd == "t_insert")
        {
			
        } else if(cmd == "t_select")
        {
			
        }
    }
}