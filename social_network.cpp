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

class AuthSystem
{
private:
    const string CREDENTIALS_FILE = "credentials.txt";

public:
    // Save encrypted credentials
    bool saveCredentials(string username, string password)
    {
        ofstream file(CREDENTIALS_FILE, ios::app);
        if (!file.is_open())
        {
            cerr << " Error opening credentials file!" << endl;
            return false;
        }

        string encrypted = encryptPassword(password);
        file << username << " " << encrypted << endl;
        file.close();
        return true;
    }
   bool usernameExists(string username)
    {
        ifstream file(CREDENTIALS_FILE);
        if (!file.is_open())
            return false;

        string user, pass;
        while (file >> user >> pass)
        {
            if (user == username)
            {
                file.close();
                return true;
            }
        }
        file.close();
        return false;
    }
  // Verify login using encrypted password
    bool verifyLogin(string username, string password)
    {
        ifstream file(CREDENTIALS_FILE);
        if (!file.is_open())
        {
            cerr << " No users registered yet!" << endl;
            return false;
        }

        string encrypted = encryptPassword(password);
        string user, pass;
        while (file >> user >> pass)
        {
            if (user == username && pass == encrypted)
            {
                file.close();
                return true;
            }
        }
        file.close();
        return false;
    }

    string signUp()
    {
        string username, password, confirmPass;

        cout << "\n===== SIGN UP =====" << endl;
        cout << "Enter username: ";
        cin >> username;
        clearScreen();

        if (usernameExists(username))
        {
            cout << " Username already exists! Try logging in." << endl;
            waitAndClear();
            return "";
        }

        cout << "Enter password: ";
        cin >> password;
        cout << "Confirm password: ";
        cin >> confirmPass;
        clearScreen();

        if (password != confirmPass)
        {
            cout << " Passwords don't match!" << endl;
            waitAndClear();
            return "";
        }

        if (saveCredentials(username, password))
        {
            cout << " Account created successfully!" << endl;
            waitAndClear();
            return username;
        }
        return "";
    }

    
    
    
    
};
