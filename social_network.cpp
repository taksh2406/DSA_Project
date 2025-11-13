#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include <sqlite3.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <cctype>
#include <unistd.h>

using namespace std;

void clearScreen()
{
  #ifdef _WIN32
system("cls");
#else
system("clear");
#endif
}

void waitAndClear()
{
  this_thread::sleep_for(std::chrono::milliseconds(900));
clearScreen();
}

class User //defining user
{
public:
string name;
string username;
string dob;
string gender;

User() {}

User(string n,string u, string d,string g)
: name(n),username(u),dob(d),gender(g) {}

void display() const
{
  cout << "Name: " << name << endl;
  cout << "Username: @" << username << endl;
  cout << "DOB: " << dob << endl;
cout << "Gender: " << gender << endl;
}
};
