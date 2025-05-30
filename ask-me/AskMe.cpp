#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>
#include <iomanip>
#include <cassert>
#include <filesystem>

using namespace std;

class User;
class Question;
class FileManager;
class UserManager;

// Represents a user account in the system with authentication capabilities
// Handles user data, permissions, and serialization to strings
class User {
public:
    // Different privilege levels for users
    enum Role {
        ADMIN,          // Full system access
        REGULAR_USER    // Standard user privileges
    };

private:
    // User data members
    int id;                     // Unique numeric identifier
    string name;                // Full display name
    string password;            // Authentication secret (stored in plaintext)
    string username;            // Login identifier
    string email;               // Contact address
    bool allow_anonymous_questions;  // Permission flag for anonymous questions
    Role role;                  // Access level

public:
    // Creates a default invalid user (ID = 0, regular privileges)
    User() : id(0), allow_anonymous_questions(false), role(REGULAR_USER) {}
    
    // Creates a fully specified user account
    // Note: Password stored in plaintext - So I'll consider hashing in real applications
    User(int id, const string& name, const string& password, const string& username,
        const string& email, bool allow_anonymous_questions, Role role = REGULAR_USER)
        : id(id), name(name), password(password), username(username), email(email),
        allow_anonymous_questions(allow_anonymous_questions), role(role) {}


    // Returns the unique user ID
    int getId() const { return id; }

    // Gets the user's full display name
    const string& getName() const { return name; }

    // Gets the username used for login
    const string& getUsername() const { return username; }

    // Gets the user's email address
    const string& getEmail() const { return email; }

    // Checks if user allows anonymous questions
    bool getAllowAnonymousQuestions() const { return allow_anonymous_questions; }

    // Gets the user's privilege level
    Role getRole() const { return role; }

    // Changes whether user accepts anonymous questions
    void setAllowAnonymousQuestions(bool allow) { allow_anonymous_questions = allow; }

    // Verifies if a password matches the user's stored password
    // inputPassword: The plaintext password to check
    // Returns: True if passwords match (case-sensitive comparison)
    bool verifyPassword(const string& inputPassword) const {
        return password == inputPassword;
    }
    
    // Converts user data to a CSV-formatted string
    // Format: "id,name,password,username,email,anonymous_flag"
    string toString() const {
        return to_string(id) + "," + name + "," + password + "," + username + ","
        + email + "," + (allow_anonymous_questions ? "1" : "0") + "," + to_string(role);
    }
};

// Represents a question/answer pair between users
// Supports conversation threads and anonymous questions
class Question {
private:
    // Question data members
    int id;             // Unique question identifier
    int parent_id;      // -1 for main questions otherwise this is a thread question
    int from_user_id;   // Asker's user ID
    int to_user_id;     // Recipient's user ID
    bool is_anonymous;  // Hide asker's identity if true
    string text;        // Question content
    string answer;      // Provided answer (empty if unanswered)

public:
    // Creates a default invalid question (ID = 0, parent_id = -1)
    Question() : id(0), parent_id(-1), from_user_id(0), 
                to_user_id(0), is_anonymous(false) {}
    
    // Creates a complete question entry
    // parent_id: -1 starts new thread, existing ID adds to thread
    // is_anonymous: Hides the sender's identity when true
    // answer: Can pre-populate an answer (default empty)
    Question(int id, int parent_id, int from_user_id, int to_user_id,
            bool is_anonymous, const string& text, const string& answer = "")
            : id(id), parent_id(parent_id), from_user_id(from_user_id),
            to_user_id(to_user_id), is_anonymous(is_anonymous),
            text(text), answer(answer) {}

    // Returns the question's unique ID
    int getId() const { return id; }

    // Gets the conversation thread ID (-1 for parent questions)
    int getParentId() const { return parent_id; }

    // Gets the user ID of the person asking
    int getFromUserId() const { return from_user_id; }

    // Gets the user ID of the recipient
    int getToUserId() const { return to_user_id; }

    // Checks if question was asked anonymously
    bool getIsAnonymous() const { return is_anonymous; }

    // Gets the question text content
    const string& getText() const { return text; }

    // Gets the answer text (empty string if unanswered)
    const string& getAnswer() const { return answer; }
    
    // Sets or updates the answer to this question
    void setAnswer(const string& newAnswer) { answer = newAnswer; }
    
    // Checks if this question has been answered
    bool isAnswered() const { return !answer.empty(); }
    
    // Serializes question to CSV string format
    // Format: "id,parent_id,from,to,anonymous,text,answer"
    // Note: Automatically escapes commas in text/answer fields
    string toString() const {
        return to_string(id) + "," + to_string(parent_id) + "," +
                to_string(from_user_id) + "," + to_string(to_user_id) + "," +
                (is_anonymous ? "1" : "0") + "," + escapeCommas(text) + "," +
                escapeCommas(answer);
    }

private:
    // Helper method to make strings safe for CSV format
    // Wraps text in quotes if it contains commas
    // Doubles up existing quotes in the text
    string escapeCommas(const string& str) const {
        if (str.find(',') != string::npos) {
            string escaped = str;
            size_t pos = 0;
            while ((pos = escaped.find('"', pos)) != string::npos) {
                escaped.replace(pos, 1, "\"\"");
                pos += 2;
            }
            return "\"" + escaped + "\"";
        }
        return str;
    }
};

// Handles all file operations for user and question data
// Manages reading, writing, and parsing of data files
class FileManager {
private:
    const string users_file_path;     // Path to users data file
    const string questions_file_path; // Path to questions data file

    // Splits a CSV line into tokens, handling quoted fields
    // str: The input string to split
    // delimiter: Character that separates fields (typically comma)
    // Returns: Vector of string tokens with quotes removed
    static vector<string> split(const string& str, char delimiter) {
        vector<string> tokens;
        string currentToken;
        bool inQuotes = false;
        
        for (auto& c : str) {
            if (c == '"') {
                inQuotes = !inQuotes;
            } else if (c == delimiter && !inQuotes) {
                tokens.push_back(currentToken);
                currentToken.clear();
            } else {
                currentToken += c;
            }
        }
        
        tokens.push_back(currentToken);
        return tokens;
    }

public:
    // Initializes file paths with default locations
    FileManager(const string& users_path = "users.txt", const string& questions_path = "questions.txt")
        : users_file_path(users_path), questions_file_path(questions_path) {}

    // Reads all lines from a text file
    // file_path: The file to read from
    // Returns: Vector of non-empty lines, or empty vector on error
    vector<string> ReadInformationFromFile(const string& file_path) const {
        ifstream file(file_path);
        if (!file.is_open()) {
            cerr << "Unable to open file: " << file_path << "\n";
            return {};
        }

        vector<string> lines;
        string line;
        while (getline(file, line)) {
            if (!line.empty()) {
                lines.push_back(line);
            }
        }
        file.close();
        return lines;
    }

    // Writes multiple lines to a file (overwrites existing content)
    // file_path: Destination file path
    // lines: Content to write
    // Returns: True if successful, false on error
    bool StoreInformationOnFile(const string& file_path, const vector<string>& lines) const {
        ofstream file(file_path);
        if (!file.is_open()) {
            cerr << "Unable to open file for writing: " << file_path << "\n";
            return false;
        }
        
        for (const auto& line : lines) {
            file << line << "\n";
        }
        file.close();
        return true;
    }
    
    // Loads all users from the users file
    // Returns: Map of user_id to User objects
    // Note: Skips malformed lines but logs errors
    unordered_map<int, User> LoadUsers() const {
        unordered_map<int, User> users;
        vector<string> lines = ReadInformationFromFile(users_file_path);
        
        for (const auto& line : lines) {
            vector<string> tokens = split(line, ',');
            if (tokens.size() < 7) {
                cerr << "Skipping malformed user line (not enough tokens): " << line << "\n";
                continue;
            }
            
            try {
                int id = stoi(tokens[0]);
                string name = tokens[1];
                string password = tokens[2];
                string username = tokens[3];
                string email = tokens[4];
                bool allowAnon = (tokens[5] == "1" || tokens[5] == "true");
                User::Role role = (tokens[6] == "0" ? User::ADMIN : User::REGULAR_USER);
                
                users.emplace(id, User(id, name, password, username, email, allowAnon, role));
            } catch (const exception& e) {
                cerr << "Error parsing user line: " << line << "\n" << "Exception: " << e.what() << "\n";
                continue;
            }
        }
        return users;
    }
    
    // Loads all questions from the questions file
    // Returns: Map of question_id to Question objects
    // Note: Handles both parent and thread questions
    unordered_map<int, Question> LoadQuestions() const {
        unordered_map<int, Question> questions;
        vector<string> lines = ReadInformationFromFile(questions_file_path);
        
        for (const auto& line : lines) {
            vector<string> tokens = split(line, ',');
            if (tokens.size() < 6) {
                cerr << "Skipping malformed question line: " << line << "\n";
                continue;
            }
            
            try {
                int id = stoi(tokens[0]);
                int parentId = stoi(tokens[1]);
                int fromUserId = stoi(tokens[2]);
                int toUserId = stoi(tokens[3]);
                bool isAnon = (tokens[4] == "1" || tokens[4] == "true");
                string text = tokens[5];
                string answer = tokens.size() >= 7 ? tokens[6] : "";
                
                questions.emplace(id, Question(id, parentId, fromUserId, toUserId, isAnon, text, answer));
            } catch (const exception& e) {
                cerr << "Error parsing question line: " << line << "\n" << "Exception: " << e.what() << "\n";
                continue;
            }
        }
        return questions;
    }
    
    // Saves all users to the users file
    // users: Map of users to save
    // Returns: True if successful, false on error
    bool SaveUsers(const unordered_map<int, User>& users) const {
        vector<string> lines;
        for (const auto& pair : users) {
            lines.push_back(pair.second.toString());
        }
        return StoreInformationOnFile(users_file_path, lines);
    }
    
    // Saves all questions to the questions file
    // questions: Map of questions to save
    // Returns: True if successful, false on error
    bool SaveQuestions(const unordered_map<int, Question>& questions) const {
        vector<string> lines;
        for (const auto& pair : questions) {
            lines.push_back(pair.second.toString());
        }
        return StoreInformationOnFile(questions_file_path, lines);
    }
};

// Manages all user-related operations including CRUD operations and authentication
// Handles persistence through FileManager and maintains in-memory user data
class UserManager {
private:
    unordered_map<int, User> users;  // Maps user IDs to User objects
    FileManager file_manager;        // Handles file system operations

public:
    // Initializes UserManager by loading users from persistent storage
    UserManager() : file_manager("users.txt", "questions.txt") {
        users = file_manager.LoadUsers();
    }
    UserManager(const FileManager& fm) : file_manager(fm) {
        users = file_manager.LoadUsers();
    }

    // Displays all users in the system with their basic information
    // Format: ID [tab] Name [tab] Role (Admin/User)
    void ListSystemUsers() const {
        if (users.empty()) {
            cout << "No users found\n";
            return;
        }
        
        for (const auto& [id, user] : users) {
            cout << "ID: " << id << "\tName: " << user.getName() << "\tRole: "
            << (user.getRole() == User::ADMIN ? "Admin" : "User") << '\n';
        }
    }

    // Updates an existing user's information
    // updated_user: User object containing new data (must have existing ID)
    // Returns: true if update successful, false if user doesn't exist
    // Note: Changes are automatically persisted to file
    bool UpdateUser(const User& updated_user) {
        if (!users.count(updated_user.getId())) {
            cerr << "User not found\n";
            return false;
        }
        
        users[updated_user.getId()] = updated_user;
        return file_manager.SaveUsers(users);
    }

    // Retrieves a user by their unique ID
    // user_id: The ID to search for
    // Returns: Const reference to the User object
    // Throws: runtime_error if user not found
    const User& GetUserByID(int user_id) const {
        auto it = users.find(user_id);
        if (it == users.end()) {
            throw runtime_error("User not found");
        }
        return it->second;
    }

    // Adds a new user to the system
    // user: The User object to add
    // Returns: True if added successfully, false if user ID already exists
    // Note: Automatically persists changes to file
    bool AddUser(const User& user) {
        if (!users.emplace(user.getId(), user).second) {
            cerr << "User ID already exists\n";
            return false;
        }
        return file_manager.SaveUsers(users);
    }

    // Generates the next available user ID
    // Returns: Highest existing ID + 1, or 1 if no users exist
    int GetNextUserID() const {
        if (users.empty()) return 1;
        int maxId = 0;
        for (const auto& pair : users) {
            maxId = max(maxId, pair.first);
        }
        return maxId + 1;
    }

    // Removes a user from the system
    // user_id: ID of the user to delete
    // Returns: True if user existed and was deleted, false otherwise
    // Note: Automatically persists changes to file
    bool DeleteUser(int user_id) {
        if (users.erase(user_id)) {
            cout << "[Success] Deleted user ID: " << user_id << "\n";
            return file_manager.SaveUsers(users);
        }
        return false;
    }

    // Verifies user credentials
    // user_id: ID of the user to authenticate
    // password: Plaintext password to verify
    // Returns: True if credentials are valid, false otherwise
    bool Authenticate(int user_id, const string& password) const {
        auto it = users.find(user_id);
        return it != users.end() && it->second.verifyPassword(password);
    }
};


// Handles user authentication and registration
// Manages login state and integrates with UserManager for user operations
class AuthService {
private:
    unordered_map<int, User> users; // Local cache of users
    UserManager& user_manager;       // Handles user operations
    FileManager file_manager;       // Handles file operations
    User current_user;              // Currently logged-in user
    bool is_logged_in = false;      // Track login state

public:
    // Initializes service and loads users from file
    AuthService(UserManager& um) : user_manager(um) {
        users = file_manager.LoadUsers();
    }
    
    // Handles user login process
    // Throws: runtime_error if authentication fails
    void Login() {
        cout << "Enter your user ID and password:\n";
        cout << "User ID: ";
        int user_id;
        cin >> user_id;
        
        cout << "Password: ";
        string password;
        cin >> password;
        
        if (user_manager.Authenticate(user_id, password)) {
            current_user = user_manager.GetUserByID(user_id);
            is_logged_in = true;
            cout << "\nLogin successful! Welcome, " << current_user.getName() << ".\n";
        } else {
            throw runtime_error("Invalid information - Please check your ID and password and try again...\n");
        }
    }

    // Handles new user registration
    // Throws: runtime_error if registration fails
    void SignUp() {
        cout << "\nNew User Registration\n";
        cout << "----------------------\n";
        
        string username, password, name, email;
        int allow_anon;

        cout << "Username: ";
        cin >> username;

        cout << "Password: ";
        cin >> password;

        cout << "Full Name: ";
        cin.ignore();
        getline(cin, name);

        cout << "Email: ";
        cin >> email;
        
        cout << "Allow anonymous questions? (1 = Yes, 0 = No): ";
        cin >> allow_anon;

        User new_user(
            user_manager.GetNextUserID(),
            name,
            password,
            username,
            email,
            allow_anon == 1
        );

        if (user_manager.AddUser(new_user)) {
            current_user = new_user;
            is_logged_in = true;
            cout << "\nRegistration successful! Welcome, " << name << ".\n";
            cout << "Your user ID is: " << new_user.getId() << " (Please remember this!)\n";
        } else {
            throw runtime_error("Registration failed - Username may already exist");
        }
    }

    // Gets the currently logged-in user
    // Returns: User object
    // Throws: runtime_error if no user is logged in
    User GetCurrentUser() {
        if (!is_logged_in) {
            throw runtime_error("No user currently logged in");
        }
        return current_user;
    }

    // Checks if a user is currently logged in
    bool IsLoggedIn() const {
        return is_logged_in;
    }

    // Logs out the current user
    void Logout() {
        current_user = User(); // Reset to empty user
        is_logged_in = false;
        cout << "Successfully logged out.\n";
    }
};


// Manages all question-related operations including creation, answering, and display
// Handles question threading and persistence through FileManager
class QuestionManager {
private:
    unordered_map<int, Question> questions; // Maps question IDs to Question objects
    UserManager& user_manager;              // Reference to user manager
    FileManager file_manager;               // Handles file system operations

    // Helper method to print question details
    void PrintQuestion(const Question& q, bool is_thread = false) const {
        if (is_thread) {
            cout << "├─ Thread ";
        }
        
        cout << "Question ID: " << q.getId() << "\n";
        
        if (!is_thread) {
            cout << "To: User ID " << q.getToUserId() << "\n";
        }
        
        if (!q.getIsAnonymous() || !is_thread) {
            cout << "From: " << (q.getIsAnonymous() ? "Anonymous" : "User ID " + to_string(q.getFromUserId())) << "\n";
        }
        
        cout << "Question: " << q.getText() << "\n";
        cout << "Answer: " << (q.isAnswered() ? q.getAnswer() : "Not answered yet") << "\n";
        cout << (is_thread ? "  " : "") << "───\n";
    }

public:
    QuestionManager(UserManager& um) : user_manager(um) {
        questions = file_manager.LoadQuestions();
    }

    // Gets the next available question ID
    int GetNextQuestionID() const {
        if (questions.empty()) return 1;
        return max_element(questions.begin(), questions.end(),
            [](const auto& a, const auto& b) {
                return a.first < b.first;
            })->first + 1;
    }

    // Adds a new question to the system
    bool AddQuestion(const Question& question) {
        if (questions.count(question.getId())) {
            cerr << "Error: Question ID " << question.getId() << " already exists\n";
            return false;
        }
        
        try {
            const User& recipient = user_manager.GetUserByID(question.getToUserId());
            if (question.getIsAnonymous() && !recipient.getAllowAnonymousQuestions()) {
                cerr << "Error: User " << recipient.getName() << " doesn't accept anonymous questions\n";
                return false;
            }
        } catch (const runtime_error& e) {
            cerr << "Error: Recipient user not found\n";
            return false;
        }
        
        questions[question.getId()] = question;
        return file_manager.SaveQuestions(questions);
    }
    
    // Updates an existing question
    bool UpdateQuestion(const Question& question) {
        if (!questions.count(question.getId())) {
            cerr << "Error: Question ID " << question.getId() << " not found\n";
            return false;
        }
        
        questions[question.getId()] = question;
        return file_manager.SaveQuestions(questions);
    }

    // Interactive question asking flow
    void AskQuestion(const User& current_user) {
        cout << "\n─── Ask a Question ───\n";
        
        cout << "Enter recipient user ID (-1 to cancel): ";
        int to_user_id;
        cin >> to_user_id;
        
        if (to_user_id == -1) {
            cout << "Question canceled.\n";
            return;
        }

        try {
            const User& recipient = user_manager.GetUserByID(to_user_id);
            
            cout << "Is this a follow-up question? (y/n): ";
            char response;
            cin >> response;
            
            int parent_id = -1;
            if (tolower(response) == 'y') {
                cout << "Enter parent question ID: ";
                if (!(cin >> parent_id) || !questions.count(parent_id)) {
                    cerr << "Invalid parent question ID. Starting new question thread.\n";
                    parent_id = -1;
                }
            }

            cout << "Enter your question (press Enter when done):\n> ";
            string text;
            cin.ignore();
            getline(cin, text);

            bool is_anonymous = false;
            if (recipient.getAllowAnonymousQuestions()) {
                cout << "Ask anonymously? (y/n): ";
                cin >> response;
                is_anonymous = (tolower(response) == 'y');
            }

            Question question(
                GetNextQuestionID(),
                parent_id,
                current_user.getId(),
                to_user_id,
                is_anonymous,
                text
            );

            if (AddQuestion(question)) {
                cout << "\n✓ Question submitted successfully!\n";
                cout << "  Question ID: " << question.getId() << "\n";
                if (parent_id != -1) {
                    cout << "  Thread to question ID: " << parent_id << "\n";
                }
            }
        } catch (const runtime_error& e) {
            cerr << "Error: " << e.what() << "\n";
        }
    }

    // Handles answering questions
    void AnswerQuestion(int current_user_id) {
        cout << "\n─── Answer a Question ───\n";
        cout << "Enter question ID (-1 to cancel): ";
        int question_id;
        cin >> question_id;
        
        if (question_id == -1) {
            cout << "Answer canceled.\n";
            return;
        }

        auto it = questions.find(question_id);
        if (it == questions.end()) {
            cerr << "Error: Question ID " << question_id << " doesn't exist\n";
            return;
        }

        Question& question = it->second;
        if (question.getToUserId() != current_user_id) {
            cerr << "Error: You can only answer questions addressed to you\n";
            return;
        }

        cout << "\nQuestion Details:\n";
        cout << "From: " << (question.getIsAnonymous() ? "Anonymous" :
                "User ID " + to_string(question.getFromUserId())) << "\n";
        cout << "Question: " << question.getText() << "\n";
        
        if (question.isAnswered()) {
            cout << "⚠️ Existing answer: " << question.getAnswer() << "\n";
            cout << "This will update the existing answer. Continue? (y/n): ";
            char response;
            cin >> response;
            if (tolower(response) != 'y') {
                cout << "Answer update canceled.\n";
                return;
            }
        }

        cout << "Enter your answer (press Enter when done):\n> ";
        string answer;
        cin.ignore();
        getline(cin, answer);

        question.setAnswer(answer);
        if (UpdateQuestion(question)) {
            cout << "\n✓ Answer submitted successfully!\n";
        }
    }

    // Prints questions addressed to a specific user
    void PrintQuestionsToUser(int user_id) const {
        cout << "\n─── Questions To You ───\n";
        bool found = false;
        
        for (const auto& [id, question] : questions) {
            if (question.getToUserId() == user_id) {
                found = true;
                PrintQuestion(question, question.getParentId() != -1);
            }
        }
        
        if (!found) {
            cout << "No questions found addressed to you.\n";
        }
    }

    // Prints questions asked by a specific user
    void PrintQuestionsFromUser(int user_id) const {
        cout << "\n─── Questions From You ───\n";
        bool found = false;
        
        for (const auto& [id, question] : questions) {
            if (question.getFromUserId() == user_id) {
                found = true;
                PrintQuestion(question, question.getParentId() != -1);
            }
        }
        
        if (!found) {
            cout << "You haven't asked any questions yet.\n";
        }
    }

    // Displays all thread questions for a parent question
    void GetThreadQuestions() {
        cout << "\n─── View Thread Questions ───\n";
        cout << "Enter parent question ID (-1 to cancel): ";
        int parent_id;
        cin >> parent_id;

        if (parent_id == -1) {
            cout << "Operation canceled.\n";
            return;
        }

        if (!questions.count(parent_id)) {
            cerr << "Error: Parent question ID " << parent_id << " doesn't exist\n";
            return;
        }

        bool found = false;
        cout << "\nThreads for question ID " << parent_id << ":\n";
        for (const auto& [id, question] : questions) {
            if (question.getParentId() == parent_id) {
                found = true;
                PrintQuestion(question, true); // true indicates it's a thread
            }
        }

        if (!found) {
            cout << "No thread questions found for this parent question.\n";
        }
    }

    // Prints all questions in the system (admin only)
    void GetFeed(const User& current_user) {
        cout << "\n─── System Questions Feed ───\n";

        if (current_user.getRole() != User::ADMIN) {
            cerr << "⛔ Access denied: This feature is only available for administrators.\n";
            return;
        }

        if (questions.empty()) {
            cout << "No questions in the system yet.\n";
            return;
        }

        cout << "Total questions: " << questions.size() << "\n\n";
        for (const auto& [id, question] : questions) {
            PrintQuestion(question, question.getParentId() != -1);
        }
    }

    // Deletes a question and all its threads
    bool DeleteQuestion(int question_id, const User& current_user) {
        if (!questions.count(question_id)) {
            cerr << "[Error] Question ID " << question_id << " doesn't exist.\n";
            return false;
        }

        const Question& question = questions[question_id];

        if (question.getFromUserId() != current_user.getId() && current_user.getRole() == User::REGULAR_USER) {
            cout << "[Access Denied] You can only delete questions you asked.\n";
            return false;
        }

        // Delete thread questions first
        DeleteThreadQuestions(question_id, current_user);

        // Delete the main question
        questions.erase(question_id);
        cout << "[Success] Deleted question ID: " << question_id << "\n";

        return file_manager.SaveQuestions(questions);
    }

    // Deletes all thread questions for a parent question
    void DeleteThreadQuestions(int parent_id, const User& current_user) {
        vector<int> threads_to_delete;

        // Collect all thread IDs
        for (const auto& [id, question] : questions) {
            if (question.getParentId() == parent_id) {
                threads_to_delete.push_back(id);
            }
        }

        // Delete each thread
        for (int thread_id : threads_to_delete) {
            const Question& thread = questions[thread_id];

            if (thread.getFromUserId() != current_user.getId() && current_user.getRole() == User::REGULAR_USER) {
                cout << "[Skipped] Cannot delete thread question ID " << thread_id << " (Not your question).\n";
                continue;
            }

            questions.erase(thread_id);
            cout << "[Success] Deleted thread question ID: " << thread_id << "\n";
        }

        if (!threads_to_delete.empty()) {
            file_manager.SaveQuestions(questions);
        }
    }

};

class AskMeSystem {
private:
    AuthService auth_service;
    UserManager user_manager;
    QuestionManager question_manager;

    void PrintHeader(const string& title) {
        cout << "\n┌─────────────────────────────────────────────────────┐\n";
        cout << "│" << centerAlign(title, 53) << "│\n";
        cout << "└─────────────────────────────────────────────────────┘\n";
    }

    string centerAlign(const string& text, int width) {
        if (text.length() >= width) return text;
        int padding = (width - text.length()) / 2;
        return string(padding, ' ') + text + string(width - text.length() - padding, ' ');
    }

    void PrintMenuOption(int num, const string& text) {
        string option = to_string(num) + ".";
        cout << "│ " << left << setw(4) << option << setw(47) << text << " │\n";
    }

    void PrintDivider() {
        cout << "├─────────────────────────────────────────────────────┤\n";
    }

    void PrintFooter() {
        cout << "└─────────────────────────────────────────────────────┘\n";
    }

    // Main entry menu (unauthenticated users)
    void ShowMainMenu() {
        PrintHeader("ASK ME SYSTEM");
        PrintMenuOption(1, "Login");
        PrintMenuOption(2, "Sign Up");
        PrintMenuOption(3, "Exit");
        PrintFooter();
        cout << "> Select an option [1-3]: ";
    }
    
    // Regular user menu
    void ShowUserMenu(const User& user) {
        PrintHeader("USER MENU - " + user.getName());
        PrintMenuOption(1, "View Questions To Me");
        PrintMenuOption(2, "View Questions From Me");
        PrintMenuOption(3, "Ask Question");
        PrintMenuOption(4, "Answer Question");
        PrintMenuOption(5, "Delete My Question");
        PrintMenuOption(6, "View Thread Questions");
        PrintMenuOption(7, "Logout");
        PrintFooter();
        cout << "> Select an option [1-7]: ";
    }
    
    // Administrator menu
    void ShowAdminMenu(const User& admin) {
        PrintHeader("ADMIN MENU - " + admin.getName());
        
        // User management
        PrintDivider();
        cout << "│ " << left << setw(49) << "  USER MANAGEMENT" << "   │\n";
        PrintDivider();
        PrintMenuOption(1, "List All Users");
        PrintMenuOption(2, "Delete User");
        
        // Question management
        PrintDivider();
        cout << "│ " << left << setw(49) << "  QUESTION MANAGEMENT" << "   │\n";
        PrintDivider();
        PrintMenuOption(3, "View All Questions (Feed)");
        PrintMenuOption(4, "Delete Any Question");
        
        // System
        PrintDivider();
        cout << "│ " << left << setw(49) << "  SYSTEM" << "   │\n";
        PrintDivider();
        PrintMenuOption(5, "View Thread Questions");
        PrintMenuOption(6, "Logout");
        
        PrintFooter();
        cout << "> Select an option [1-6]: ";
    }

public:
    AskMeSystem() : auth_service(user_manager), question_manager(user_manager) {}
    // Runs the main system loop
    void Run() {
        while (true) {
            while (!auth_service.IsLoggedIn()) {
                ShowMainMenu();
                int option;
                cin >> option;
    
                switch (option) {
                    case 1:
                        auth_service.Login();
                        break;
                    case 2:
                        auth_service.SignUp();
                        break;
                    case 3:
                        cout << "Goodbye!\n";
                        return;
                    default:
                        cout << "Invalid option. Try again...\n";
                }
            }
    
            User current_user = auth_service.GetCurrentUser();
    
            if (current_user.getRole() == User::ADMIN) {
                // Admin menu loop
                bool back_to_main = false;
                while (auth_service.IsLoggedIn() && !back_to_main) {
                    ShowAdminMenu(current_user);
                    int choice;
                    cin >> choice;
    
                    switch (choice) {
                        case 1:
                            user_manager.ListSystemUsers();
                            break;
                        case 2: {
                            int id;
                            cout << "Enter user ID to delete: ";
                            cin >> id;
                            user_manager.DeleteUser(id);
                            break;
                        }
                        case 3:
                            question_manager.GetFeed(current_user);
                            break;
                        case 4: {
                            int qid;
                            cout << "Enter question ID to delete: ";
                            cin >> qid;
                            question_manager.DeleteQuestion(qid, current_user);
                            break;
                        }
                        case 5:
                            question_manager.GetThreadQuestions();
                            break;
                        case 6:
                            auth_service.Logout();
                            back_to_main = true;
                            break;
                        default:
                            cout << "Invalid option. Try again.\n";
                    }
                }
    
            } else {
                // Regular user menu loop
                bool back_to_main = false;
                while (auth_service.IsLoggedIn() && !back_to_main) {
                    ShowUserMenu(current_user);
                    int choice;
                    cin >> choice;
    
                    switch (choice) {
                        case 1:
                            question_manager.PrintQuestionsToUser(current_user.getId());
                            break;
                        case 2:
                            question_manager.PrintQuestionsFromUser(current_user.getId());
                            break;
                        case 3:
                            question_manager.AskQuestion(current_user);
                            break;
                        case 4: {
                            question_manager.AnswerQuestion(current_user.getId());
                            break;
                        }
                        case 5: {
                            int qid;
                            cout << "Enter question ID to delete: ";
                            cin >> qid;
                            question_manager.DeleteQuestion(qid, current_user);
                            break;
                        }
                        case 6:
                            question_manager.GetThreadQuestions();
                            break;
                        case 7:
                            auth_service.Logout();
                            back_to_main = true;
                            break;
                        default:
                            cout << "Invalid option. Try again.\n";
                    }
                }
            }
        }
    }
};

int main() {
    AskMeSystem system;
    system.Run();
    return 0;
}