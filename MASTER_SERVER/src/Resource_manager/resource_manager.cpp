#include "resource_manager.h"

#ifdef __linux__
	#include <sys/mman.h>
#else
	#include "mmap-windows.h"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>

#include <openssl/md5.h>

#include <sstream>      // std::istringstream

//#define PRINT_INFO
#define PRINT_INFO_2

// PUBLIC-------------------------------------------------------------------

Resource_manager::Resource_manager(std::string _resource_dir)
{
	// Создадим необходимые папки
	std::string cmd = "mkdir " + _resource_dir;	// Папка с ресурсами
	system(cmd.c_str());
	cmd = "mkdir tmp";							// Папка с временным хранилищем
	system(cmd.c_str());
	cmd = "rm -r -f tmp/*";						// Очистим папку с временным хранилищем
	system(cmd.c_str());
	
	this->resource_dir = _resource_dir;
	// Получим информацию о всех файлах в папке + сохраним названия файлов
	this->get_dir_info(_resource_dir);	
	
#ifdef PRINT_INFO
    look_resources_filenames();
#endif
	
	// Создадим хешь для всех обнаруженных файлов
	for(int i = 0; i < dir_vec.size(); i++)
	{
		dir_vec[i].hash = create_hash(resource_dir + dir_vec.at(i).file_name);
	}
	
#ifdef PRINT_INFO_2
	std::cout << "Create recourses: \n";
	for(int i = 0; i < dir_vec.size(); i++)
	{
		std::cout << "server file name: " << dir_vec.at(i).file_name << "\tserver file hash:" << dir_vec.at(i).hash << "\n";
	}
#endif
}

void Resource_manager::look_resources_filenames()
{
	for(int i = 0; i < dir_vec.size(); i++)
	{
		std::cout << "file name:" << dir_vec.at(i).file_name << "\n";
	}
}

std::vector<Resource_manager::file_info> Resource_manager::get_dir_vec()
{
	return dir_vec;
}

// PRVATE-------------------------------------------------------------------

bool Resource_manager::get_dir_info(std::string _resource_dir)
{
	DIR *dir = opendir(_resource_dir.c_str());
	if(dir)
	{
		struct dirent *ent;
		while((ent = readdir(dir)) != NULL)
		{
			dir_vec.push_back(file_info{std::string(ent->d_name),std::string()});
			//std::cout << "file name:" << ent->d_name << "\n";
		}
	}
	else
	{
		fprintf(stderr, "Error opening directory\n");
		return false;
	}
	
	// Убрать такие элементы из списка, как "." и ".."

	int counter = 0;
	while(counter != dir_vec.size())
	{
		counter = 0;
		for(int i = 0; i < dir_vec.size(); i++)
		{
			if((dir_vec.at(i).file_name.compare(".") == 0) || (dir_vec.at(i).file_name.compare("..") == 0))
			{
				dir_vec.erase(dir_vec.begin() + i);
			} else 
			{
				counter++;
			}
		}
	}
	
	return true;
}

void Resource_manager::print_md5_sum(unsigned char* md) {
    int i;
    for(i=0; i <MD5_DIGEST_LENGTH; i++) {
            printf("%02x",md[i]);
    }
}

std::string Resource_manager::md5_sum_to_string(unsigned char* md, int size)
{
	char result[MD5_DIGEST_LENGTH*2];
	for(int i=0; i <size; i++)
	{
		sprintf(&result[i*2],"%02x",md[i]);
	}
	//printf("result: %s\n",result);
	// Получим строку в двоичном формате
	std::string binary_str(reinterpret_cast<char const*>(result),MD5_DIGEST_LENGTH*2);
	return binary_str;
}

unsigned long Resource_manager::get_size_by_fd(int fd) {
    struct stat statbuf;
    if(fstat(fd, &statbuf) < 0) exit(-1);
    return statbuf.st_size;
}

std::string Resource_manager::create_hash(std::string file_name)
{
	int file_descript;
    unsigned long file_size;
    char* file_buffer;
	unsigned char result[MD5_DIGEST_LENGTH];

#ifdef PRINT_INFO
    printf("using file:\t%s\n", file_name.c_str());
#endif

    file_descript = open(file_name.c_str(), O_RDONLY);
    if(file_descript < 0)
	{
		printf("Can't open file - STOP APPLICATION!\n");
		exit(-1);
	}

    file_size = get_size_by_fd(file_descript);
#ifdef PRINT_INFO
    printf("file size:\t%lu\n", file_size);
#endif
	
    file_buffer = static_cast<char*>(mmap(0, file_size, PROT_READ, MAP_SHARED, file_descript, 0));
	MD5((unsigned char*) file_buffer, file_size, result);
    munmap(file_buffer, file_size);
	
#ifdef PRINT_INFO
    print_md5_sum(result); printf("  %s\n", file_name.c_str());
#endif
	
	return md5_sum_to_string(result,MD5_DIGEST_LENGTH);
}