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
        int rc = sqlite3_open("social_network.db", &db); // Open social_network.db if it exists. If it does not, then create it.
        if (rc) // Incase if database fails to open, this wil;l be printed
        {
            cerr << "❌ Can't open database: " << sqlite3_errmsg(db) << endl;
            return false;
        }

      // This will create a User Table
      // Username is the primary key
      // name, date of birth and gender are the required fields
      // Bio is optional to add. Add if you want, no pressure.
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

    bool loadFromDatabase()
    {
        const char *selectUsers = "SELECT username, name, dob, gender FROM Users;";
        sqlite3_stmt *stmt;

        int rc = sqlite3_prepare_v2(db, selectUsers, -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            cerr << "❌ Failed to fetch users: " << sqlite3_errmsg(db) << endl;
            return false;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            string username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            string dob = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            string gender = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            string bio = (sqlite3_column_text(stmt, 4) ? reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4)) : "");

            users[username] = User(name, username, dob, gender);
            noticeBoard[username];   // unchanged
            userBio[username] = bio; // <-- ADD THIS
            adjList[username] = vector<string>();
            friendRequests[username] = vector<string>();
        }
        sqlite3_finalize(stmt);

        const char *selectConnections = "SELECT user1, user2 FROM Connections;";
        rc = sqlite3_prepare_v2(db, selectConnections, -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            cerr << " Failed to fetch connections: " << sqlite3_errmsg(db) << endl;
            return false;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            string user1 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            string user2 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

            // Ensure entries exist
            if (adjList.find(user1) == adjList.end())
                adjList[user1] = vector<string>();
            if (adjList.find(user2) == adjList.end())
                adjList[user2] = vector<string>();

            adjList[user1].push_back(user2);
        }
        sqlite3_finalize(stmt);

        // Load friend requests
        const char *selectRequests = "SELECT sender, receiver FROM FriendRequests;";
        sqlite3_stmt *stmt2;
        rc = sqlite3_prepare_v2(db, selectRequests, -1, &stmt2, nullptr);
        if (rc == SQLITE_OK)
        {
            while (sqlite3_step(stmt2) == SQLITE_ROW)
            {
                string sender = reinterpret_cast<const char *>(sqlite3_column_text(stmt2, 0));
                string receiver = reinterpret_cast<const char *>(sqlite3_column_text(stmt2, 1));
                // push_back keeps insertion order from DB; treat vector as stack (LIFO) later
                friendRequests[receiver].push_back(sender);
            }
            sqlite3_finalize(stmt2);
        }
        else
        {
            // If table exists but prepare failed, report
            // not fatal
            sqlite3_finalize(stmt2);
        }

        // Load notices
        const char *selectNotices = "SELECT username, notice FROM NoticeBoard;";
        sqlite3_stmt *stmt3;
        rc = sqlite3_prepare_v2(db, selectNotices, -1, &stmt3, nullptr);

        if (rc == SQLITE_OK)
        {
            while (sqlite3_step(stmt3) == SQLITE_ROW)
            {
                string username = reinterpret_cast<const char *>(sqlite3_column_text(stmt3, 0));
                string note = reinterpret_cast<const char *>(sqlite3_column_text(stmt3, 1));
                noticeBoard[username] = note;
            }
        }
        sqlite3_finalize(stmt3);

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




