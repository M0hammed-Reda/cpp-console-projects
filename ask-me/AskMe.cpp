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
    UserManager() {
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
        if (user.getRole() != User::ADMIN) {

        }

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


class BlackBoxTester {
private:
    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;
    string test_users_file = "test_users.txt";
    string test_questions_file = "test_questions.txt";
    
    // Test results tracking
    struct TestResult {
        string test_name;
        bool passed;
        string error_message;
    };
    
    vector<TestResult> test_results;
    
    // Helper method to print test results
    void printTestResult(const string& test_name, bool passed, const string& error_msg = "") {
        total_tests++;
        TestResult result = {test_name, passed, error_msg};
        test_results.push_back(result);
        
        if (passed) {
            passed_tests++;
            cout << "✓ PASS: " << test_name << endl;
        } else {
            failed_tests++;
            cout << "✗ FAIL: " << test_name;
            if (!error_msg.empty()) {
                cout << " - " << error_msg;
            }
            cout << endl;
        }
    }
    
    // Helper to create test files with sample data
    void setupTestFiles() {
        // Create test users file
        ofstream users_file(test_users_file);
        users_file << "1,Admin User,admin123,admin,admin@test.com,1,0\n";
        users_file << "2,Regular User,user123,user1,user1@test.com,1,1\n";
        users_file << "3,No Anon User,pass123,user2,user2@test.com,0,1\n";
        users_file.close();
        
        // Create test questions file
        ofstream questions_file(test_questions_file);
        questions_file << "1,-1,2,1,0,What is your favorite color?,Blue is my favorite\n";
        questions_file << "2,-1,1,2,1,How are you today?,I'm doing great thanks!\n";
        questions_file << "3,1,1,2,0,Follow up question,This is a follow up\n";
        questions_file.close();
    }
    
    // Helper to cleanup test files
    void cleanupTestFiles() {
        remove(test_users_file.c_str());
        remove(test_questions_file.c_str());
    }
    
    // Capture cout output for testing
    stringstream captured_output;
    streambuf* original_cout;
    
    void startCapturingOutput() {
        original_cout = cout.rdbuf();
        cout.rdbuf(captured_output.rdbuf());
    }
    
    void stopCapturingOutput() {
        cout.rdbuf(original_cout);
    }
    
    string getCapturedOutput() {
        return captured_output.str();
    }
    
    void clearCapturedOutput() {
        captured_output.str("");
        captured_output.clear();
    }

public:
    BlackBoxTester() {
        cout << "========================================" << endl;
        cout << "    BLACK BOX TESTING - ASK ME SYSTEM  " << endl;
        cout << "========================================" << endl;
        setupTestFiles();
    }
    
    ~BlackBoxTester() {
        cleanupTestFiles();
    }
    
    // Test User Class Functionality
    void testUserClass() {
        cout << "\n--- Testing User Class ---" << endl;
        
        try {
            // Test 1: User creation with all parameters
            User user1(1, "Test User", "password123", "testuser", "test@email.com", true, User::REGULAR_USER);
            bool test1 = (user1.getId() == 1 && 
                         user1.getName() == "Test User" && 
                         user1.getUsername() == "testuser" &&
                         user1.getEmail() == "test@email.com" &&
                         user1.getAllowAnonymousQuestions() == true &&
                         user1.getRole() == User::REGULAR_USER);
            printTestResult("User Constructor with Full Parameters", test1);
            
            // Test 2: Default user constructor
            User user2;
            bool test2 = (user2.getId() == 0 && 
                         user2.getAllowAnonymousQuestions() == false &&
                         user2.getRole() == User::REGULAR_USER);
            printTestResult("User Default Constructor", test2);
            
            // Test 3: Password verification
            bool test3a = user1.verifyPassword("password123");
            bool test3b = !user1.verifyPassword("wrongpassword");
            printTestResult("User Password Verification", test3a && test3b);
            
            // Test 4: Admin user creation
            User admin(2, "Admin", "admin123", "admin", "admin@test.com", false, User::ADMIN);
            bool test4 = (admin.getRole() == User::ADMIN);
            printTestResult("Admin User Creation", test4);
            
            // Test 5: User toString method
            string userString = user1.toString();
            bool test5 = (userString.find("1") != string::npos && 
                         userString.find("Test User") != string::npos);
            printTestResult("User toString Method", test5);
            
            // Test 6: Anonymous questions setting
            user1.setAllowAnonymousQuestions(false);
            bool test6 = !user1.getAllowAnonymousQuestions();
            printTestResult("Set Allow Anonymous Questions", test6);
            
        } catch (const exception& e) {
            printTestResult("User Class Tests", false, e.what());
        }
    }
    
    // Test Question Class Functionality
    void testQuestionClass() {
        cout << "\n--- Testing Question Class ---" << endl;
        
        try {
            // Test 1: Question creation with all parameters
            Question q1(1, -1, 1, 2, false, "What is your name?", "My name is John");
            bool test1 = (q1.getId() == 1 &&
                         q1.getParentId() == -1 &&
                         q1.getFromUserId() == 1 &&
                         q1.getToUserId() == 2 &&
                         !q1.getIsAnonymous() &&
                         q1.getText() == "What is your name?" &&
                         q1.getAnswer() == "My name is John");
            printTestResult("Question Constructor with Full Parameters", test1);
            
            // Test 2: Default question constructor
            Question q2;
            bool test2 = (q2.getId() == 0 && 
                         q2.getParentId() == -1 &&
                         q2.getFromUserId() == 0 &&
                         q2.getToUserId() == 0 &&
                         !q2.getIsAnonymous());
            printTestResult("Question Default Constructor", test2);
            
            // Test 3: Question without answer
            Question q3(2, -1, 2, 1, true, "How are you?");
            bool test3 = (q3.getAnswer().empty() && !q3.isAnswered());
            printTestResult("Question Without Answer", test3);
            
            // Test 4: Setting answer
            q3.setAnswer("I'm doing well, thank you!");
            bool test4 = (q3.getAnswer() == "I'm doing well, thank you!" && q3.isAnswered());
            printTestResult("Question Set Answer", test4);
            
            // Test 5: Anonymous question
            bool test5 = q3.getIsAnonymous();
            printTestResult("Anonymous Question Flag", test5);
            
            // Test 6: Thread question (with parent)
            Question q4(3, 1, 1, 2, false, "Follow up question");
            bool test6 = (q4.getParentId() == 1);
            printTestResult("Thread Question with Parent", test6);
            
            // Test 7: Question toString method
            string qString = q1.toString();
            bool test7 = (qString.find("1") != string::npos && 
                         qString.find("What is your name?") != string::npos);
            printTestResult("Question toString Method", test7);
            
        } catch (const exception& e) {
            printTestResult("Question Class Tests", false, e.what());
        }
    }
    
    // Test FileManager Class Functionality
    void testFileManager() {
        cout << "\n--- Testing FileManager Class ---" << endl;
        
        try {
            FileManager fm(test_users_file, test_questions_file);
            
            // Test 1: Reading existing files
            vector<string> user_lines = fm.ReadInformationFromFile(test_users_file);
            bool test1 = (!user_lines.empty() && user_lines.size() >= 3);
            printTestResult("FileManager Read Existing File", test1);
            
            // Test 2: Reading non-existent file
            vector<string> empty_lines = fm.ReadInformationFromFile("nonexistent.txt");
            bool test2 = empty_lines.empty();
            printTestResult("FileManager Read Non-existent File", test2);
            
            // Test 3: Writing to file
            vector<string> test_data = {"line1", "line2", "line3"};
            bool write_success = fm.StoreInformationOnFile("test_write.txt", test_data);
            bool test3 = write_success;
            printTestResult("FileManager Write to File", test3);
            
            // Test 4: Verify written data
            vector<string> read_back = fm.ReadInformationFromFile("test_write.txt");
            bool test4 = (read_back.size() == 3 && 
                         read_back[0] == "line1" && 
                         read_back[2] == "line3");
            printTestResult("FileManager Verify Written Data", test4);
            
            // Test 5: Load users from file
            unordered_map<int, User> users = fm.LoadUsers();
            bool test5 = (!users.empty() && users.size() >= 3);
            printTestResult("FileManager Load Users", test5);
            
            // Test 6: Load questions from file
            unordered_map<int, Question> questions = fm.LoadQuestions();
            bool test6 = (!questions.empty() && questions.size() >= 3);
            printTestResult("FileManager Load Questions", test6);
            
            // Test 7: Save users to file
            bool save_users = fm.SaveUsers(users);
            bool test7 = save_users;
            printTestResult("FileManager Save Users", test7);
            
            // Test 8: Save questions to file
            bool save_questions = fm.SaveQuestions(questions);
            bool test8 = save_questions;
            printTestResult("FileManager Save Questions", test8);
            
            // Cleanup test file
            remove("test_write.txt");
            
        } catch (const exception& e) {
            printTestResult("FileManager Tests", false, e.what());
        }
    }
    
    // Test UserManager Class Functionality
    void testUserManager() {
        cout << "\n--- Testing UserManager Class ---" << endl;
        
        try {
            UserManager um;
            
            // Test 1: Get next user ID
            int next_id = um.GetNextUserID();
            bool test1 = (next_id > 0);
            printTestResult("UserManager Get Next User ID", test1);
            
            // Test 2: Add new user
            User new_user(next_id, "New User", "newpass", "newuser", "new@test.com", true);
            bool add_success = um.AddUser(new_user);
            bool test2 = add_success;
            printTestResult("UserManager Add New User", test2);
            
            // Test 3: Get user by ID
            try {
                const User& retrieved_user = um.GetUserByID(next_id);
                bool test3 = (retrieved_user.getName() == "New User");
                printTestResult("UserManager Get User by ID", test3);
            } catch (const exception& e) {
                printTestResult("UserManager Get User by ID", false, e.what());
            }
            
            // Test 4: Authentication with correct credentials
            bool auth_success = um.Authenticate(next_id, "newpass");
            bool test4 = auth_success;
            printTestResult("UserManager Authentication Success", test4);
            
            // Test 5: Authentication with wrong credentials
            bool auth_fail = um.Authenticate(next_id, "wrongpass");
            bool test5 = !auth_fail;
            printTestResult("UserManager Authentication Failure", test5);
            
            // Test 6: Update user
            User updated_user = new_user;
            updated_user.setAllowAnonymousQuestions(false);
            bool update_success = um.UpdateUser(updated_user);
            bool test6 = update_success;
            printTestResult("UserManager Update User", test6);
            
            // Test 7: List system users (capture output)
            startCapturingOutput();
            um.ListSystemUsers();
            stopCapturingOutput();
            string output = getCapturedOutput();
            bool test7 = (output.find("New User") != string::npos);
            printTestResult("UserManager List System Users", test7);
            clearCapturedOutput();
            
            // Test 8: Delete user
            bool delete_success = um.DeleteUser(next_id);
            bool test8 = delete_success;
            printTestResult("UserManager Delete User", test8);
            
            // Test 9: Try to get deleted user (should fail)
            try {
                um.GetUserByID(next_id);
                printTestResult("UserManager Get Deleted User", false, "Should have thrown exception");
            } catch (const exception& e) {
                printTestResult("UserManager Get Deleted User", true);
            }
            
        } catch (const exception& e) {
            printTestResult("UserManager Tests", false, e.what());
        }
    }
    
    // Test QuestionManager Class Functionality
    void testQuestionManager() {
        cout << "\n--- Testing QuestionManager Class ---" << endl;
        
        try {
            UserManager um;
            QuestionManager qm(um);
            
            // Test 1: Get next question ID
            int next_id = qm.GetNextQuestionID();
            bool test1 = (next_id > 0);
            printTestResult("QuestionManager Get Next Question ID", test1);
            
            // Test 2: Add new question
            Question new_question(next_id, -1, 2, 1, false, "Test question?");
            bool add_success = qm.AddQuestion(new_question);
            bool test2 = add_success;
            printTestResult("QuestionManager Add New Question", test2);
            
            // Test 3: Update question with answer
            Question updated_question = new_question;
            updated_question.setAnswer("Test answer");
            bool update_success = qm.UpdateQuestion(updated_question);
            bool test3 = update_success;
            printTestResult("QuestionManager Update Question", test3);
            
            // Test 4: Print questions to user (capture output)
            startCapturingOutput();
            qm.PrintQuestionsToUser(1);
            stopCapturingOutput();
            string output = getCapturedOutput();
            bool test4 = (output.find("Question") != string::npos);
            printTestResult("QuestionManager Print Questions To User", test4);
            clearCapturedOutput();
            
            // Test 5: Print questions from user
            startCapturingOutput();
            qm.PrintQuestionsFromUser(2);
            stopCapturingOutput();
            output = getCapturedOutput();
            bool test5 = (output.find("Question") != string::npos);
            printTestResult("QuestionManager Print Questions From User", test5);
            clearCapturedOutput();
            
            // Test 6: Add anonymous question to user who allows it
            Question anon_question(next_id + 1, -1, 1, 2, true, "Anonymous test?");
            bool anon_success = qm.AddQuestion(anon_question);
            bool test6 = anon_success;
            printTestResult("QuestionManager Add Anonymous Question", test6);
            
            // Test 7: Try to add anonymous question to user who doesn't allow it
            Question blocked_anon(next_id + 2, -1, 1, 3, true, "Should be blocked");
            bool blocked_success = qm.AddQuestion(blocked_anon);
            bool test7 = !blocked_success;
            printTestResult("QuestionManager Block Anonymous Question", test7);
            
            // Test 8: Add thread question
            Question thread_question(next_id + 3, next_id, 1, 2, false, "Thread question");
            bool thread_success = qm.AddQuestion(thread_question);
            bool test8 = thread_success;
            printTestResult("QuestionManager Add Thread Question", test8);
            
            // Test 9: Delete question (create admin user first)
            User admin(999, "Test Admin", "admin", "admin", "admin@test.com", false, User::ADMIN);
            um.AddUser(admin);
            bool delete_success = qm.DeleteQuestion(next_id, admin);
            bool test9 = delete_success;
            printTestResult("QuestionManager Delete Question", test9);
            
        } catch (const exception& e) {
            printTestResult("QuestionManager Tests", false, e.what());
        }
    }
    
    // Test AuthService Class Functionality
    void testAuthService() {
        cout << "\n--- Testing AuthService Class ---" << endl;
        
        try {
            UserManager um;
            AuthService auth(um);
            
            // Test 1: Initial login state
            bool test1 = !auth.IsLoggedIn();
            printTestResult("AuthService Initial Login State", test1);
            
            // Test 2: Try to get current user when not logged in
            try {
                auth.GetCurrentUser();
                printTestResult("AuthService Get Current User Not Logged In", false, "Should throw exception");
            } catch (const exception& e) {
                printTestResult("AuthService Get Current User Not Logged In", true);
            }
            
            // Note: Login and SignUp methods require user input, so we test the underlying logic
            // by testing the UserManager authentication directly
            
            // Test 3: Authentication logic (via UserManager)
            bool auth_test = um.Authenticate(1, "admin123");
            bool test3 = auth_test;
            printTestResult("AuthService Authentication Logic", test3);
            
            // Test 4: Logout functionality
            auth.Logout();
            bool test4 = !auth.IsLoggedIn();
            printTestResult("AuthService Logout", test4);
            
        } catch (const exception& e) {
            printTestResult("AuthService Tests", false, e.what());
        }
    }
    
    // Integration Tests
    void testSystemIntegration() {
        cout << "\n--- Testing System Integration ---" << endl;
        
        try {
            // Test 1: Full user lifecycle
            UserManager um;
            int user_id = um.GetNextUserID();
            User test_user(user_id, "Integration User", "intpass", "intuser", "int@test.com", true);
            
            bool add_user = um.AddUser(test_user);
            bool auth_user = um.Authenticate(user_id, "intpass");
            const User& retrieved = um.GetUserByID(user_id);
            bool delete_user = um.DeleteUser(user_id);
            
            bool test1 = (add_user && auth_user && retrieved.getName() == "Integration User" && delete_user);
            printTestResult("System Integration User Lifecycle", test1);
            
            // Test 2: Question workflow
            QuestionManager qm(um);
            int question_id = qm.GetNextQuestionID();
            Question test_question(question_id, -1, 2, 1, false, "Integration test question?");
            
            bool add_question = qm.AddQuestion(test_question);
            test_question.setAnswer("Integration test answer");
            bool update_question = qm.UpdateQuestion(test_question);
            
            bool test2 = (add_question && update_question);
            printTestResult("System Integration Question Workflow", test2);
            
            // Test 3: File persistence
            FileManager fm(test_users_file, test_questions_file);
            unordered_map<int, User> users = fm.LoadUsers();
            unordered_map<int, Question> questions = fm.LoadQuestions();
            
            bool save_users = fm.SaveUsers(users);
            bool save_questions = fm.SaveQuestions(questions);
            
            bool test3 = (save_users && save_questions && !users.empty() && !questions.empty());
            printTestResult("System Integration File Persistence", test3);
            
        } catch (const exception& e) {
            printTestResult("System Integration Tests", false, e.what());
        }
    }
    
    // Edge Cases and Error Handling Tests
    void testEdgeCases() {
        cout << "\n--- Testing Edge Cases ---" << endl;
        
        try {
            UserManager um;
            QuestionManager qm(um);
            
            // Test 1: Add duplicate user ID
            User user1(1000, "User1", "pass1", "user1", "user1@test.com", true);
            User user2(1000, "User2", "pass2", "user2", "user2@test.com", false);
            
            bool add1 = um.AddUser(user1);
            bool add2 = um.AddUser(user2); // Should fail
            bool test1 = (add1 && !add2);
            printTestResult("Edge Case Duplicate User ID", test1);
            
            // Test 2: Question to non-existent user
            Question invalid_question(9999, -1, 1000, 99999, false, "Invalid question");
            bool add_invalid = qm.AddQuestion(invalid_question);
            bool test2 = !add_invalid;
            printTestResult("Edge Case Question to Non-existent User", test2);
            
            // Test 3: Update non-existent question
            Question non_existent(88888, -1, 1000, 1, false, "Non-existent");
            bool update_invalid = qm.UpdateQuestion(non_existent);
            bool test3 = !update_invalid;
            printTestResult("Edge Case Update Non-existent Question", test3);
            
            // Test 4: Get non-existent user
            try {
                um.GetUserByID(77777);
                printTestResult("Edge Case Get Non-existent User", false, "Should throw exception");
            } catch (const exception& e) {
                printTestResult("Edge Case Get Non-existent User", true);
            }
            
            // Test 5: Delete non-existent user
            bool delete_invalid = um.DeleteUser(66666);
            bool test5 = !delete_invalid;
            printTestResult("Edge Case Delete Non-existent User", test5);
            
            // Test 6: Empty strings in user creation
            User empty_user(1001, "", "", "", "", false);
            bool add_empty = um.AddUser(empty_user);
            bool test6 = add_empty; // Should still work, but not recommended
            printTestResult("Edge Case Empty User Fields", test6);
            
            // Cleanup
            um.DeleteUser(1000);
            um.DeleteUser(1001);
            
        } catch (const exception& e) {
            printTestResult("Edge Cases Tests", false, e.what());
        }
    }
    
    // Run all tests
    void runAllTests() {
        cout << "\nStarting comprehensive black box testing...\n" << endl;
        
        testUserClass();
        testQuestionClass();
        testFileManager();
        testUserManager();
        testQuestionManager();
        testAuthService();
        testSystemIntegration();
        testEdgeCases();
        
        printSummary();
    }
    
    // Print test summary
    void printSummary() {
        cout << "\n========================================" << endl;
        cout << "           TEST SUMMARY REPORT          " << endl;
        cout << "========================================" << endl;
        cout << "Total Tests:  " << total_tests << endl;
        cout << "Passed:       " << passed_tests << " (" << fixed << setprecision(1) 
             << (100.0 * passed_tests / total_tests) << "%)" << endl;
        cout << "Failed:       " << failed_tests << " (" << fixed << setprecision(1) 
             << (100.0 * failed_tests / total_tests) << "%)" << endl;
        cout << "========================================" << endl;
        
        if (failed_tests > 0) {
            cout << "\nFAILED TESTS DETAILS:" << endl;
            cout << "---------------------" << endl;
            for (const auto& result : test_results) {
                if (!result.passed) {
                    cout << "✗ " << result.test_name;
                    if (!result.error_message.empty()) {
                        cout << " - " << result.error_message;
                    }
                    cout << endl;
                }
            }
        }
        
        cout << "\nTesting completed!" << endl;
        
        if (failed_tests == 0) {
            cout << "🎉 All tests passed! Your system is working correctly." << endl;
        } else {
            cout << "⚠️  Some tests failed. Please review the failed tests above." << endl;
        }
    }
};

// Main function to run the tests
int main() {
    try {
        BlackBoxTester tester;
        tester.runAllTests();
    } catch (const exception& e) {
        cout << "Critical error during testing: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}