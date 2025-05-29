#include <bits/stdc++.h>
using namespace std;

enum class SortOption {
    ID,
    NAME
};

class Book {
private:
    int id {};
    string name {};

public:
    Book() {}
    Book(int id, string name) : id(id), name(name) {}
    Book(const Book& other) : id(other.id), name(other.name) {}

    int getId() const {
        return id;
    }

    string getName() const {
        return name;
    }
};

class BookInfo {
private:
    Book book;
    int total_quantity;
    int total_borrowed;

public:
    BookInfo(const Book& book, int total_quantity) :
        book(book), total_quantity(total_quantity), total_borrowed(0) {}

    const Book& getBook() const {
        return book;
    }

    int getTotalQuantity() const {
        return total_quantity;
    }

    int getTotalBorrowed() const {
        return total_borrowed;
    }

    void setBook(const Book& newBook) {
        book = newBook;
    }

    void adjustQuantity(int delta) {
        total_quantity += delta;
    }

    void adjustBorrowed(int delta) {
        total_borrowed += delta;
    }
};

class BookInventory {
private:
    unordered_map<int, BookInfo> inventory;

public:
    void addBook(const Book& book, int total_quantity) {
        inventory.emplace(book.getId(), BookInfo(book, total_quantity));
    }

    BookInfo& getBookInfo(int bookId) {
        return inventory.at(bookId);
    }

    const BookInfo& getBookInfo(int bookId) const {
        return inventory.at(bookId);
    }

    const unordered_map<int, BookInfo>& getInventory() const {
        return inventory;
    }

    void updateBookInfo(int bookId, const Book& updatedInfo) {
        auto it = inventory.find(bookId);
        if (it != inventory.end()) {
            it->second.setBook(updatedInfo);
            cout << "Book information has been updated successfully!\n";
        } else {
            cout << "There is no book with this ID. Please try again!\n";
        }
    }

    bool checkAvailability(int bookId) const {
        auto it = inventory.find(bookId);
        return it != inventory.end() && it->second.getTotalQuantity() > it->second.getTotalBorrowed();
    }

    void adjustQuantity(int bookId, int delta) {
        auto it = inventory.find(bookId);
        if (it != inventory.end()) {
            it->second.adjustQuantity(delta);
        } else {
            cout << "Book not found in inventory!\n";
        }
    }
    
    bool bookExists(int bookId) const {
        return inventory.find(bookId) != inventory.end();
    }
};

class BookService {
private:
    BookInventory& bookInventory;

public:
    BookService(BookInventory& bookInventory) : bookInventory(bookInventory) {}

    BookInventory& getBookInventory() {
        return bookInventory;
    }

    void addBook(const Book& book, int total_quantity) {
        bookInventory.addBook(book, total_quantity);
        cout << "Book added successfully!\n";
    }

    void readAndAddBook() {
        cout << "Enter Book info: \n";
        
        cout << "Book ID: ";
        int id;
        cin >> id;
        
        cin.ignore(); 
        cout << "Book Name: ";
        string name;
        getline(cin, name);
        
        cout << "Total quantity of this Book: ";
        int total_quantity;
        cin >> total_quantity;
        
        Book book(id, name);
        addBook(book, total_quantity);
    }

    void listBooks(SortOption sortBy) {
        auto inventory = bookInventory.getInventory();
        vector<pair<int, const BookInfo*>> books;
        
        for (const auto& item : inventory) {
            books.push_back({item.first, &item.second});
        }
        
        if (sortBy == SortOption::ID) {
            sort(books.begin(), books.end(),
                [](const auto& a, const auto& b) {
                    return a.first < b.first;
                });
        } else if (sortBy == SortOption::NAME) {
            sort(books.begin(), books.end(),
                [](const auto& a, const auto& b) {
                    return a.second->getBook().getName() < b.second->getBook().getName();
                });
        } else {
            cout << "Invalid sort option!\n";
            return;
        }
        
        for (const auto& book : books) {
            cout << "ID: " << book.second->getBook().getId()
                << "\tName: " << book.second->getBook().getName()
                << "\tTotal Quantity: " << book.second->getTotalQuantity()
                << "\tTotal Borrowed: " << book.second->getTotalBorrowed() << '\n';
        }
    }

    vector<Book> searchBooksByPrefix(const string& prefix) const {
        vector<Book> res;
        for (const auto& book : bookInventory.getInventory()) { // It's better to ask BookInventory to return its Books
            const string& bookName = book.second.getBook().getName();
            if (bookName.size() >= prefix.size() && bookName.substr(0, prefix.size()) == prefix) {
                res.push_back(book.second.getBook());
            }
        }
        return res;
    }

    void readAndSearchBooksByPrefix() {
        cout << "Enter book name prefix: ";
        string prefix;
        cin >> prefix;
        
        vector<Book> books = searchBooksByPrefix(prefix);
        
        if (books.empty()) {
            cout << "No books found with the prefix \"" << prefix << "\".\n";
            return;
        }
        
        cout << "Books matching prefix \"" << prefix << "\":\n";
        for (size_t i = 0; i < books.size(); ++i) {
            cout << i + 1 << ") ID: " << books[i].getId() << " - Name: " << books[i].getName() << '\n';
        }
    }
};

class User {
private:
    int id {};
    string name {};

public:
    User() {}
    User(int id, string name) : id(id), name(name) {}
    User(const User& other) : id(other.id), name(other.name) {}

    int getId() const {
        return id;
    }

    string getName() const {
        return name;
    }
    
    bool operator==(const User& other) const {
        return id == other.id;
    }
};

namespace std {
    template<>
    struct hash<User> {
        size_t operator()(const User& user) const {
            return hash<int>()(user.getId());
        }
    };
}

class UserService {
private:
    vector<User> users;

public:
    void listUsers(SortOption sortOption) const {
        cout << "All Users in the system: \n";
        vector<User> sortedUsers = users;

        if (sortOption == SortOption::ID) {
            sort(sortedUsers.begin(), sortedUsers.end(),
                [](const User& a, const User& b) {
                    return a.getId() < b.getId();
                });
        } else if (sortOption == SortOption::NAME) {
            sort(sortedUsers.begin(), sortedUsers.end(),
                [](const User& a, const User& b) {
                    return a.getName() < b.getName();
                });
        } else {
            cout << "Invalid sort option!\n";
            return;
        }

        for (size_t i = 0; i < sortedUsers.size(); ++i)
            cout << i + 1 << ") " << sortedUsers[i].getName() << '\n';
    }

    void updateUserDetails(int userId, const User& updatedInfo) {
        auto it = find_if(users.begin(), users.end(),
            [userId](const User& user) {
                return user.getId() == userId;
            });

        if (it != users.end()) {
            *it = updatedInfo;
            cout << "User details updated successfully!\n";
        } else {
            cout << "User not found!\n";
        }
    }

    void registerUser(const User& user) {
        users.push_back(user);
        cout << "User registered successfully!\n";
    }

    void readAndRegisterUser() {
        cout << "Enter User info: \n";
        
        cout << "User ID: ";
        int id;
        cin >> id;
        
        cin.ignore();
        cout << "User Name: ";
        string name;
        getline(cin, name);
        
        User user(id, name);
        registerUser(user);
    }

    bool userExist(int user_id) const {
        for (const auto& user : users) {
            if (user.getId() == user_id)
                return true;
        }
        return false;
    }
    
    const User* getUserById(int user_id) const {
        for (const auto& user : users) {
            if (user.getId() == user_id)
                return &user;
        }
        return nullptr;
    }
};

class LoanService {
private:
    struct BorrowRecord {
        int userId;
        int bookId;
        BorrowRecord(int userId, int bookId) : userId(userId), bookId(bookId) {}
    };

    BookInventory& bookInventory;
    UserService& userService;
    vector<BorrowRecord> borrowRecords;
public:
    LoanService(BookInventory& inventory, UserService& users)
        : bookInventory(inventory), userService(users) {}

    bool borrowBook(int bookId, int userId) {
        if (!bookInventory.bookExists(bookId)) {
            cout << "This book doesn't exist in the inventory.\n";
            return false;
        }
        
        if (!bookInventory.checkAvailability(bookId)) {
            cout << "This book is not available for borrowing.\n";
            return false;
        }

        if (!userService.userExist(userId)) {
            cout << "User doesn't exist! Try registering first.\n";
            return false;
        }

        borrowRecords.emplace_back(userId, bookId);
        bookInventory.getBookInfo(bookId).adjustBorrowed(1);
        cout << "Book borrowed successfully!\n";
        return true;
    }

    void readAndBorrowBook() {
        cout << "Enter User ID: ";
        int userId;
        cin >> userId;
        
        cout << "Enter Book ID: ";
        int bookId;
        cin >> bookId;
        
        borrowBook(bookId, userId);
    }
    
    bool returnBook(int bookId, int userId) {
        auto it = find_if(borrowRecords.begin(), borrowRecords.end(),
            [bookId, userId](const BorrowRecord& record) {
                return record.bookId == bookId && record.userId == userId;
            });
        
        if (it == borrowRecords.end()) {
            cout << "No record found of this user borrowing this book.\n";
            return false;
        }
        
        borrowRecords.erase(it);
        bookInventory.getBookInfo(bookId).adjustBorrowed(-1);
        cout << "Book returned successfully!\n";
        return true;
    }

    void readAndReturnBook() {
        cout << "Enter User ID: ";
        int userId;
        cin >> userId;
        
        cout << "Enter Book ID: ";
        int bookId;
        cin >> bookId;
        
        returnBook(bookId, userId);
    }
    
    vector<Book> listLoansForUser(int userId) const {
        vector<Book> books;
        for (const auto& record : borrowRecords) {
            if (record.userId == userId && bookInventory.bookExists(record.bookId)) {
                books.push_back(bookInventory.getBookInfo(record.bookId).getBook());
            }
        }
        return books;
    }

    vector<User> listBorrowers(int bookId) const {
        vector<User> res;
        for (const auto& borrowRecord : borrowRecords) {
            if (borrowRecord.bookId == bookId) {
                const User* user = userService.getUserById(borrowRecord.userId);
                if (user) {
                    res.push_back(*user);
                }
            }
        }
        return res;
    }
    
    void printBorrowersByBookId(int bookId) const {
        if (!bookInventory.bookExists(bookId)) {
            cout << "Book doesn't exist in the inventory.\n";
            return;
        }
        
        const Book& book = bookInventory.getBookInfo(bookId).getBook();
        cout << "Users who borrowed book \"" << book.getName() << "\" (ID: " << book.getId() << "):\n";
        
        vector<User> borrowers = listBorrowers(bookId);
        if (borrowers.empty()) {
            cout << "No users have borrowed this book.\n";
            return;
        }
        
        for (size_t i = 0; i < borrowers.size(); ++i) {
            cout << i + 1 << ") " << borrowers[i].getName() 
                << " (ID: " << borrowers[i].getId() << ")\n";
        }
    }
    
    void printBorrowersByBookName(const string& bookName) const {
        bool found = false;
        for (const auto& item : bookInventory.getInventory()) {
            const Book& book = item.second.getBook();
            if (book.getName() == bookName) {
                printBorrowersByBookId(book.getId());
                found = true;
            }
        }
        
        if (!found) {
            cout << "No book with name \"" << bookName << "\" found in the inventory.\n";
        }
    }

    void readAndPrintBorrowersByBookName() {
        cout << "Enter book name: ";
        string name;
        getline(cin, name);
        
        printBorrowersByBookName(name);
    }
};

class AdminService {
private:
    UserService& userService;
    BookService& bookService;

public:
    AdminService(UserService& users, BookService& books)
        : userService(users), bookService(books) {}

    void addUser(const User& user) {
        userService.registerUser(user);
    }

    void readAndAddUser() {
        userService.readAndRegisterUser();
    }

    void printLibraryById() const {
        cout << "Library Books (sorted by ID):\n";
        bookService.listBooks(SortOption::ID);
    }

    void printLibraryByName() const {
        cout << "Library Books (sorted by Name):\n";
        bookService.listBooks(SortOption::NAME);
    }
    
    void printUsers() const {
        userService.listUsers(SortOption::NAME);
    }
};

class LibraryManager {
private:
    BookInventory inventory;
    BookService bookService;
    UserService userService;
    LoanService loanService;
    AdminService adminService;
    bool running;

public:
    LibraryManager() :
        bookService(inventory),
        loanService(inventory, userService),
        adminService(userService, bookService),
        running(true) {}

    void displayMenu() const {
        cout << "\n========================================\n";
        cout << "              Library Menu              \n";
        cout << "========================================\n";
        cout << setw(2) << "1)" << " Add Book\n";
        cout << setw(2) << "2)" << " Search Books by Prefix\n";
        cout << setw(2) << "3)" << " Print Who Borrowed Book by Name\n";
        cout << setw(2) << "4)" << " Print Library by ID\n";
        cout << setw(2) << "5)" << " Print Library by Name\n";
        cout << setw(2) << "6)" << " Add User\n";
        cout << setw(2) << "7)" << " User Borrow Book\n";
        cout << setw(2) << "8)" << " User Return Book\n";
        cout << setw(2) << "9)" << " Print Users\n";
        cout << setw(3) << "10)" << " Exit\n";
        cout << "========================================\n";
        cout << "Enter your menu choice [1 - 10]: ";
    }

    void run() {
        while (running) {
            displayMenu();
            int choice;
            cin >> choice;
            cin.ignore();

            switch (choice) {
                case 1:
                    bookService.readAndAddBook();
                    break;
                case 2:
                    bookService.readAndSearchBooksByPrefix();
                    break;
                case 3:
                    loanService.readAndPrintBorrowersByBookName();
                    break;
                case 4:
                    adminService.printLibraryById();
                    break;
                case 5:
                    adminService.printLibraryByName();
                    break;
                case 6:
                    adminService.readAndAddUser();
                    break;
                case 7:
                    loanService.readAndBorrowBook();
                    break;
                case 8:
                    loanService.readAndReturnBook();
                    break;
                case 9:
                    adminService.printUsers();
                    break;
                case 10:
                    cout << "Exiting the library system. Goodbye!\n";
                    running = false;
                    break;
                default:
                    cout << "Invalid choice. Please try again.\n";
                    break;
            }
        }
    }
};


//////////////////////////
//  Black Box Testing   //
//////////////////////////


void testAddBook() {
    cout << "Test: Add Book" << endl;
    BookInventory inventory;
    BookService bookService(inventory);
    Book book(101, "C++ Primer");
    bookService.addBook(book, 5);
    assert(inventory.bookExists(101));
    const BookInfo& info = inventory.getBookInfo(101);
    assert(info.getTotalQuantity() == 5);
    cout << "Passed: Add Book" << endl;
}

void testListBooks() {
    cout << "Test: List Books" << endl;
    BookInventory inventory;
    BookService bookService(inventory);
    bookService.addBook(Book(101, "C++ Primer"), 5);
    bookService.addBook(Book(102, "Effective C++"), 3);
    bookService.addBook(Book(103, "Algorithms"), 2);
    
    cout << "\nListing books sorted by ID:" << endl;
    bookService.listBooks(SortOption::ID);
    
    cout << "\nListing books sorted by Name:" << endl;
    bookService.listBooks(SortOption::NAME);
    
    cout << "Passed: List Books" << endl;
}

void testSearchBooksByPrefix() {
    cout << "Test: Search Books by Prefix" << endl;
    BookInventory inventory;
    BookService bookService(inventory);
    bookService.addBook(Book(101, "C++ Primer"), 5);
    bookService.addBook(Book(102, "Effective C++"), 3);
    
    vector<Book> results = bookService.searchBooksByPrefix("C++");
    assert(!results.empty());
    for (const auto& b : results) {
        assert(b.getName().substr(0, 3) == "C++");
    }
    cout << "Passed: Search Books by Prefix" << endl;
}

void testRegisterUser() {
    cout << "Test: Register User" << endl;
    UserService userService;
    User user(1, "Alice");
    userService.registerUser(user);
    assert(userService.userExist(1));
    cout << "Passed: Register User" << endl;
}

void testBorrowAndReturnBook() {
    cout << "Test: Borrow and Return Book" << endl;
    BookInventory inventory;
    BookService bookService(inventory);
    UserService userService;
    
    bookService.addBook(Book(101, "C++ Primer"), 5);
    userService.registerUser(User(1, "Alice"));
    
    LoanService loanService(inventory, userService);
    
    bool borrowed = loanService.borrowBook(101, 1);
    assert(borrowed);
    const BookInfo& infoAfterBorrow = inventory.getBookInfo(101);
    assert(infoAfterBorrow.getTotalBorrowed() == 1);
    
    bool returned = loanService.returnBook(101, 1);
    assert(returned);
    const BookInfo& infoAfterReturn = inventory.getBookInfo(101);
    assert(infoAfterReturn.getTotalBorrowed() == 0);
    
    cout << "Passed: Borrow and Return Book" << endl;
}

void testAdminFunctions() {
    cout << "Test: Admin Functions" << endl;
    BookInventory inventory;
    BookService bookService(inventory);
    UserService userService;
    LoanService loanService(inventory, userService);
    AdminService adminService(userService, bookService);
    
    adminService.addUser(User(1, "Alice"));
    assert(userService.userExist(1));
    
    bookService.addBook(Book(101, "C++ Primer"), 5);
    bookService.addBook(Book(102, "Effective C++"), 3);
    
    cout << "\nAdmin printing library sorted by ID:" << endl;
    adminService.printLibraryById();
    
    cout << "\nAdmin printing library sorted by Name:" << endl;
    adminService.printLibraryByName();
    
    cout << "\nAdmin printing all users:" << endl;
    adminService.printUsers();
    
    cout << "Passed: Admin Functions" << endl;
}

int main() {
    testAddBook();
    testListBooks();
    testSearchBooksByPrefix();
    testRegisterUser();
    testBorrowAndReturnBook();
    testAdminFunctions();
    
    cout << "\nAll black-box tests passed successfully." << endl;
    return 0;
}