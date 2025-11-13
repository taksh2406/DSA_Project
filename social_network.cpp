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
  string login()
    {
        string username, password;

        cout << "\n===== LOGIN =====" << endl;
        cout << "Enter username: ";
        cin >> username;
        cout << "Enter password: ";
        cin >> password;
        clearScreen();

        if (verifyLogin(username, password))
        {
            cout << "✅ Login successful! Welcome back, @" << username << endl;
            waitAndClear();
            return username;
        }
        else
        {
            cout << "❌ Invalid username or password!" << endl;
            waitAndClear();
            return "";
        }
    }    
};

class SocialNetworkGraph
{
private:
  map <string, User> users; // User to User
  map <string, vector<string>> adjList;  //  User to friends list
  map <string, vector<string>> friendRequests;  // REceiever Name to Stack/Vector of Senders
  map <string, string> noticeBoard;  // A friend only noticeboard. 
  map <string, string> userBio;  // Public Bio for all

  sqlite3 *db = nullptr;

  bool initDatabase()
    {
        int rc = sqlite3_open("social_network.db", &db);
        if (rc)
        {
            cerr << "❌ Can't open database: " << sqlite3_errmsg(db) << endl;
            return false;
        }

        const char *createUsersTable =
            "CREATE TABLE IF NOT EXISTS Users ("
            "username TEXT PRIMARY KEY,"
            "name TEXT NOT NULL,"
            "dob TEXT NOT NULL,"
            "gender TEXT NOT NULL,"
            "bio TEXT);";

        char *errMsg = nullptr;
        rc = sqlite3_exec(db, createUsersTable, nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK)
        {
            cerr << "❌ SQL error: " << errMsg << endl;
            sqlite3_free(errMsg);
            return false;
        }

        const char *createConnectionsTable =
            "CREATE TABLE IF NOT EXISTS Connections ("
            "user1 TEXT NOT NULL,"
            "user2 TEXT NOT NULL,"
            "PRIMARY KEY (user1, user2),"
            "FOREIGN KEY (user1) REFERENCES Users(username) ON DELETE CASCADE,"
            "FOREIGN KEY (user2) REFERENCES Users(username) ON DELETE CASCADE);";

        rc = sqlite3_exec(db, createConnectionsTable, nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK)
        {
            cerr << "❌ SQL error: " << errMsg << endl;
            sqlite3_free(errMsg);
            return false;
        }

        const char *createRequestsTable =
            "CREATE TABLE IF NOT EXISTS FriendRequests ("
            "sender TEXT NOT NULL,"
            "receiver TEXT NOT NULL,"
            "PRIMARY KEY (sender, receiver),"
            "FOREIGN KEY (sender) REFERENCES Users(username) ON DELETE CASCADE,"
            "FOREIGN KEY (receiver) REFERENCES Users(username) ON DELETE CASCADE);";

        rc = sqlite3_exec(db, createRequestsTable, nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK)
        {
            cerr << "❌ SQL error (FriendRequests): " << errMsg << endl;
            sqlite3_free(errMsg);
            return false;
        }

        const char *createNoticeTable =
            "CREATE TABLE IF NOT EXISTS NoticeBoard ("
            "username TEXT PRIMARY KEY,"
            "notice TEXT,"
            "FOREIGN KEY(username) REFERENCES Users(username) ON DELETE CASCADE);";

        rc = sqlite3_exec(db, createNoticeTable, nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK)
        {
            cerr << "❌ SQL error (NoticeBoard): " << errMsg << endl;
            sqlite3_free(errMsg);
            return false;
        }

        return true;
    }

public:

    void displayAvatar()
    {
      // A gender neutral avatar of a person amde using AsCii letters
        cout << "+--------------------------+\n";
        cout << "|          ****            |\n";
        cout << "|        ********          |\n";
        cout << "|       **********         |\n";
        cout << "|      ************        |\n";
        cout << "|      ************        |\n";
        cout << "|       **********         |\n";
        cout << "|        ********          |\n";
        cout << "|          ****            |\n";
        cout << "|      **************      |\n";
        cout << "|    ******************    |\n";
        cout << "|   ********************   |\n";
        cout << "+--------------------------+\n";
    }
};




