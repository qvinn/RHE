#include <iostream> // for write to wile with c++
#include <fstream>	// for write to wile with c++

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#define MD5_DIGEST_LENGTH 16

// ВНИМАНИЕ!

// При передаче данных использовть "короткие" мьютексы, то бишь такие мьютексы, которые будут
// использоваться лишь для получения какой-то информации, а не для комплексной задачи

class Resource_manager
{	
	
public:

	// Структура, которая описывает файл
	typedef struct file_info{
		std::string file_name;	// Название файла
		std::string hash;		// Хэш-сумма файла
	}file_info;
	
	Resource_manager(std::string _resource_dir);
	
	void look_resources_filenames();
	
	std::vector<file_info> get_dir_vec();
	
private:
	std::string resource_dir;

	// Вектор, который хранит в себе информацию о файлах, которые хранятся в папке "resource_dir"
	//(см. конструктор)
	std::vector<file_info> dir_vec;
	
	// Получение информации о файлах в директории(информация об имени файлов)
	bool get_dir_info(std::string resource_dir);
	
	// Метод для отображения md5-хэша в hex-формате
	void print_md5_sum(unsigned char* md);
	
	std::string md5_sum_to_string(unsigned char* md, int size);
	
	// Метод для получения размера файла(файлового дескриптора)
	unsigned long get_size_by_fd(int fd);
	
	// Метод для генерации md5-хэша
	std::string create_hash(std::string file_name);
	
};