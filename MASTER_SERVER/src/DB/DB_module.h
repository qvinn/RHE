#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sqlite3.h> 

// Структа, описывающая информацию о пользователе в БД
typedef struct user_info {
			int user_id;				// id Пользователя(уникальный)
			std::string first_name;		// Имя пользователя
			std::string second_name;	// Фамилия пользователя
			std::string login;			// Логин
			std::string password;		// Пароль
			int approve; // true|false	// Поле подтверждения
		} user_info;


// https://www.tutorialspoint.com/sqlite/sqlite_c_cpp.htm

class DB_module {
	
	public:
	
		DB_module();
		
		// Метод для создания БД
		bool create_DB();
		
		// Метод для добавления нового пользователя в БД
		// user_info info - Структура описывающая пользователя
		bool insert_new_user(user_info info);
		
		// Мтод для выборки всех пользователей - ВНИМАНИЕ! Этот метод обновлят вектор users_buffer
		// Поэтому, рекомендуется вызывать этот метод каждый раз, когда выполнены изменения в БД
		bool select_all_users();
		
		// Метод для проврки того, существует ли пользователь с таким логинов в сиситеме
		// std::string _login - Логин пользователя
		bool user_exist(std::string _login);
		
		// Метод для проверки того, существует ли пользователь с таким логинов в сиситеме
		// !!! И при этом, подтвржден ли этот пользователь или нет
		/*
			* std::string _login	- Логин
			* std::string _password	- Пароль
		*/
		int user_exist_approved(std::string _login, std::string _password);
		
		// Метод для задания поля подтвердждения у пользователя по его id
		/*
			* int user_id	- id Пользователя в БД
			* int approve	- значение поля, которое означает "подтвержден/не подтвержден"
		*/
		bool user_set_approved(int user_id, int approve);
		
		// Метод для удаления пользователя из БД
		// int user_id - id Пользователя
		bool delete_user(int user_id);
		
		// Метод для получения Имени и Фамилии пользователя по его Логину
		/*
			* std::string _login		- Логин
			* std::string *_first_name	- Указатель на строку Имя
			* std::string *_second_name	- Указатель на строку Фамилия
		*/
		bool get_first_name_second_name(std::string _login, std::string *_first_name, std::string *_second_name);
		

	private:
		// Указатель на БД
		sqlite3 *db;
		
		// Указатель на строку с сообщением об ошибке
		char *zErrMsg = 0;
		
		// Переменная по которой анализируем успешность выполнения операций
		int rc;
		
		// Вектор, который хранит в ОЗУ список пользователей(так как предполагается,
		// что одновременно подключенныйх пользователей будет относительно не много, это 
		// безопасное и быстродействующее решение)
		// Поэтому, при каждом изменении БД, рекомендуется выполнять <select_all_users()>,
		// чтобы в ОЗУ хранились всегда свежие данные
		std::vector<user_info> users_buffer;
		
		// Данный метод преобразует каждый символ "?", в std::string, который соответствут [i] в vec
		// ПРИМЕР: query = "Я ? программист", при vec = {"хороший"} -->
		// Получим: "Я хороший программист" НО!!!
		// ВНИМАНИЕ: на самом деле получим "Я 'хороший' программист"(см. <prepare_parameters>)
		/*
			* std::string query				- Строка, которая подлежит обработке
			* std::vector<std::string> vec	- Набор параметорв, которыми будут заменены символы "?"
			* return						- Строка после редактирования
		*/
		std::string form_sql_query(std::string query, std::vector<std::string> vec);
		
		// Данный метод используется в методе <form_sql_query>
		// Задача этого метода предварительно преобразовать каждый параметр (std::string) так,
		// чтобы каждая строка была обрамлена символом ' (ПРИМЕР: "хороший" --> "'хороший'")
		// 
		// Это необходимо для того, чтобы потом, подставляя необходимые параметры, каждый имел
		// обрамление в виде '~~~', так как SQL принимает параметры в таком виде.
		/*
			* std::vector<std::string> vec	- Входной вектор
			* return						- Выходной вектор
		*/
		std::vector<std::string> prepare_parameters(std::vector<std::string> vec);
		
		// Метод для отображения в консоли всех пользоватетей
		void look_all_users(std::vector<user_info> users_vec);
		
};