#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>

struct ImageEntry {
	std::string name;
	std::string category;
	std::vector<unsigned char> data;
};

class Database {
public:
	bool open(const std::string& dbPath);
	bool getImageByName(const std::string& name, std::vector<unsigned char>& outData);
	std::vector<ImageEntry> getAllImages();
	void close();
private:
	struct sqlite3* db = nullptr;
};

#endif