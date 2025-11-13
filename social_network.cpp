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

    SocialNetworkGraph()
    {
        if (initDatabase())
        {
            loadFromDatabase();
        }
    }

    ~SocialNetworkGraph()
    {
        if (db)
        {
            sqlite3_close(db);
        }
    }

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

    bool sendFriendRequest(const string &from, const string &to)
    {
        if (users.find(from) == users.end() || users.find(to) == users.end())
        {
            cout << "❌ One or both users not found!" << endl;
            waitAndClear();
            return false;
        }
        if (from == to)
        {
            cout << "❌ You cannot send a friend request to yourself!" << endl;
            waitAndClear();
            return false;
        }
        // Already friends?
        if (find(adjList[from].begin(), adjList[from].end(), to) != adjList[from].end())
        {
            cout << "✅ You are already friends!" << endl;
            waitAndClear();
            return false;
        }
        // Request already sent?
        auto &requestsForTo = friendRequests[to];
        if (find(requestsForTo.begin(), requestsForTo.end(), from) != requestsForTo.end())
        {
            cout << "⏳ Friend request already sent!" << endl;
            waitAndClear();
            return false;
        }
        // If 'to' already sent a request to 'from', consider informing user that there's a pending incoming request;
        // We keep separate: the receiver will see and accept/reject when they check.

        // Add to DB
        sqlite3_stmt *stmt;
        const char *insertSQL = "INSERT OR IGNORE INTO FriendRequests (sender, receiver) VALUES (?, ?);";
        sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, from.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, to.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        // Add to in-memory stack (vector push_back -> we'll treat as stack LIFO)
        friendRequests[to].push_back(from);

        cout << "📨 Friend request sent to @" << to << "!" << endl;
        waitAndClear();
        return true;
    }

    void viewAndHandleRequests(const string &username)
    {
        clearScreen();
        if (users.find(username) == users.end())
        {
            cout << " User not found!" << endl;
            waitAndClear();
            return;
        }

        vector<string> &requests = friendRequests[username];
        if (requests.empty())
        {
            cout << "📭 No pending friend requests." << endl;
            waitAndClear();
            return;
        }

        cout << "\n===== PENDING FRIEND REQUESTS (newest first) =====" << endl;
        // Process as stack (LIFO): pop from back
        while (!requests.empty())
        {
            string sender = requests.back();
            // show one at a time
            cout << "\nFriend request from @" << sender << endl;
            cout << "Accept (A) / Reject (R) / Skip (S)? ";
            char choice;
            cin >> choice;
            choice = static_cast<char>(toupper(choice));
            if (choice == 'A')
            {
                acceptFriendRequest(sender, username);
            }
            else if (choice == 'R')
            {
                rejectFriendRequest(sender, username);
            }
            else if (choice == 'S')
            {
                // Move this request to front so it is treated as older (we'll pop then push_front simulation):
                requests.pop_back();
                // push at beginning (older)
                requests.insert(requests.begin(), sender);
                cout << "⏭ Skipped. Will appear later." << endl;
                waitAndClear();
            }
            else
            {
                cout << "❌ Invalid choice. Skipping this request." << endl;
                // pop anyway to avoid infinite loop; user can re-send if needed
                requests.pop_back();
                waitAndClear();
            }
        }
        // After processing, refresh
        clearScreen();
        cout << "✅ Done processing friend requests." << endl;
        waitAndClear();
    }

    void acceptFriendRequest(const string &from, const string &to)
    {
        // Remove from DB
        sqlite3_stmt *stmt;
        const char *deleteSQL = "DELETE FROM FriendRequests WHERE sender=? AND receiver=?;";
        sqlite3_prepare_v2(db, deleteSQL, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, from.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, to.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        // Remove from in-memory requests (pop back expected, but ensure removal)
        auto &requests = friendRequests[to];
        auto it = find(requests.begin(), requests.end(), from);
        if (it != requests.end())
            requests.erase(it);

        // Create mutual connection
        addConnection(from, to); // addConnection handles DB and in-memory

        // Inform user (addConnection already prints a message)
        // but also show accept confirmation
        cout << "✅ Friend request from @" << from << " accepted." << endl;
        waitAndClear();
    }

    void rejectFriendRequest(const string &from, const string &to)
    {
        // Remove from DB
        sqlite3_stmt *stmt;
        const char *deleteSQL = "DELETE FROM FriendRequests WHERE sender=? AND receiver=?;";
        sqlite3_prepare_v2(db, deleteSQL, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, from.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, to.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        // Remove from in-memory requests
        auto &requests = friendRequests[to];
        auto it = find(requests.begin(), requests.end(), from);
        if (it != requests.end())
            requests.erase(it);

        cout << "❌ Friend request from @" << from << " rejected." << endl;
        waitAndClear();
    }

    void searchUsers(const string &viewer)
    {
        clearScreen();
        string searchTerm;
        cout << "\nEnter username to search: ";
        cin >> searchTerm;
        clearScreen();

        User *user = findUser(searchTerm);
        if (!user)
        {
            cout << "❌ User @" << searchTerm << " does not exist!" << endl;
            sleep(2);
            return;
        }

        cout << "\n===== USER PROFILE =====" << endl;
        displayAvatar();

        // Always public info
        cout << "Name: " << user->name << endl;
        cout << "Username: @" << user->username << endl;

        // Friendship check
        bool isFriend = false;
        if (adjList.find(viewer) != adjList.end())
        {
            auto &friends = adjList[viewer];
            if (find(friends.begin(), friends.end(), searchTerm) != friends.end())
            {
                isFriend = true;
            }
        }

        // Private info only visible to friends
        if (isFriend)
        {
            cout << "DOB: " << user->dob << endl;
            cout << "Gender: " << user->gender << endl;
        }

        // Bio is public
        cout << "\nBio: " << (userBio[searchTerm].empty() ? "(No bio added)" : userBio[searchTerm]) << endl;

        sleep(3);
    }

    bool addUser(string name, string username, string dob, string gender)
    {
        if (users.find(username) != users.end())
        {
            cout << " User @" << username << " already exists!" << endl;
            waitAndClear();
            return false;
        }

        sqlite3_stmt *stmt;
        const char *insertSQL = "INSERT INTO Users (username, name, dob, gender) VALUES (?, ?, ?, ?);";

        int rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            cerr << "❌ Failed to prepare statement: " << sqlite3_errmsg(db) << endl;
            return false;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, dob.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, gender.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE)
        {
            cerr << " Failed to insert user: " << sqlite3_errmsg(db) << endl;
            return false;
        }

        users[username] = User(name, username, dob, gender);
        adjList[username] = vector<string>();
        friendRequests[username] = vector<string>();
        userBio[username] = "";

        cout << " User @" << username << " added successfully!" << endl;
        waitAndClear();
        return true;
    }

    void displayMyProfile(string username)
    {
        // clearScreen();
        User *user = findUser(username);
        if (user)
        {
            cout << "\n===== MY PROFILE =====\n";
            displayAvatar();
            user->display();
        }
    }
    
    User *findUser(string username)
    {
        if (users.find(username) != users.end())
        {
            return &users[username];
        }
        return nullptr;
    }

    bool deleteUser(string username)
    {
        if (users.find(username) == users.end())
        {
            cout << " User @" << username << " does not exist!" << endl;
            return false;
        }

        sqlite3_stmt *stmt;
        const char *deleteSQL = "DELETE FROM Users WHERE username = ?;";

        int rc = sqlite3_prepare_v2(db, deleteSQL, -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            cerr << " Failed to prepare delete statement: " << sqlite3_errmsg(db) << endl;
            return false;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE)
        {
            cerr << "❌ Failed to delete user: " << sqlite3_errmsg(db) << endl;
            return false;
        }

        // Remove any connections referencing this user from DB
        const char *deleteConn1 = "DELETE FROM Connections WHERE user1 = ? OR user2 = ?;";
        rc = sqlite3_prepare_v2(db, deleteConn1, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        // Remove pending friend requests referencing this user from DB
        const char *deleteReqs = "DELETE FROM FriendRequests WHERE sender = ? OR receiver = ?;";
        rc = sqlite3_prepare_v2(db, deleteReqs, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        // Remove from in-memory adjacency lists
        for (string friendUsername : adjList[username])
        {
            vector<string> &friendsList = adjList[friendUsername];
            friendsList.erase(remove(friendsList.begin(), friendsList.end(), username), friendsList.end());
        }

        // Remove from all adjacency lists
        for (auto &pair : adjList)
        {
            vector<string> &connections = pair.second;
            connections.erase(remove(connections.begin(), connections.end(), username), connections.end());
        }

        // Remove pending requests in memory
        for (auto &pair : friendRequests)
        {
            vector<string> &reqs = pair.second;
            reqs.erase(remove(reqs.begin(), reqs.end(), username), reqs.end());
        }

        adjList.erase(username);
        friendRequests.erase(username);
        users.erase(username);

        cout << " User @" << username << " deleted successfully!" << endl;
        waitAndClear();
        return true;
    }

    bool addConnection(string username1, string username2)
    {
        // mutual connection (adds both directions)
        if (users.find(username1) == users.end() || users.find(username2) == users.end())
        {
            cout << "❌ One or both users not found!" << endl;
            waitAndClear();
            return false;
        }

        if (username1 == username2)
        {
            cout << "❌ Cannot add connection to self!" << endl;
            waitAndClear();
            return false;
        }

        vector<string> &friends1 = adjList[username1];
        if (find(friends1.begin(), friends1.end(), username2) != friends1.end())
        {
            cout << "❌ Connection already exists!" << endl;
            waitAndClear();
            return false;
        }

        sqlite3_stmt *stmt;
        const char *insertSQL = "INSERT OR IGNORE INTO Connections (user1, user2) VALUES (?, ?);";

        // insert username1 -> username2
        sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, username1.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, username2.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        // insert username2 -> username1
        sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, username2.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, username1.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        adjList[username1].push_back(username2);
        adjList[username2].push_back(username1);

        cout << " Connection added between @" << username1 << " and @" << username2 << endl;
        waitAndClear();
        return true;
    }

    vector<string> findMutualConnections(string username, string usernameX)
    {
        vector<string> mutualFriends;

      // Scour through the kist to find mutual friends
        if (users.find(username) == users.end() || users.find(usernameX) == users.end())
        {
            cout << " One or both users not found!" << endl;
            return mutualFriends;
        }

        set<string> friendsOfUser(adjList[username].begin(), adjList[username].end());
        set<string> friendsOfX(adjList[usernameX].begin(), adjList[usernameX].end());

        set_intersection(friendsOfUser.begin(), friendsOfUser.end(),
                         friendsOfX.begin(), friendsOfX.end(),
                         back_inserter(mutualFriends));

        return mutualFriends;
    }

    void displayMyConnections(string username)
    {
        clearScreen();
        if (users.find(username) == users.end())
        {
            cout << " User not found!" << endl;
            return;
        }

        cout << "\n===== MY CONNECTIONS =====" << endl;
        if (adjList[username].empty())
        {
            cout << "No connections yet." << endl;
        }
        else
        {
            for (size_t i = 0; i < adjList[username].size(); i++)
            {
                string friendUsername = adjList[username][i];
                cout << (i + 1) << ". @" << friendUsername << " - " << users[friendUsername].name << endl;
            }
        }
    }

    void viewMyNotice(const string &username)
    {
        clearScreen();
        cout << "===== MY NOTICE =====\n";

        if (noticeBoard.find(username) == noticeBoard.end())
        {
            cout << "(No notice written yet)\n";
        }
        else
        {
            cout << noticeBoard[username] << endl;
        }

        sleep(2);
    }

    void editMyNotice(const string &username)
    {
      // This will edit the noticeboard that is only visible to friend and the user itself, acting as a status or means of commmunication among the friends
        clearScreen();
        cout << "===== EDIT NOTICE =====\n";
        cout << "Type your notice below.\n";
        cout << "(Use single-line input)\n\n";

        cin.ignore();
        string newNotice;
        getline(cin, newNotice);

        // Save to DB
        sqlite3_stmt *stmt;
        const char *insertSQL =
            "INSERT INTO NoticeBoard (username, notice) VALUES (?, ?) "
            "ON CONFLICT(username) DO UPDATE SET notice = excluded.notice;";

        sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, newNotice.c_str(), -1, SQLITE_TRANSIENT);

        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        noticeBoard[username] = newNotice;

        cout << "✅ Notice saved!" << endl;
        sleep(2);
    }

    void viewFriendNotice(const string &viewer)
    {
        clearScreen();
        cout << "Enter friend's username: ";
        string friendUser;
        cin >> friendUser;

        // Check existence
        if (users.find(friendUser) == users.end())
        {
            cout << "❌ User does not exist!" << endl;
            sleep(2);
            return;
        }

        // Check friendship
        auto &friends = adjList[viewer];
        if (find(friends.begin(), friends.end(), friendUser) == friends.end())
        {
            cout << "❌ You are not friends. Cannot view their notice." << endl;
            sleep(2);
            return;
        }

        cout << "\n===== @" << friendUser << "'s NOTICE =====\n";
        if (noticeBoard.find(friendUser) == noticeBoard.end())
        {
            cout << "(They have not written a notice yet)\n";
        }
        else
        {
            cout << noticeBoard[friendUser] << endl;
        }
        sleep(3);
    }
};




